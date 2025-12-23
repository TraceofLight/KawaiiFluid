# KawaiiFluid 시스템 비교

## 아키텍처 개요

### 레거시 시스템 (AFluidSimulator)
- **타입**: Actor 기반 모놀리식 시스템
- **파티클 소유권**: 단일 Actor가 모든 파티클 소유
- **시뮬레이션**: 모든 로직이 한 클래스에
- **프리셋**: Enum 기반 (`EFluidType`)

### 새 시스템 (Subsystem 아키텍처)
- **타입**: Component + Subsystem 기반 SOLID 아키텍처
- **파티클 소유권**: 각 `UKawaiiFluidSimulationComponent`가 자신의 파티클 소유
- **시뮬레이션**: Stateless `UKawaiiFluidSimulationContext`가 로직 처리
- **오케스트레이션**: `UKawaiiFluidSimulatorSubsystem`이 모든 컴포넌트 관리
- **프리셋**: DataAsset 기반 (`UKawaiiFluidPresetDataAsset`)

---

## 기능 비교

| 기능 | 레거시 (AFluidSimulator) | 새 시스템 (Subsystem) | 상태 |
|------|-------------------------|----------------------|------|
| 파티클 시뮬레이션 | O | O | 지원 |
| 밀도 제약 (XPBD) | O | O | 지원 |
| 점성 (XSPH) | O | O | 지원 |
| 접착력 | O | O | 지원 |
| 공간 해시 | O | O | 지원 |
| 월드 충돌 | O | O | 지원 |
| Collider 시스템 | O | O | 지원 |
| InteractionComponent | O | O | 지원 |
| 외부 힘 적용 | O | O | 지원 |
| 파티클 스폰 | O | O | 지원 |
| 연속 스폰 | X | O | 신규 기능 |
| 디버그 메시 렌더링 | O | O | 지원 |
| RenderResource (GPU) | O | O | 지원 |
| 다중 유체 인스턴스 | 제한적 | O | 개선됨 |
| 배치 시뮬레이션 | X | O | 신규 기능 |
| 독립 시뮬레이션 | X | O | 신규 기능 |
| DataAsset 프리셋 | X | O | 신규 기능 |
| 커스텀 Context 클래스 | X | O | 신규 기능 |
| 글로벌 Collider | X | O | 신규 기능 |

---

## 아직 미지원 기능

### 1. Blueprint 충돌 이벤트 델리게이트
**레거시:**
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
    FOnFluidParticleHit,
    int32, ParticleIndex,
    FVector, HitLocation,
    FVector, HitNormal,
    AActor*, HitActor,
    UPrimitiveComponent*, HitComponent);

UPROPERTY(BlueprintAssignable)
FOnFluidParticleHit OnParticleHit;
```

**새 시스템:**
- C++ 델리게이트만 사용 (`FOnFluidCollisionEvent`)
- Blueprint에 아직 노출 안됨

**TODO:** `UKawaiiFluidSimulationComponent`에 `UPROPERTY(BlueprintAssignable)` 델리게이트 추가 필요

---

### 2. Actor별 충돌 이벤트 쿨다운
**레거시:**
```cpp
TMap<AActor*, float> LastCollisionEventTime;
float CollisionEventCooldown = 0.1f;
```

**새 시스템:**
- `FKawaiiFluidSimulationParams`에 쿨다운 시스템 존재
- 프레임당 최대 이벤트 수 제한용 atomic counter 사용
- Actor별 쿨다운은 완전히 구현 안됨

---

### 3. GetParticleInfo 쿼리 함수
**레거시:**
```cpp
UFUNCTION(BlueprintCallable)
bool GetParticleInfo(int32 ParticleIndex, FVector& OutPosition,
                     FVector& OutVelocity, float& OutDensity) const;
```

**새 시스템:**
- `GetParticles()`로 직접 파티클 배열 접근
- 편의용 Blueprint 함수 없음

---

### 4. Enum 기반 유체 타입 프리셋
**레거시:**
```cpp
UENUM(BlueprintType)
enum class EFluidType : uint8 { Water, Honey, Slime };

