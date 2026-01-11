# KawaiiFluid Architecture Review

> 작성일: 2026-01-11
> 버전: 1.0
> 목적: 시뮬레이션/렌더링 파이프라인 구조 분석 및 개선 방향 제시

---

## 1. 현재 아키텍처 개요

### 1.1 전체 구조

```
┌─────────────────────────────────────────────────────────────────────┐
│                         GAME THREAD                                 │
├─────────────────────────────────────────────────────────────────────┤
│  UKawaiiFluidSimulatorSubsystem                                     │
│  └── Tick() → SimulationModule.Tick()                               │
│       └── Context.SimulateSubstep()                                 │
│            └── GPU: UploadParticles → SimulateSubstep_RDG → Download│
│                                                                     │
│  UFluidRendererSubsystem                                            │
│  └── RenderingModule.UpdateRenderers()                              │
│       └── MetaballRenderer.SetParticles() → RenderResource 캐시    │
├─────────────────────────────────────────────────────────────────────┤
│                        RENDER THREAD                                │
├─────────────────────────────────────────────────────────────────────┤
│  FFluidSceneViewExtension                                           │
│  ├── PreRenderViewFamily_RenderThread()                             │
│  │    └── GPU→GPU 버퍼 추출 (ExtractRenderDataSoAPass)              │
│  ├── PostRenderBasePassDeferred_RenderThread()                      │
│  │    └── GBuffer/Translucent 렌더링                                │
│  └── PrePostProcessPass_RenderThread()                              │
│       └── Transparency/ScreenSpace/Shadow 렌더링                   │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.2 핵심 클래스 관계

```
UKawaiiFluidSimulatorSubsystem (World Subsystem)
    │
    ├── UKawaiiFluidSimulationModule[] (시뮬레이션 데이터)
    │       │
    │       └── implements IKawaiiFluidDataProvider
    │
    ├── UKawaiiFluidSimulationContext (Preset별 공유 리소스)
    │       │
    │       └── FGPUFluidSimulator (GPU 시뮬레이션)
    │
    └── TMap<FContextCacheKey, Context> (Context 캐시)

UFluidRendererSubsystem (World Subsystem)
    │
    ├── UKawaiiFluidRenderingModule[] (렌더링 관리)
    │       │
    │       ├── UKawaiiFluidISMRenderer
    │       └── UKawaiiFluidMetaballRenderer
    │               │
    │               └── FKawaiiFluidRenderResource
    │
    ├── FFluidSceneViewExtension (렌더 파이프라인 인젝션)
    │
    └── FFluidShadowHistoryManager (그림자 히스토리)
```

---

## 2. 잘된 점

### 2.1 모듈화 분리 (Module-based Architecture)

- 시뮬레이션(`SimulationModule`)과 렌더링(`RenderingModule`)을 별도 모듈로 분리
- `IKawaiiFluidDataProvider` 인터페이스로 느슨한 결합 시도
- Preset 기반 설정 분리 (Data-Driven Design)

### 2.2 Pipeline 추상화

```cpp
TSharedPtr<IKawaiiMetaballRenderingPipeline> Pipeline;
Pipeline->ExecutePostBasePass(...);
Pipeline->ExecutePrePostProcess(...);
```

- 렌더링 파이프라인을 인터페이스로 추상화
- ShadingMode별 분기를 Pipeline 내부로 캡슐화

### 2.3 Context 패턴 (Flyweight)

```cpp
UKawaiiFluidSimulationContext* GetOrCreateContext(Preset, bUseGPU);
```

- 같은 Preset을 공유하는 모듈들의 리소스 재사용
- GPU 시뮬레이터, Spatial Hash 등 무거운 리소스 공유

---

## 3. 설계 문제점

### 3.1 Single Responsibility 위반

**문제**: `UKawaiiFluidSimulationModule`이 너무 많은 책임을 가짐

| 책임 | 현재 위치 |
|------|-----------|
| 데이터 저장소 | `TArray<FFluidParticle> Particles` |
| 파티클 생성 팩토리 | `SpawnParticlesSphere()`, `SpawnParticlesBox()` 등 |
| 물리 설정 관리 | `SetOverride_*()` 메서드들 |
| 콜라이더 레지스트리 | `RegisterCollider()`, `TArray<UFluidCollider*>` |
| 이벤트 시스템 | `FOnModuleCollisionEvent` |
| GPU 시뮬레이터 참조 | `CachedGPUSimulator` |
| Containment 충돌 | `ResolveContainmentCollisions()` |

**제안**: 책임 분리

```
UKawaiiFluidSimulationModule (데이터 + 설정만)
├── FParticleSpawner (생성 로직)
├── FColliderRegistry (콜라이더 관리)
├── FFluidEventDispatcher (이벤트 처리)
└── FContainmentVolume (경계 충돌)
```

---

### 3.2 Dependency Inversion 위반

**문제**: 고수준 모듈이 저수준 구현에 직접 의존

```cpp
// KawaiiFluidSimulationModule.h:10
#include "GPU/GPUFluidSimulator.h"  // 구체 클래스 직접 포함

