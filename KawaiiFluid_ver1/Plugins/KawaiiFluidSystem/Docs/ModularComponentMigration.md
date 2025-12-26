# Modular Component Migration Guide

## Overview

`UKawaiiFluidSimulationComponent` (Legacy) → `UKawaiiFluidComponent` + `UKawaiiFluidSimulationModule` (New)

레거시 컴포넌트의 모든 기능을 모듈 기반 아키텍처로 분리하여 재설계.

---

## Architecture Comparison

### Legacy (UKawaiiFluidSimulationComponent)
```
UKawaiiFluidSimulationComponent : UActorComponent
├── Particles[]
├── SpatialHash
├── Colliders[]
├── InteractionComponents[]
├── Preset + Override values
├── RenderResource
├── DebugMeshComponent
├── Event system
└── All APIs (SpawnParticle, ApplyForce, etc.)
```

### New (UKawaiiFluidComponent + Module)
```
UKawaiiFluidComponent : UActorComponent
├── SimulationModule (UObject, Instanced)
│   ├── Particles[]
│   ├── SpatialHash
│   ├── Colliders[]
│   ├── InteractionComponents[]
│   ├── Preset + Override values
│   ├── ParticleLastEventTime
│   └── All Simulation APIs (BlueprintCallable)
│
├── Event system (OnParticleHit, settings)
├── Spawn settings
├── World collision settings
└── RenderingModule (TODO)
```

---

## Class Responsibilities

### UKawaiiFluidComponent
- ActorComponent lifecycle (BeginPlay, EndPlay, Tick)
- Subsystem registration
- World/Owner 접근이 필요한 설정
- Event delegate (OnParticleHit) - Blueprint 바인딩
- Continuous spawn 처리
- BuildSimulationParams() - World, IgnoreActor 등 설정

### UKawaiiFluidSimulationModule (UObject)
- 파티클 데이터 소유 및 API
- SpatialHash 소유
- Collider/InteractionComponent 관리
- Preset + Override 시스템
- External force 관리
- 쿨다운 추적 (ParticleLastEventTime)

---

## API Migration

### Particle Spawning
```cpp
// Legacy
Component->SpawnParticles(Location, Count, Radius);
Component->SpawnParticle(Position, Velocity);

// New
Component->SimulationModule->SpawnParticles(Location, Count, Radius);
Component->SimulationModule->SpawnParticle(Position, Velocity);
```

### External Force
```cpp
// Legacy
Component->ApplyExternalForce(Force);
Component->ApplyForceToParticle(Index, Force);

// New
Component->SimulationModule->ApplyExternalForce(Force);
Component->SimulationModule->ApplyForceToParticle(Index, Force);
```

### Collider Management
```cpp
// Legacy
Component->RegisterCollider(Collider);
Component->UnregisterCollider(Collider);

// New
Component->SimulationModule->RegisterCollider(Collider);
Component->SimulationModule->UnregisterCollider(Collider);
```

### Query
```cpp
// Legacy
Component->GetParticles();
Component->GetParticlePositions();
Component->GetParticlesInRadius(Location, Radius);

// New
Component->SimulationModule->GetParticles();
Component->SimulationModule->GetParticlePositions();
Component->SimulationModule->GetParticlesInRadius(Location, Radius);
```

### Preset/Override
```cpp
// Legacy
Component->Preset = MyPreset;
Component->bOverride_Gravity = true;
Component->Override_Gravity = FVector(0, 0, -500);

// New
Component->SimulationModule->Preset = MyPreset;
Component->SimulationModule->bOverride_Gravity = true;
Component->SimulationModule->Override_Gravity = FVector(0, 0, -500);
```

### Events
```cpp
// Legacy
Component->OnParticleHit.AddDynamic(this, &AMyActor::OnHit);
Component->bEnableParticleHitEvents = true;

// New (동일)
Component->OnParticleHit.AddDynamic(this, &AMyActor::OnHit);
Component->bEnableParticleHitEvents = true;
```

---

## Delegate Changes

| Legacy | New |
|--------|-----|
| `FOnFluidParticleHitLegacy` | `FOnFluidParticleHitComponent` |

---

## Details Panel Structure

### Legacy
```
[Fluid Simulation]
  Preset

[Fluid Simulation|Override]
  bOverride_Gravity
  Override_Gravity
  ...

[Fluid|Events]
  OnParticleHit
  bEnableParticleHitEvents
  ...
```

### New
```
[Fluid]
  SimulationModule ▼
    [Fluid Simulation]
      Preset
    [Fluid Simulation|Override]
      bOverride_Gravity
      Override_Gravity
      ...

[Fluid|Events]
  OnParticleHit
  bEnableParticleHitEvents
  ...

[Fluid|Spawn]
  bSpawnOnBeginPlay
  AutoSpawnCount
  ...
```

---

## Subsystem Integration

### Registration
```cpp
// Legacy
Subsystem->RegisterComponent(UKawaiiFluidSimulationComponent*);
Subsystem->GetAllComponents(); // returns legacy array

// New
Subsystem->RegisterComponent(UKawaiiFluidComponent*);
Subsystem->GetAllFluidComponents(); // returns new array
```

### Simulation Flow
```cpp
// Subsystem::Tick()

// Legacy
if (AllComponents.Num() > 0)
{
    SimulateIndependentComponents(DeltaTime);
    SimulateBatchedComponents(DeltaTime);
}

// New
if (AllFluidComponents.Num() > 0)
{
    SimulateIndependentFluidComponents(DeltaTime);
    SimulateBatchedFluidComponents(DeltaTime);
}
```

---

## Batching

동일한 Preset을 사용하는 컴포넌트들은 파티클을 병합하여 시뮬레이션 후 분리.

### Batching Condition
- `IsIndependentSimulation() == false`
- Override 없음
- `IsSimulationEnabled() == true`
- `GetParticleCount() > 0`

### Flow
```
GroupFluidComponentsByPreset()
  ↓
MergeFluidParticles() → MergedFluidParticleBuffer
  ↓
Context->Simulate()
  ↓
SplitFluidParticles() → 각 컴포넌트로 복귀
```

---

## Event System Flow

```
Component::TickComponent()
  └── EventCountThisFrame = 0

Subsystem::Tick()
  └── Context->Simulate()
        └── 충돌 감지
              └── Params.OnCollisionEvent.Execute()
                    └── Component::HandleCollisionEvent()
                          ├── MaxEventsPerFrame 체크
                          ├── 쿨다운 체크 (Module->ParticleLastEventTime)
                          └── OnParticleHit.Broadcast()
```

---

## Implementation Status

| Feature | Status |
|---------|--------|
| Particle Data | ✅ |
| Collider Management | ✅ |
| External Force | ✅ |
| Preset/Override | ✅ |
| SpatialHash | ✅ |
| Subsystem Integration | ✅ |
| Batching | ✅ |
| Event System | ✅ |
| Rendering | ❌ TODO |

---

## Files

### New
- `Public/Components/KawaiiFluidComponent.h`
- `Private/Components/KawaiiFluidComponent.cpp`
- `Public/Modules/KawaiiFluidSimulationModule.h`
- `Private/Modules/KawaiiFluidSimulationModule.cpp`

### Modified
- `Public/Core/KawaiiFluidSimulatorSubsystem.h`
- `Private/Core/KawaiiFluidSimulatorSubsystem.cpp`
- `Public/Core/KawaiiFluidSimulationTypes.h`

### Legacy (유지)
- `Public/Components/KawaiiFluidSimulationComponent.h`
- `Private/Components/KawaiiFluidSimulationComponent.cpp`