UFUNCTION(BlueprintCallable)
void ApplyFluidTypePreset(EFluidType NewType);
```

**새 시스템:**
- DataAsset 사용 (더 유연함)
- 빠른 프리셋 전환 함수 없음

---

## 파라미터 차이

### 기본값 비교

| 파라미터 | 레거시 기본값 | 새 시스템 기본값 (DataAsset) |
|----------|-------------|---------------------------|
| RestDensity | 1000.0 | 1200.0 |
| SmoothingRadius | 20.0 | 20.0 |
| Compliance | 0.0001 | 0.01 |
| ViscosityCoefficient | 0.01 | 0.5 |
| ParticleMass | 1.0 | 1.0 |
| Gravity | (0, 0, -980) | (0, 0, -980) |
| SubstepDeltaTime | 1/120 | 1/120 |
| MaxSubsteps | 8 | 8 |

**중요:** `Compliance` 기본값이 크게 다름 (0.0001 vs 0.01)
- 낮은 Compliance = 더 뻣뻣한 제약, 불안정할 수 있음
- 안정적인 퍼짐을 위해 0.01 권장

---

## 마이그레이션 가이드

### AFluidSimulator에서 새 시스템으로

1. **Actor를 Component로 교체:**
   ```cpp
   // 이전
   AFluidSimulator* Simulator;

   // 이후
   UKawaiiFluidSimulationComponent* FluidComponent;
   ```

2. **프리셋 DataAsset 생성:**
   - Content Browser에서 `UKawaiiFluidPresetDataAsset` 생성
   - 기존 설정과 일치하도록 파라미터 설정
   - Component의 `Preset` 속성에 할당

3. **충돌 이벤트 처리 업데이트:**
   ```cpp
   // 이전 (Blueprint)
   Simulator->OnParticleHit.AddDynamic(this, &MyClass::OnHit);

   // 이후 (현재 C++만 가능)
   FKawaiiFluidSimulationParams Params = Component->BuildSimulationParams();
   Params.OnCollisionEvent.BindUObject(this, &MyClass::OnCollision);
   ```

4. **다중 유체 소스:**
   - 이전: 여러 AFluidSimulator 액터 생성
   - 이후: 액터에 여러 UKawaiiFluidSimulationComponent 추가
   - `bIndependentSimulation = true`: 격리된 시뮬레이션
   - `bIndependentSimulation = false`: 배치 시뮬레이션 (파티클끼리 상호작용)

---

## 아키텍처 다이어그램

```
[레거시]
AFluidSimulator (Actor)
    |-- Particles[]
    |-- SpatialHash
    |-- DensityConstraint
    |-- ViscositySolver
    |-- AdhesionSolver
    |-- Colliders[]
    |-- RenderResource

[새 시스템]
UKawaiiFluidSimulatorSubsystem (World Subsystem)
    |-- AllComponents[]
    |-- GlobalColliders[]
    |-- ContextCache (프리셋별)
    |-- SharedSpatialHash (배치 시뮬용)
    |
    +-- UKawaiiFluidSimulationComponent (Actor별)
            |-- Particles[]
            |-- LocalSpatialHash (독립 시뮬용)
            |-- Preset (DataAsset 참조)
            |-- Colliders[]
            |-- RenderResource
            |
            +-- UKawaiiFluidSimulationContext (stateless, 공유)
                    |-- DensityConstraint
                    |-- ViscositySolver
                    |-- AdhesionSolver
```

---

## 권장 프리셋 값

### 슬라임 (기본)
```
RestDensity: 1200
Compliance: 0.01
SmoothingRadius: 20
ViscosityCoefficient: 0.5
AdhesionStrength: 0.5
```

### 물
```
RestDensity: 1000
Compliance: 0.001
SmoothingRadius: 20
ViscosityCoefficient: 0.01
AdhesionStrength: 0.1
```

### 꿀
```
RestDensity: 1400
Compliance: 0.1
SmoothingRadius: 25
ViscosityCoefficient: 0.8
AdhesionStrength: 0.7
```

---

## 알려진 이슈

### 파티클이 퍼지지 않는 문제
**원인:** RestDensity가 계산된 밀도보다 높으면 퍼지지 않음

**해결:**
1. RestDensity를 낮추거나 (1200 권장)
2. Compliance를 높이거나 (0.01 권장)
3. SmoothingRadius를 조정 (20 권장)

### DebugMesh가 파티클을 따라가지 않는 문제
**해결됨:** `SetAbsolute(true, true, true)` 적용으로 월드 좌표 사용
