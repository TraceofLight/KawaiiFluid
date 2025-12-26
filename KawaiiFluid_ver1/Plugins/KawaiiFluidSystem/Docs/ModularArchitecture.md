# Modular Architecture Design

## Why Module-Based?

### Problem: Component Hell
```
// 기존 방식 - Actor에 컴포넌트가 덕지덕지
AActor
├── UKawaiiFluidSimulationComponent  (시뮬레이션)
├── UFluidColliderComponent          (충돌)
├── UFluidInteractionComponent       (상호작용)
├── UFluidRendererComponent          (렌더링)
├── UFluidEventComponent             (이벤트)
└── ... 계속 늘어남
```

### Solution: Module Composition
```
// 새 방식 - 하나의 컴포넌트, 내부 모듈 조합
AActor
└── UKawaiiFluidComponent
    ├── SimulationModule (데이터 + API)
    └── RenderModule     (렌더링) [TODO]
```

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        UKawaiiFluidComponent                        │
│                         (ActorComponent)                            │
├─────────────────────────────────────────────────────────────────────┤
│  Lifecycle: BeginPlay, EndPlay, Tick                                │
│  Subsystem Registration                                             │
│  Event Delegates (OnParticleHit)                                    │
│  Spawn Settings (Auto/Continuous)                                   │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────────────────────┐  ┌─────────────────────────────┐  │
│  │  UKawaiiFluidSimulation     │  │  UKawaiiFluidRender         │  │
│  │        Module               │  │        Module               │  │
│  │       (UObject)             │  │       (UObject)             │  │
│  ├─────────────────────────────┤  ├─────────────────────────────┤  │
│  │ ■ Data                      │  │ ■ Resources                 │  │
│  │   - Particles[]             │  │   - RenderResource          │  │
│  │   - SpatialHash             │  │   - DebugMeshComponent      │  │
│  │   - Colliders[]             │  │                             │  │
│  │   - InteractionComponents[] │  │ ■ Settings                  │  │
│  │   - ParticleLastEventTime   │  │   - RenderingMode           │  │
│  │                             │  │   - ParticleRadius          │  │
│  │ ■ Configuration             │  │   - bEnableDebugRendering   │  │
│  │   - Preset                  │  │                             │  │
│  │   - Override Values         │  │ ■ API                       │  │
│  │   - bIndependentSimulation  │  │   - UpdateRenderData()      │  │
│  │   - Event Settings          │  │   - UpdateDebugInstances()  │  │
│  │                             │  │                             │  │
│  │ ■ API (BlueprintCallable)   │  └─────────────────────────────┘  │
│  │   - SpawnParticle()         │               [TODO]              │
│  │   - ApplyExternalForce()    │                                   │
│  │   - RegisterCollider()      │                                   │
│  │   - GetParticles()          │                                   │
│  │   - GetParticlesInRadius()  │                                   │
│  │   - BuildSimulationParams() │                                   │
│  └─────────────────────────────┘                                   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   │ Register/Unregister
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                   UKawaiiFluidSimulatorSubsystem                    │
│                       (WorldSubsystem)                              │
├─────────────────────────────────────────────────────────────────────┤
│  AllModules[]                                                       │
│  GlobalColliders[]                                                  │
│  GlobalInteractionComponents[]                                      │
│  SharedSpatialHash                                                  │
│  ContextCache                                                       │
├─────────────────────────────────────────────────────────────────────┤
│  Tick()                                                             │
│    ├── SimulateIndependentFluidComponents()                         │
│    └── SimulateBatchedFluidComponents()                             │
│          ├── GroupModulesByPreset()                                 │
│          ├── MergeModuleParticles()                                 │
│          ├── Context->Simulate()                                    │
│          └── SplitModuleParticles()                                 │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   │ Simulate()
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                   UKawaiiFluidSimulationContext                     │
├─────────────────────────────────────────────────────────────────────┤
│  Solver Pipeline:                                                   │
│    1. External Forces                                               │
│    2. Predict Positions                                             │
│    3. Spatial Hash Build                                            │
│    4. Density Calculation                                           │
│    5. Pressure Solve (Jacobi)                                       │
│    6. Viscosity                                                     │
│    7. Collision Detection                                           │
│    8. Update Velocities                                             │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Data Flow

```
┌──────────┐     ┌──────────────┐     ┌───────────┐     ┌─────────┐
│  Module  │────▶│  Subsystem   │────▶│  Context  │────▶│ Solvers │
│  (Data)  │◀────│ (Orchestrate)│◀────│ (Execute) │◀────│ (Math)  │
└──────────┘     └──────────────┘     └───────────┘     └─────────┘
     │
     │ BuildSimulationParams()
     ▼
┌─────────────────────────────────┐
│   FKawaiiFluidSimulationParams  │
├─────────────────────────────────┤
│ - World                         │
│ - ExternalForce                 │
│ - Colliders[]                   │
│ - InteractionComponents[]       │
│ - bUseWorldCollision            │
│ - CollisionChannel              │
│ - Event Settings                │
│ - OnCollisionEvent callback     │
└─────────────────────────────────┘
```

---

## Module vs Component Comparison

