# Z-Order (Morton Code) Sorting - Technical Documentation

## Overview

Z-Order Sorting은 GPU XPBD 유체 시뮬레이션에서 이웃 탐색 성능을 최적화하기 위한 공간 정렬 기법입니다. 파티클들을 Morton Code(Z-Order Curve)에 따라 정렬하여, 공간적으로 가까운 파티클들이 메모리상에서도 연속적으로 배치되도록 합니다.

## Implementation Architecture

### Pipeline Overview

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Morton Code    │ -> │   Radix Sort    │ -> │    Reorder      │ -> │  CellStart/End  │
│   Computation   │    │  (6 passes)     │    │   Particles     │    │   Computation   │
└─────────────────┘    └─────────────────┘    └─────────────────┘    └─────────────────┘
  FluidMortonCode.usf   FluidRadixSort.usf   FluidReorderParticles.usf   FluidCellStartEnd.usf
```

### Key Files

| File | Purpose |
|------|---------|
| `FluidMortonCode.usf` | Morton Code 계산 셰이더 |
| `FluidRadixSort.usf` | GPU Radix Sort 구현 |
| `FluidReorderParticles.usf` | 파티클 데이터 재정렬 |
| `FluidCellStartEnd.usf` | 셀별 시작/끝 인덱스 계산 |
| `GPUFluidSimulator.cpp` | C++ 파이프라인 구현 |
| `GPUFluidSimulatorShaders.h` | 셰이더 클래스 정의 및 상수 |

## How It Works

### Step 1: Morton Code Computation

Morton Code는 3D 좌표를 1D 정수로 인코딩하여 공간적 지역성을 보존합니다.

```hlsl
// 7-bit per axis -> 21-bit Morton code
uint Morton3D_7bit(uint x, uint y, uint z)
{
    x = min(x, 127u);  // 7 bits (0-127)
    y = min(y, 127u);
    z = min(z, 127u);

    // Bit interleaving: z6y6x6 z5y5x5 ... z0y0x0
    uint xx = ExpandBits7(x);
    uint yy = ExpandBits7(y);
    uint zz = ExpandBits7(z);

    return (zz << 2) | (yy << 1) | xx;
}
```

**Cell-based 접근**: 정규화된 위치 대신 그리드 셀 좌표를 사용하여 Spatial Hash와의 일관성을 유지합니다.

```hlsl
// PredictedPosition 사용 (Position 아님!)
float3 pos = Particles[idx].PredictedPosition;
int3 cellCoord = WorldToCell(pos, CellSize);
uint mortonCode = Morton3DFromCell(cellCoord, gridMin);

// 21-bit Morton code = Cell ID (truncation 불필요)
// MAX_CELLS = 128^3 = 2,097,152
```

### Step 2: Radix Sort

4-bit Radix Sort (16 buckets)를 6 passes 수행하여 21-bit Morton code를 정렬합니다.

```
Pass 구조:
1. Histogram: 각 thread group에서 digit별 개수 집계
2. Global Prefix Sum: 버킷별 전역 오프셋 계산
3. Bucket Prefix Sum: 버킷 시작 위치 계산
4. Scatter: 최종 위치로 데이터 이동
```

**Stable Sort 구현**: InterlockedAdd의 비결정적 특성을 피하기 위해 per-thread prefix sum 방식 사용

```hlsl
// Per-thread digit counts로 결정적 오프셋 계산
uint outputPos = DigitBaseOffsets[digit]
               + ThreadDigitCounts[digit * THREAD_GROUP_SIZE + localIdx]
               + myLocalRank[digit];
```

### Step 3: Particle Reordering

정렬된 인덱스를 사용하여 파티클 데이터를 물리적으로 재배치합니다.

```hlsl
uint newIdx = DispatchThreadId.x;
uint oldIdx = SortedIndices[newIdx];
SortedParticles[newIdx] = OldParticles[oldIdx];
```

### Step 4: CellStart/End Computation

정렬된 배열에서 각 셀의 시작/끝 인덱스를 탐지합니다.

```hlsl
uint currentCode = SortedMortonCodes[idx];
// With 7-bit per axis: 21-bit Morton code = Cell ID directly (no masking needed)
uint cellID = currentCode;

