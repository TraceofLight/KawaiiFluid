# GPUFluidSimulator 아키텍처 분석 (2026-01-13)

## 아키텍처 다이어그램

```
FGPUFluidSimulator (Facade/Orchestrator - 806줄 헤더)
├── FGPUSpawnManager             (파티클 생성 시스템)
├── FGPUStreamCompactionManager  (AABB 필터링 + 스트림 컴팩션)
├── FGPUCollisionManager         (충돌 시스템 통합)
│   └── FGPUCollisionFeedbackManager (내부 - 피드백 리드백)
├── FGPUSpatialHashManager       (Z-Order 정렬 + Morton 코드)
├── FGPUBoundarySkinningManager  (바운더리 스키닝 + Flex 어드히전)
└── FGPUAdhesionManager          (본 기반 부착 시스템)
```

## 매니저별 상세 분석

| 매니저 | 책임 (Passes/Functions) | Lock | 구현 상태 | LOC | 추출된 기능 |
|--------|-------------------------|------|----------|-----|-----------|
| **SpawnManager** | 스폰 요청 큐, GPU 스폰 패스, ParticleID 할당 | SpawnLock | ✅ 완료 | 180 | 파티클 스폰 시스템 전체 |
| **StreamCompactionManager** | AABB 필터링, Prefix Sum, 스트림 컴팩션, 리드백 | ❌ | ✅ 완료 | 599 | Per-polygon 충돌 필터링 |
| **CollisionManager** | Bounds/DF/Primitive 충돌, 피드백 시스템 | CollisionLock | ✅ 완료 | 604 | 모든 충돌 시스템 + 피드백 |
| **SpatialHashManager** | Morton 코드, Radix Sort (6-pass), Cell Start/End | ❌ | ✅ 완료 | 368 | 공간 해싱 전체 |
| **BoundarySkinningManager** | GPU 스키닝, 본 트랜스폼, Boundary Adhesion | SkinningLock | ✅ 완료 | 431 | 바운더리 파티클 시스템 |
| **AdhesionManager** | 파티클 부착, 업데이트, 탈착, Stack Pressure | AdhesionLock | ✅ 완료 | 453 | 본 부착 시스템 전체 |

**총 매니저 코드**: 2,635 LOC

---

## 1. SpawnManager (180 LOC)

### 책임
- Thread-safe 파티클 스폰 요청 큐 (Game Thread → Render Thread)
- Double-buffered 스폰 요청 (PendingSpawnRequests + ActiveSpawnRequests)
- GPU 스폰 패스 (RDG)
- ParticleID 할당 및 추적

### Lock 전략
- **FCriticalSection (SpawnLock)**: PendingSpawnRequests 보호
- **Atomic flag**: `bHasPendingSpawnRequests` (lock-free 체크)
- **Atomic counter**: `NextParticleID`

### 주요 API
```cpp
void AddSpawnRequest(const FVector3f& Position, const FVector3f& Velocity, float Mass);
void AddSpawnRequests(const TArray<FGPUSpawnRequest>& Requests);
void SwapBuffers();  // Render thread buffer swap
void AddSpawnParticlesPass(FRDGBuilder&, FRDGBufferUAVRef, FRDGBufferUAVRef);
void OnSpawnComplete(int32 SpawnedCount);
```

### 추출된 기능
- 기존 GPUFluidSimulator의 산재된 스폰 로직을 관리형 큐로 전환

---

## 2. StreamCompactionManager (599 LOC)

### 책임
- GPU AABB 필터링 (Parallel Prefix Sum - Blelloch scan)
- 스트림 컴팩션 파이프라인: Mark → Prefix Sum → Compact
- GPU→CPU 리드백 (필터링된 후보)
- Per-polygon 충돌 보정 적용
- Attachment 위치 업데이트 적용

### Lock 전략
- **None** (stateless 연산)
- 모든 연산은 Compute Shader 기반
- 버퍼 관리는 Render Thread 전용

### 주요 컴포넌트
- Marked flags buffer (AABB 내부 파티클)
- Prefix sum buffers (Blelloch scan)
- Compacted candidates buffer
- Staging buffers (CPU 리드백용)

### 파이프라인
1. **Mark Pass**: AABB 내부 파티클 플래그 설정
2. **Prefix Sum Pass**: Blelloch scan (4단계)
3. **Compact Pass**: 압축된 인덱스 생성
4. **Readback**: Async GPU→CPU 전송

### 추출된 기능
- Per-polygon 충돌 필터링 시스템 (기존 인라인 로직)

---

## 3. CollisionManager (604 LOC)

### 책임
- Bounds 충돌 (AABB/OBB + restitution/friction)
- Distance Field 충돌 (GDF volume sampling)
- Primitive 충돌 (Sphere, Capsule, Box, Convex)
- 충돌 피드백 시스템 (GPU 리드백)
- Collider contact counting

### Lock 전략
- **FCriticalSection (CollisionLock)**: 캐시된 프리미티브 보호
  - Spheres, Capsules, Boxes, ConvexHeaders/Planes
  - BoneTransforms