```
┌─────────────────────────────────────────────────────────────────────┐
│                    OLD: Multiple Components                         │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   AActor                                                            │
│   ├── UFluidSimComponent      ◄── 파티클 데이터                     │
│   ├── UFluidColliderComponent ◄── 충돌 처리 (별도 컴포넌트)         │
│   ├── UFluidInteractionComp   ◄── 본 트래킹 (별도 컴포넌트)         │
│   ├── UFluidRendererComponent ◄── 렌더링 (별도 컴포넌트)            │
│   └── UFluidEventComponent    ◄── 이벤트 (별도 컴포넌트)            │
│                                                                     │
│   Problems:                                                         │
│   - 컴포넌트 간 의존성 관리 복잡                                    │
│   - 초기화 순서 문제                                                │
│   - 데이터 중복 / 동기화 필요                                       │
│   - Blueprint에서 관리 어려움                                       │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                    NEW: Single Component + Modules                  │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   AActor                                                            │
│   └── UKawaiiFluidComponent                                         │
│       ├── SimulationModule ◄── 모든 시뮬레이션 데이터 + API         │
│       │   ├── Particles, SpatialHash                                │
│       │   ├── Colliders (레퍼런스)                                  │
│       │   ├── InteractionComponents (레퍼런스)                      │
│       │   ├── Preset + Overrides                                    │
│       │   └── Event Settings                                        │
│       │                                                             │
│       └── RenderModule ◄── 모든 렌더링 데이터 + API [TODO]          │
│           ├── RenderResource                                        │
│           ├── DebugMesh                                             │
│           └── RenderingMode                                         │
│                                                                     │
│   Benefits:                                                         │
│   - 단일 컴포넌트로 관리                                            │
│   - 모듈 간 명확한 책임 분리                                        │
│   - Details 패널에서 계층적 표시                                    │
│   - Blueprint에서 모듈 직접 접근 가능                               │
│   - 모듈 단위 재사용 가능                                           │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Registration Flow

```
BeginPlay
    │
    ▼
┌─────────────────────────────────┐
│ UKawaiiFluidComponent           │
│   └── SimulationModule          │
│         │                       │
│         │ Initialize(Preset)    │
│         ▼                       │
│   ┌─────────────────────┐       │
│   │ - SpatialHash 생성  │       │
│   │ - RuntimePreset     │       │
│   └─────────────────────┘       │
│                                 │
│   RegisterToSubsystem()         │
│         │                       │
└─────────│───────────────────────┘
          │
          ▼
┌─────────────────────────────────┐
│ UKawaiiFluidSimulatorSubsystem  │
│                                 │
│   RegisterModule(Module)        │
│         │                       │
│         ▼                       │
│   AllModules.Add(Module)        │
│                                 │
└─────────────────────────────────┘
```

---

## Batching Strategy

```
Same Preset = Batched Together
──────────────────────────────

Module A (Preset: Water)  ──┐
Module B (Preset: Water)  ──┼──▶ Merge ──▶ Simulate ──▶ Split
Module C (Preset: Water)  ──┘

Module D (Preset: Blood)  ──┐
Module E (Preset: Blood)  ──┼──▶ Merge ──▶ Simulate ──▶ Split
Module F (Preset: Blood)  ──┘

Module G (Override: Yes)  ─────▶ Independent Simulate (No Batch)
```

---

## Event System Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│                          Event Flow                                 │
└─────────────────────────────────────────────────────────────────────┘

Component::Tick()
      │
      └──▶ EventCountThisFrame = 0

Subsystem::Tick()
      │
      └──▶ Context->Simulate()
                 │
                 └──▶ Collision Detected
                           │
                           └──▶ Params.OnCollisionEvent.Execute()
                                       │
                                       ▼
                           ┌───────────────────────┐
                           │ Module::              │
                           │ HandleCollisionEvent()│
                           ├───────────────────────┤
                           │ 1. MaxEvents 체크     │
                           │ 2. Cooldown 체크      │
                           │    (ParticleLastEvent │
                           │     Time 맵 사용)     │
                           │ 3. Callback 호출      │
                           └───────────────────────┘
                                       │
                                       ▼
                           ┌───────────────────────┐
                           │ Component::           │
                           │ OnParticleHit         │
                           │ .Broadcast()          │
                           └───────────────────────┘
                                       │
                                       ▼
                           ┌───────────────────────┐
                           │ Blueprint/C++         │
                           │ Bound Handler         │
                           └───────────────────────┘
```

---

## File Structure

```
KawaiiFluidRuntime/
├── Public/
│   ├── Components/
│   │   ├── KawaiiFluidComponent.h           ◄── 새 통합 컴포넌트
│   │   └── KawaiiFluidSimulationComponent.h ◄── [DEPRECATED]
│   │
│   ├── Modules/
│   │   ├── KawaiiFluidSimulationModule.h    ◄── 시뮬레이션 모듈
│   │   └── KawaiiFluidRenderModule.h        ◄── [TODO] 렌더링 모듈
│   │
│   └── Core/
│       ├── KawaiiFluidSimulatorSubsystem.h  ◄── 오케스트레이션
│       ├── KawaiiFluidSimulationContext.h   ◄── 시뮬레이션 로직
│       └── KawaiiFluidSimulationTypes.h     ◄── 공통 타입
│
└── Private/
    ├── Components/
    │   ├── KawaiiFluidComponent.cpp
    │   └── KawaiiFluidSimulationComponent.cpp
    │
    ├── Modules/
    │   ├── KawaiiFluidSimulationModule.cpp
    │   └── KawaiiFluidRenderModule.cpp      ◄── [TODO]
    │
    └── Core/
        ├── KawaiiFluidSimulatorSubsystem.cpp
        └── KawaiiFluidSimulationContext.cpp
```