// KawaiiFluidSimulationModule.h:649
virtual FGPUFluidSimulator* GetGPUSimulator() const override;  // 구체 타입 반환
```

**문제점**: `IKawaiiFluidDataProvider` 인터페이스가 `FGPUFluidSimulator*`를 반환 → 추상화 누수

**제안**:

```cpp
// 추상화된 시뮬레이터 인터페이스
class IFluidSimulator {
public:
    virtual void Simulate(float DeltaTime) = 0;
    virtual TRefCountPtr<FRDGPooledBuffer> GetPositionBuffer() = 0;
    virtual int32 GetParticleCount() const = 0;
};

// DataProvider는 추상 인터페이스만 노출
class IKawaiiFluidDataProvider {
public:
    virtual IFluidSimulator* GetSimulator() = 0;
};
```

---

### 3.3 이중 서브시스템 (Cohesion 부족)

**현재 구조**:

| 서브시스템 | 역할 | 관리 대상 |
|------------|------|-----------|
| `UKawaiiFluidSimulatorSubsystem` | 시뮬레이션 | `SimulationModule[]` |
| `UFluidRendererSubsystem` | 렌더링 | `RenderingModule[]` |

**문제점**:
- 두 서브시스템이 같은 모듈 집합을 다른 방식으로 관리
- `FContextCacheKey` 동일한 키 구조를 양쪽에서 사용
- ViewExtension이 SimulatorSubsystem 헤더를 include (암묵적 의존)

**선택지**:

| 옵션 A: 통합 | 옵션 B: 명확한 분리 |
|--------------|---------------------|
| `UKawaiiFluidSubsystem` 단일화 | Mediator 패턴으로 연결 |
| 시뮬레이션 + 렌더링 통합 관리 | 이벤트 기반 통신 |
| 단순하지만 거대해질 수 있음 | 복잡하지만 확장성 좋음 |

---

### 3.4 데이터 소유권 불명확

**현재 상태**: 동일 데이터의 3개 복사본 존재

```
SimulationModule.Particles (CPU TArray)
    ↓ Upload
GPUSimulator.ParticleBufferRHI (GPU Buffer)
    ↓ Extract
RenderResource.CachedParticles (렌더용 캐시)
```

**문제**: 어느 것이 Single Source of Truth인지 불명확

```cpp
// CPU 모드에서는 이게 진짜?
TArray<FFluidParticle> Particles;

// GPU 모드에서는 이게 진짜?
TRefCountPtr<FRDGPooledBuffer> PersistentParticleBuffer;