### 주요 패스
```cpp
void AddBoundsCollisionPass(FRDGBuilder&, FRDGBufferUAVRef, const FGPUFluidSimulationParams&);
void AddDistanceFieldCollisionPass(FRDGBuilder&, FRDGBufferUAVRef, const FGPUFluidSimulationParams&);
void AddPrimitiveCollisionPass(FRDGBuilder&, FRDGBufferUAVRef, const FGPUFluidSimulationParams&);
```

### 피드백 시스템
- 내부 **FGPUCollisionFeedbackManager** 사용
- Triple-buffered async GPU 리드백
- Per-collider contact counts
- 충돌 위치/속도/힘 데이터

### 추출된 기능
- 모든 충돌 시스템을 단일 매니저로 통합
- 기존 2개의 Lock (BufferLock, FeedbackLock) → 1개로 통합

---

## 4. SpatialHashManager (368 LOC)

### 책임
- Z-Order (Morton code) 계산 (파티클 위치 기반)
- GPU Radix Sort (4-bit, 6-pass for 21-bit codes)
- 파티클 재정렬 (정렬된 인덱스 기반)
- Cell start/end 인덱스 계산
- Cache-coherent 공간 해싱

### Lock 전략
- **None** (pure compute pipeline)

### 파이프라인
1. **Compute Morton Codes**: 7-bit per axis = 128³ grid
2. **6-Pass Radix Sort**: Histogram 기반 버킷팅
3. **Reorder Particles**: Scatter 기반 데이터 재배치
4. **Compute Cell Start/End**: Neighbor search용

### 주요 특징
- 21-bit Morton codes (해시 충돌 없음)
- Blelloch-style block-wise radix sort
- SPH용 cache-coherent 메모리 레이아웃

### 추출된 기능
- 완전한 Z-Order 정렬 시스템 (기존 인라인 구현)

---

## 5. BoundarySkinningManager (431 LOC)

### 책임
- GPU 바운더리 스키닝 (bone-local → world transform)
- Per-mesh 바운더리 파티클 버퍼 관리
- 프레임별 본 트랜스폼 업로드
- Boundary adhesion 공간 해싱 (Flex-style)
- Legacy CPU 바운더리 파티클 경로 지원

### Lock 전략
- **FCriticalSection (BoundarySkinningLock)**:
  - Boundary skinning data map 보호
  - Local/World 바운더리 파티클 버퍼 보호
  - Dirty tracking flag 보호

### 주요 데이터 구조
- Per-owner skinning data (local particles, bone transforms, component transform)
- Persistent local boundary buffers (per OwnerID)
- World boundary buffer (aggregated)
- Boundary spatial hash (65536 cells, 최대 16 particles/cell)

### 주요 패스
```cpp
void AddBoundarySkinningPass(FRDGBuilder&, FRDGBufferUAVRef, const FGPUFluidSimulationParams&);
void AddBoundaryAdhesionPass(FRDGBuilder&, FRDGBufferUAVRef, const FGPUFluidSimulationParams&);
```

### 추출된 기능
- 바운더리 파티클 시스템 전체 (기존 CPU 기반 → GPU)
- 기존 BoundarySkinningLock을 독립 매니저로 분리

---

## 6. AdhesionManager (453 LOC)

### 책임
- 파티클을 본 충돌 프리미티브에 부착
- Attachment tracking buffer (per-particle)
- 부착된 파티클 위치 업데이트 (본과 함께 이동)
- 가속도/거리 임계값으로 탈착 처리
- Stack pressure 계산 (쌓인 파티클의 무게 전달)
- 본 표면의 슬라이딩 마찰

### Lock 전략
- **FCriticalSection (AdhesionLock)**:
  - Persistent attachment buffer 접근 보호
  - Buffer size tracking 보호

### 주요 패스
```cpp
void AddAdhesionPass(FRDGBuilder&, FRDGBufferUAVRef ParticlesUAV, FRDGBufferUAVRef AttachmentUAV,
                     FGPUCollisionManager*, int32, const FGPUFluidSimulationParams&);
void AddUpdateAttachedPositionsPass(FRDGBuilder&, FRDGBufferUAVRef, FRDGBufferUAVRef,
                                    FGPUCollisionManager*, int32, const FGPUFluidSimulationParams&);
void AddClearDetachedFlagPass(FRDGBuilder&, FRDGBufferUAVRef, int32);
void AddStackPressurePass(FRDGBuilder&, FRDGBufferUAVRef, FRDGBufferSRVRef, FRDGBufferSRVRef,
                          FRDGBufferSRVRef, FGPUCollisionManager*, int32, const FGPUFluidSimulationParams&);
```

### Attachment 데이터
- Per-particle attachment info (본 인덱스, 로컬 위치)
- Detachment tracking (가속도/거리 임계값)
- Sliding friction 파라미터
- Gravity sliding scale

### 추출된 기능
- Adhesion/Attachment 시스템 전체 (기존 GPUFluidSimulator_SimPasses.cpp에서 ~290줄 제거)

---

## Lock 전략 요약