if (idx == 0 || cellID != prevCellID)
{
    CellStart[cellID] = idx;
    if (idx > 0) CellEnd[prevCellID] = idx - 1;
}
```

## Performance Optimization

### What It Optimizes

1. **Cache Coherency**: 공간적으로 가까운 파티클이 메모리상 연속 배치
2. **Sequential Access**: Hash table linked-list 대신 순차 메모리 접근
3. **Memory Bandwidth**: 랜덤 접근 패턴 제거로 캐시 히트율 향상

### Memory Access Pattern Comparison

**Before (Hash Table)**:
```
for each neighborCell in 27 cells:
    head = HashTable[cellID]
    while head != INVALID:
        particleIdx = ParticleIndices[head]
        particle = Particles[particleIdx]  // Random access!
        head = next
```

**After (Z-Order)**:
```
for each neighborCell in 27 cells:
    cellID = GetMortonCellID(neighborCell)
    for i = CellStart[cellID] to CellEnd[cellID]:
        particle = SortedParticles[i]  // Sequential access!
```

### Constants

```cpp
// Z-Order Sorting Configuration (GPUFluidSimulatorShaders.h)
#define GPU_MORTON_GRID_AXIS_BITS 7                    // 7 bits per axis
#define GPU_MORTON_GRID_SIZE (1 << GPU_MORTON_GRID_AXIS_BITS)  // 128 (2^7)
#define GPU_MAX_CELLS (GPU_MORTON_GRID_SIZE * GPU_MORTON_GRID_SIZE * GPU_MORTON_GRID_SIZE)  // 2,097,152

// Auto-calculated from GridAxisBits
#define GPU_MORTON_CODE_BITS (GPU_MORTON_GRID_AXIS_BITS * 3)  // 7 × 3 = 21 bits
#define GPU_RADIX_SORT_PASSES ((GPU_MORTON_CODE_BITS + GPU_RADIX_BITS - 1) / GPU_RADIX_BITS)  // ceil(21/4) = 6 passes

// Radix Sort Configuration
#define GPU_RADIX_BITS 4               // 4-bit radix (16 buckets)
#define GPU_RADIX_SIZE 16
#define GPU_RADIX_THREAD_GROUP_SIZE 256
#define GPU_RADIX_ELEMENTS_PER_THREAD 4
#define GPU_RADIX_ELEMENTS_PER_GROUP 1024  // 256 * 4
```

## Troubleshooting History

### Issue 1: Position vs PredictedPosition Mismatch

**Problem**: Morton Code 계산에서 `Position`을 사용했으나, Solver에서는 `PredictedPosition`으로 이웃을 탐색하여 불일치 발생

**Symptom**: 파티클이 잘못된 이웃을 찾아 비물리적인 힘 적용

**Solution**: Morton Code 계산 시 `PredictedPosition` 사용
```hlsl
// CORRECT
float3 pos = Particles[idx].PredictedPosition;

// WRONG (causes mismatch)
float3 pos = Particles[idx].Position;
```

### Issue 2: Full Morton Code vs Truncated CellID (RESOLVED)

**Problem** (with old 10-bit per axis config): 30-bit Morton Code 전체로 정렬하면, 같은 셀에 속한 파티클들이 상위 비트 차이로 인해 분산됨

**Current Solution** (7-bit per axis): 21-bit Morton Code = Cell ID 직접 매핑으로 truncation 불필요
```hlsl
// With 7-bit per axis: Morton code IS the Cell ID (no truncation needed)
// 21-bit Morton code → 2,097,152 max cells
uint cellID = mortonCode;  // Direct mapping
```

### Issue 3: Bounds Overflow

**Problem**: 시뮬레이션 영역이 Morton Code 용량(128 cells per axis with 7-bit config)을 초과

**Symptom**: 파티클들이 같은 Morton Code를 가지게 되어 정렬 효과 감소

**Solution**: CellSize = SmoothingRadius로 설정하여 자동으로 적절한 bounds 계산
```cpp
// With GridAxisBits=7, SmoothingRadius=20cm:
// SimulationBounds = 128 × 20cm = 2560cm (25.6m) per axis
const float BoundsExtent = GPU_MORTON_GRID_SIZE * SmoothingRadius;
```

### Issue 4: Radix Sort Instability

**Problem**: InterlockedAdd를 사용한 Scatter에서 같은 digit를 가진 요소들의 순서가 비결정적

**Symptom**: 프레임마다 다른 정렬 결과, 시각적 떨림

**Solution**: Per-thread prefix sum 기반 stable scatter 구현
```hlsl
// Deterministic offset calculation
uint outputPos = DigitBaseOffsets[digit]
               + ThreadDigitCounts[digit * THREAD_GROUP_SIZE + localIdx]
               + myLocalRank[digit];
