# KawaiiFluidComponent ISM 렌더링 통합

## 개요
`UKawaiiFluidComponent`에 ISM(Instanced Static Mesh) 렌더링 기능 직접 통합.

## 변경 파일
- `KawaiiFluidComponent.h`
- `KawaiiFluidComponent.cpp`

## 추가된 기능

### 새 프로퍼티
```cpp
// ISM 렌더링 활성화
UPROPERTY(EditAnywhere, Category = "Fluid|Debug")
bool bEnableISMRendering = true;

// ISM 컴포넌트
UPROPERTY(VisibleAnywhere, Category = "Fluid|Debug")
UInstancedStaticMeshComponent* ISMComponent;
```

### 새 함수
```cpp
void InitializeISM();        // BeginPlay에서 호출
void UpdateISMInstances();   // TickComponent에서 호출
```

## 동작 방식
1. `BeginPlay`: `bEnableISMRendering=true`면 `InitializeISM()` 호출
2. `InitializeISM`:
   - ISM 컴포넌트 생성
   - `/Engine/BasicShapes/Sphere.Sphere` 메시 로드
   - 월드 좌표 사용 (`SetAbsolute(true, true, true)`)
3. `TickComponent`: 매 프레임 `UpdateISMInstances()` 호출
4. `UpdateISMInstances`:
   - `SimulationModule->GetParticles()` 에서 파티클 위치 가져옴
   - 각 파티클 위치에 ISM 인스턴스 생성
   - 스케일 = `ParticleRenderRadius / 50.0f` (기본 Sphere 반지름 50cm 기준)

## 사용법
1. Actor에 `UKawaiiFluidComponent` 추가
2. `bEnableISMRendering = true` (기본값)
3. `ParticleRenderRadius`로 파티클 크기 조절
4. Play!

## 참고
- 콜리전 없음 (`NoCollision`)
- 그림자 없음 (`SetCastShadow(false)`)
- 기본 머티리얼: `/Engine/BasicShapes/BasicShapeMaterial`