// 렌더링에서는 이게 진짜?
TArray<FKawaiiRenderParticle> CachedParticles;
```

**제안**:

| 모드 | Source of Truth | 다른 데이터 |
|------|-----------------|-------------|
| CPU 시뮬레이션 | `SimulationModule.Particles` | 렌더링은 여기서 직접 읽음 |
| GPU 시뮬레이션 | `GPUSimulator.PersistentParticleBuffer` | CPU 배열은 Debug용만 |

---

### 3.5 Open/Closed 원칙 위반

**문제**: ShadingMode 추가 시 여러 파일 수정 필요

```cpp
// FluidSceneViewExtension.cpp
switch (Params.ShadingMode) {
case EMetaballShadingMode::GBuffer:     // 수정 필요
case EMetaballShadingMode::Translucent: // 수정 필요
case EMetaballShadingMode::PostProcess: // 수정 필요
// 새 모드 추가하려면 여기도 수정
}
```

**수정 필요 파일 목록** (새 ShadingMode 추가 시):
1. `EMetaballShadingMode` enum
2. `FluidSceneViewExtension.cpp` switch문
3. Pipeline 구현 클래스
4. 각 렌더 패스별 처리 로직

**제안**: Strategy 패턴 강화

```cpp
class IShadingStrategy {
public:
    virtual ERenderTiming GetRenderTiming() = 0;
    virtual void Execute(FRDGBuilder&, const FSceneView&, ...) = 0;
};

// ViewExtension은 타이밍만 확인
for (auto& Renderer : Renderers) {
    auto* Strategy = Renderer->GetShadingStrategy();
    if (Strategy->GetRenderTiming() == CurrentTiming) {
        Strategy->Execute(GraphBuilder, View, ...);
    }
}
```

---

### 3.6 God Object: FGPUFluidSimulator

**현재 상태**: 868줄, 100개 이상 멤버

```cpp
class FGPUFluidSimulator {
    // 시뮬레이션 실행
    void SimulateSubstep_RDG(...);

    // 데이터 전송
    void UploadParticles(...);
    void DownloadParticles(...);

    // 충돌 처리
    void UploadCollisionPrimitives(...);
    void SetDistanceFieldCollisionParams(...);

    // Adhesion 시스템
    void SetAdhesionParams(...);

    // Stream Compaction
    void ExecuteAABBFiltering(...);

    // Collision Feedback
    void GetCollisionFeedbackForCollider(...);

    // Spawn 시스템
    void AddSpawnRequest(...);

    // Anisotropy
    void SetAnisotropyParams(...);

    // ... 30개 이상의 RDG Pass 함수들
};
```

**제안**: 기능별 분리

```
FGPUFluidSimulator (오케스트레이션만)
├── FGPUParticleBuffer (버퍼 생성/관리)
├── FGPUSpatialHashBuilder (공간 해싱)
├── FGPUCollisionSystem (충돌 처리)
│   ├── PrimitiveCollision
│   ├── DistanceFieldCollision
│   └── CollisionFeedback
├── FGPUAdhesionSystem (접착)
├── FGPUSpawnSystem (파티클 생성)
└── FGPUAnisotropyCompute (비등방성 계산)
```

---

### 3.7 테스트 용이성 부족

**문제점**:
- 거의 모든 클래스가 UObject 상속 → Mocking 어려움
- GPU 의존성이 핵심 로직에 침투
- 인터페이스가 있지만 구체 타입 노출

```cpp
// 테스트하기 어려운 구조
virtual FGPUFluidSimulator* GetGPUSimulator() const override;
```

**제안**:

```cpp
// 테스트 가능한 구조
class IParticleSimulator {
public:
    virtual void Step(float DeltaTime) = 0;
    virtual int32 GetParticleCount() const = 0;
};

// CPU 구현 (테스트/에디터용)
class FCPUParticleSimulator : public IParticleSimulator { ... };

// GPU 구현 (프로덕션용)
class FGPUParticleSimulator : public IParticleSimulator { ... };
```

---

## 4. 성능 관련 구조 문제

### 4.1 데이터 변환 오버헤드

```
FFluidParticle (CPU)
    ↓ ConvertToGPU()
FGPUFluidParticle (GPU)
    ↓ ExtractRenderDataSoAPass