```

### Issue 5: Invalid CellStart/CellEnd Handling

**Problem**: 빈 셀에 대한 검사 누락으로 무한 루프 또는 잘못된 메모리 접근

**Symptom**: GPU hang 또는 크래시

**Solution**: 양쪽 모두 INVALID 검사
```hlsl
// CRITICAL: Check BOTH to prevent infinite loop
if (cellStart == INVALID_INDEX || cellEnd == INVALID_INDEX) continue;
```

## Debug Visualization

Z-Order 정렬 결과를 시각적으로 확인하는 Debug Draw 기능이 있습니다.

### Visualization Modes

- **ZOrderArrayIndex**: 배열 인덱스 기반 색상 (정렬 성공 시 공간적으로 부드러운 그라데이션)
- **ZOrderMortonCode**: Morton Code 기반 색상

### Usage

```cpp
// Blueprint or C++
FluidComponent->EnableDebugDraw(EFluidDebugVisualization::ZOrderArrayIndex);
```

**정렬 성공 확인**: ArrayIndex 모드에서 공간적으로 가까운 파티클들이 비슷한 색상을 가지면 정렬이 올바르게 동작하는 것입니다.

## Configuration

### Preset Bounds Settings

Simulation bounds are **auto-calculated** in `UKawaiiFluidPresetDataAsset` based on:

- `GridAxisBits` (global constant from GPUFluidSimulatorShaders.h)
- `SmoothingRadius` (from Physics section)

```cpp
// KawaiiFluidPresetDataAsset.h - Z-Order Sorting parameters

// Global constant (read-only, set by GPU_MORTON_GRID_AXIS_BITS)
int32 GridAxisBits = 7;  // 7 bits per axis

// Auto-calculated from GridAxisBits
int32 ZOrderGridResolution = 128;      // 2^7
int32 ZOrderMortonBits = 21;           // 7 × 3
int32 ZOrderMaxCells = 2097152;        // 128^3

// Auto-calculated from SmoothingRadius
float ZOrderCellSize = 20.0f;          // = SmoothingRadius
float ZOrderBoundsExtent = 2560.0f;    // GridResolution × CellSize

// Simulation bounds (centered at component origin)
FVector SimulationBoundsMin = FVector(-1280.0f, -1280.0f, -1280.0f);  // -BoundsExtent/2
FVector SimulationBoundsMax = FVector(1280.0f, 1280.0f, 1280.0f);     // +BoundsExtent/2

// Note: Wireframe visualization is always enabled in Editor mode (red color)
// No configuration needed - automatically shown when GPU simulation is active
```

### Bounds are Component-Relative

Preset bounds are defined **relative to the UKawaiiFluidComponent's world location**, not in absolute world coordinates:

- If Component is at `(1000, 0, 0)` and preset bounds are `(-500, -500, -500)` to `(500, 500, 500)`
- Actual world bounds become `(500, -500, -500)` to `(1500, 500, 500)`

This design allows the same preset to work for components placed anywhere in the level.

**Recommendation**: Set bounds large enough to cover expected particle movement area. Particles outside bounds will have degraded sorting performance.

### GPUFluidSimulator Settings

```cpp
// GPUFluidSimulator.h
bool bUseZOrderSorting = true;  // Enable/disable Z-Order sorting

// Set simulation bounds (called from SimulateGPU with preset + component offset)
void SetSimulationBounds(const FVector3f& BoundsMin, const FVector3f& BoundsMax);
```

### Performance Notes

- Small particle counts (<1024): Single-pass shared memory sort 사용
- Large particle counts: Multi-pass Radix Sort 사용
- Sorting cost: ~0.5ms for 100K particles
- Memory: Temporary buffers created as transient RDG resources

## References

- Morton Code (Z-Order Curve): https://en.wikipedia.org/wiki/Z-order_curve
- GPU Radix Sort: "Fast Parallel Sorting Algorithms on GPUs" (Satish et al.)
- SPH Spatial Hashing: "Optimized Spatial Hashing for Collision Detection of Deformable Objects" (Teschner et al.)
