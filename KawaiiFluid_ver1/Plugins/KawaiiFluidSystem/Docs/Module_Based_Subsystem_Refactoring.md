# Subsystem 모듈 기반 리팩토링

## 개요
`UKawaiiFluidSimulatorSubsystem`이 Component 대신 Module을 직접 관리하도록 변경.

## 변경 전 구조 (문제점)
```
Subsystem
    └── RegisterComponent(UKawaiiFluidComponent*)
            └── Component->SimulationModule->GetParticles()  // 체이닝 (Law of Demeter 위반)
```

## 변경 후 구조
```
Subsystem
    └── RegisterModule(UKawaiiFluidSimulationModule*)
            └── Module->GetParticles()      // 직접 접근
            └── Module->GetWorld()          // UObject Outer 체인 활용
            └── Module->GetOwnerActor()     // 캐시됨
```

---

## 핵심 변경: UObject Outer 체인 활용

Module은 `CreateDefaultSubobject`로 생성되므로:
```cpp
Module->GetOuter() = Component
Component->GetOuter() = Actor
Actor->GetWorld() = World

// 따라서 Module에서 직접:
GetWorld();  // UObject 기본 메서드로 World 접근 가능
```

---

## 수정된 파일

### 1. KawaiiFluidSimulationModule.h
**추가된 멤버:**
```cpp
// Context Cache
UPROPERTY(Transient)
TWeakObjectPtr<AActor> CachedOwnerActor;
bool bUseWorldCollision = true;
FOnModuleCollisionEvent OnCollisionEventCallback;

// Event Settings (Component에서 이동)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid|Events")
bool bEnableCollisionEvents = false;
float MinVelocityForEvent = 50.0f;
int32 MaxEventsPerFrame = 10;
float EventCooldownPerParticle = 0.1f;
```

**추가된 메서드:**
```cpp
AActor* GetOwnerActor() const;
void SetUseWorldCollision(bool bUse);
bool GetUseWorldCollision() const;
void SetCollisionEventCallback(FOnModuleCollisionEvent InCallback);
```

### 2. KawaiiFluidSimulationModule.cpp
**Initialize()에서 Owner 캐시:**
```cpp
if (UActorComponent* OwnerComp = Cast<UActorComponent>(GetOuter()))
{
    CachedOwnerActor = OwnerComp->GetOwner();
}
```

**BuildSimulationParams() 완성:**
```cpp
// Context - Module에서 직접 접근 (Outer 체인 활용)
Params.World = GetWorld();
Params.IgnoreActor = CachedOwnerActor.Get();
Params.bUseWorldCollision = bUseWorldCollision;

// Event Settings
Params.bEnableCollisionEvents = bEnableCollisionEvents;
Params.MinVelocityForEvent = MinVelocityForEvent;
// ...
```

### 3. KawaiiFluidSimulatorSubsystem.h
**추가:**
```cpp
// Module Registration (New)
void RegisterModule(UKawaiiFluidSimulationModule* Module);
void UnregisterModule(UKawaiiFluidSimulationModule* Module);
const TArray<UKawaiiFluidSimulationModule*>& GetAllModules() const;

// Module 배열
TArray<UKawaiiFluidSimulationModule*> AllModules;

// Module 기반 배칭
TArray<FKawaiiFluidModuleBatchInfo> ModuleBatchInfos;
```

**변경된 시뮬레이션 메서드:**
```cpp
TMap<UKawaiiFluidPresetDataAsset*, TArray<UKawaiiFluidSimulationModule*>> GroupModulesByPreset() const;
void MergeModuleParticles(const TArray<UKawaiiFluidSimulationModule*>& Modules);
void SplitModuleParticles(const TArray<UKawaiiFluidSimulationModule*>& Modules);
FKawaiiFluidSimulationParams BuildMergedModuleSimulationParams(const TArray<UKawaiiFluidSimulationModule*>& Modules);
```

### 4. KawaiiFluidSimulatorSubsystem.cpp
- `Tick()`: `AllModules` 기반으로 시뮬레이션
- 시뮬레이션 메서드들이 Module을 직접 순회
- `Module->BuildSimulationParams()` 직접 호출 (Component 거치지 않음)

### 5. KawaiiFluidComponent.h
**제거된 프로퍼티 (Module로 이동):**
```cpp
// 제거됨 - Module에서 관리
bool bEnableParticleHitEvents;
float MinVelocityForEvent;
int32 MaxEventsPerFrame;
float EventCooldownPerParticle;
int32 EventCountThisFrame;
```

### 6. KawaiiFluidComponent.cpp
**BeginPlay() 변경:**
```cpp
// Module 초기화 후 설정 전달
SimulationModule->SetUseWorldCollision(bUseWorldCollision);

// 이벤트 콜백만 연결 (설정은 Module에서 직접 관리)
if (SimulationModule->bEnableCollisionEvents)
{
    SimulationModule->SetCollisionEventCallback(
        FOnModuleCollisionEvent::CreateUObject(this, &UKawaiiFluidComponent::HandleCollisionEvent)
    );
}
```

**RegisterToSubsystem() 변경:**
```cpp
// Module을 직접 등록!
Subsystem->RegisterModule(SimulationModule);
```

**BuildSimulationParams() 간소화:**
```cpp
// Module에서 모든 Params를 빌드
return SimulationModule->BuildSimulationParams();
```

**HandleCollisionEvent() 간소화:**
```cpp
// Module에서 필터링 완료 후 호출됨 - 바로 브로드캐스트
OnParticleHit.Broadcast(...);
```

### 7. KawaiiFluidSimulationTypes.h
**추가:**
```cpp
struct FKawaiiFluidModuleBatchInfo
{
    UKawaiiFluidSimulationModule* Module = nullptr;
    int32 StartIndex = 0;
    int32 ParticleCount = 0;
};
```

---

## 데이터 흐름 변경

### Before
```
Component.BeginPlay()
    → Subsystem.RegisterComponent(Component)

Subsystem.Tick()
    → for Component in AllFluidComponents
        → Params = Component.BuildSimulationParams()
            → Module.BuildSimulationParams()  // 불완전
            → Component가 World, IgnoreActor 추가
        → Module.GetParticles()
```

### After
```
Component.BeginPlay()
    → Module.Initialize()
    → Module.SetUseWorldCollision()
    → Module.SetCollisionEventCallback()
    → Subsystem.RegisterModule(Module)  // Module 직접 등록!

Subsystem.Tick()
    → for Module in AllModules
        → Params = Module.BuildSimulationParams()  // 완전한 Params
        → Module.GetParticles()
```

---

## 장점

1. **Law of Demeter 준수**: `Component->Module->X` 체이닝 제거
2. **직접 접근**: Subsystem이 Module과 직접 통신
3. **단일 책임**: Module이 시뮬레이션 데이터와 컨텍스트 모두 소유
4. **테스트 용이**: Module 단독 테스트 가능
5. **이벤트 설정 통합**: Module에서 일원화 관리