FVector3f[] + FKawaiiRenderParticle[] (렌더링)
```

**비용**: 매 프레임 3회 변환

### 4.2 Anisotropy 이중 계산

| 패스 | 커널 | Spatial Hash |
|------|------|--------------|
| `FluidComputeDensity.usf` | Poly6 | 사용 |
| `FluidAnisotropyCompute.usf` | Cubic Spline | 동일 Hash 재사용 |

**문제**: 같은 이웃 탐색을 두 번 수행

### 4.3 렌더링 인젝션 포인트 분산

| 메서드 | 타이밍 | 용도 |
|--------|--------|------|
| `PreRenderViewFamily_RenderThread` | 렌더링 시작 전 | GPU 버퍼 추출 |
| `PostRenderBasePassDeferred_RenderThread` | BasePass 후 | GBuffer 렌더링 |
| `PrePostProcessPass_RenderThread` | PostProcess 전 | Transparency |
| `SubscribeToPostProcessingPass` | Tonemap | (현재 미사용) |

---

## 5. 종합 평가

| 영역 | 점수 | 평가 |
|------|------|------|
| 모듈화 | ★★★☆☆ | 시도는 좋으나 경계가 불명확 |
| 의존성 관리 | ★★☆☆☆ | 구체 클래스 직접 참조 많음 |
| 확장성 | ★★★☆☆ | Pipeline 추상화 좋음, 나머지 부족 |
| 응집도 | ★★☆☆☆ | 클래스당 책임이 너무 많음 |
| 테스트 용이성 | ★☆☆☆☆ | GPU/UObject 강결합 |

---

## 6. 개선 우선순위

### Phase 1: 즉시 개선 (High Impact, Medium Effort)

1. **SimulationModule 책임 분리**
   - Spawner, ColliderRegistry, EventDispatcher 분리
   - 기존 API는 Facade로 유지 (하위 호환성)

2. **데이터 소유권 명확화**
   - GPU 모드: GPU 버퍼가 유일한 Source of Truth
   - CPU 모드: CPU 배열이 유일한 Source of Truth
   - 렌더링: Source에서 직접 읽기

### Phase 2: 중기 개선 (Medium Impact, High Effort)

3. **GPUFluidSimulator 분해**
   - Collision, Adhesion, Spawn 시스템 분리
   - 각 시스템을 독립적으로 테스트 가능하게

4. **인터페이스 추상화 수준 개선**
   - `IFluidSimulator` 인터페이스 도입
   - 구체 타입 노출 제거

### Phase 3: 장기 개선 (Low Impact, High Effort)

5. **서브시스템 관계 정리**
   - 단일 서브시스템 또는 Mediator 패턴 결정
   - 명시적 초기화 순서 정의

6. **Strategy 패턴 적용**
   - ShadingMode별 Strategy 클래스
   - 새 모드 추가 시 파일 1개만 수정

---

## 7. 참고: 현재 파일 구조

```
Plugins/KawaiiFluidSystem/Source/
├── KawaiiFluidRuntime/
│   ├── Public/
│   │   ├── Core/
│   │   │   ├── KawaiiFluidSimulatorSubsystem.h
│   │   │   ├── KawaiiFluidSimulationContext.h
│   │   │   └── FluidParticle.h
│   │   ├── Modules/
│   │   │   ├── KawaiiFluidSimulationModule.h
│   │   │   └── KawaiiFluidRenderingModule.h
│   │   ├── GPU/
│   │   │   ├── GPUFluidSimulator.h
│   │   │   └── GPUFluidParticle.h
│   │   ├── Rendering/
│   │   │   ├── FluidRendererSubsystem.h
│   │   │   ├── FluidSceneViewExtension.h
│   │   │   └── KawaiiFluidMetaballRenderer.h
│   │   └── Interfaces/
│   │       └── IKawaiiFluidDataProvider.h
│   └── Private/
│       └── ... (구현 파일들)
└── KawaiiFluidEditor/
    └── ... (에디터 전용)
```

---

## 변경 이력

| 날짜 | 버전 | 변경 내용 |
|------|------|-----------|
| 2026-01-11 | 1.0 | 초기 작성 |