### 게임 스레드 → 렌더 스레드 동기화
```
SpawnLock          → SpawnManager (스폰 요청 큐)
CollisionLock      → CollisionManager (프리미티브 업로드)
SkinningLock       → BoundarySkinningManager (바운더리 데이터)
AdhesionLock       → AdhesionManager (Attachment 버퍼)
```

### 렌더 스레드 전용 (Lock 없음)
```
StreamCompactionManager  → Pure compute (AABB 필터링)
SpatialHashManager       → Pure compute (Z-Order 정렬)
```

### Lock 규칙
- **절대 중첩 금지**: 각 매니저는 하나의 Lock만 소유
- **Game Thread Lock**: 업로드 시 동기화 필요한 경우만
- **Render Thread Only**: 시뮬레이션 패스는 Lock 불필요
- **Atomic 우선**: Lock-free 체크 가능한 경우 atomic 사용

---

## 주요 기능별 매니저 매핑

| 기능 | 담당 매니저 | 주요 패스 |
|-----|-----------|----------|
| 파티클 생성 | SpawnManager | AddSpawnParticlesPass |
| AABB 필터링 | StreamCompactionManager | Mark → PrefixSum → Compact |
| 공간 해싱 | SpatialHashManager | MortonCode → RadixSort (6단계) |
| 바운드 충돌 | CollisionManager | AddBoundsCollisionPass |
| DF 충돌 | CollisionManager | AddDistanceFieldCollisionPass |
| 프리미티브 충돌 | CollisionManager | AddPrimitiveCollisionPass |
| 바운더리 스키닝 | BoundarySkinningManager | AddBoundarySkinningPass |
| Flex 어드히전 | BoundarySkinningManager | AddBoundaryAdhesionPass |
| 본 부착 | AdhesionManager | AddAdhesionPass |
| 부착 업데이트 | AdhesionManager | AddUpdateAttachedPositionsPass |
| Stack Pressure | AdhesionManager | AddStackPressurePass |

---

## 구현 품질 메트릭

| 메트릭 | 상태 |
|--------|------|
| 총 매니저 코드 | 2,635 LOC |
| Thread Safety | ✅ 필요한 곳에 적절히 Lock 적용 |
| GPU 패스 구성 | ✅ 잘 구조화된 RDG 패스 |
| 메모리 관리 | ✅ 올바른 리소스 생명주기 (Extract/Register) |
| Async GPU Readback | ✅ 구현됨 (StreamCompaction, CollisionFeedback) |
| 에러 핸들링 | ✅ Validation guard 존재 |
| 문서화 | ✅ 전체 헤더 문서화 완료 |

---

## GPUFluidSimulator에서 추출된 기능

1. **Spawn System** → 산재된 스폰 로직을 관리형 큐로 전환
2. **Collision Pipeline** → CollisionManager + FeedbackManager 서브시스템으로 분리
3. **Spatial Hashing** → 완전한 Z-Order 정렬 시스템 (기존 인라인)
4. **Boundary Management** → GPU 스키닝 및 어드히전 (기존 CPU 전용)
5. **Adhesion/Attachment** → 포괄적인 부착 시스템 (stack pressure 포함)
6. **Stream Compaction** → AABB 필터링 및 per-polygon 충돌 지원

---

## 아키텍처 하이라이트

- **Double/Triple Buffering**: Spawn (double), CollisionFeedback (triple)
- **Async GPU Readback**: StreamCompaction, CollisionFeedback 모두 non-blocking 리드백 사용
- **Persistent Buffers**: Boundary particles, attachments, collision primitives 프레임 간 재사용
- **RDG Integration**: 모든 패스가 Unreal의 Render Dependency Graph에 올바르게 통합
- **Thread Safety**: 게임 스레드 데이터 보호가 필요한 곳에만 Critical Section 적용

---

## 스텁 없음

모든 구현이 완전한 기능으로 완료되었습니다. 미구현 함수나 placeholder 메서드 없음.

---

## 파일 구조

```
Source/KawaiiFluidRuntime/
├── Public/GPU/
│   ├── GPUFluidSimulator.h (806 lines)
│   └── Managers/
│       ├── GPUSpawnManager.h
│       ├── GPUStreamCompactionManager.h
│       ├── GPUCollisionManager.h
│       ├── GPUCollisionFeedbackManager.h
│       ├── GPUSpatialHashManager.h
│       ├── GPUBoundarySkinningManager.h
│       └── GPUAdhesionManager.h
│
└── Private/GPU/
    ├── GPUFluidSimulator.cpp
    ├── GPUFluidSimulator_SimPasses.cpp
    └── Managers/
        ├── GPUSpawnManager.cpp (180 LOC)
        ├── GPUStreamCompactionManager.cpp (599 LOC)
        ├── GPUCollisionManager.cpp (604 LOC)
        ├── GPUCollisionFeedbackManager.cpp
        ├── GPUSpatialHashManager.cpp (368 LOC)
        ├── GPUBoundarySkinningManager.cpp (431 LOC)
        └── GPUAdhesionManager.cpp (453 LOC)
```

---

**분석 일자**: 2026-01-13
**분석자**: Claude Code
**프로젝트**: KawaiiFluid_ver1
