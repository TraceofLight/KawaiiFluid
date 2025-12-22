// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Test/KawaiiFluidDummy.h"
#include "Rendering/KawaiiFluidRenderResource.h"
#include "Rendering/FluidRendererSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AKawaiiFluidDummy::AKawaiiFluidDummy()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	// RootComponent 생성 (에디터에서 이동 가능)
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = RootSceneComponent;

	// 디버그 메시 컴포넌트 생성
	DebugMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DebugMeshComponent"));
	DebugMeshComponent->SetupAttachment(RootSceneComponent);
	DebugMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugMeshComponent->SetVisibility(false);  // 기본적으로 숨김

	// 기본 Sphere 메시 로드
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		DebugMeshComponent->SetStaticMesh(SphereMesh.Object);
	}

	// 기본 머티리얼 설정
	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (DefaultMaterial.Succeeded())
	{
		DebugMeshComponent->SetMaterial(0, DefaultMaterial.Object);
	}
}

AKawaiiFluidDummy::~AKawaiiFluidDummy() = default;

void AKawaiiFluidDummy::BeginPlay()
{
	Super::BeginPlay();

	InitializeRenderResource();
	InitializeDebugMesh();
	GenerateTestParticles();

	// Subsystem에 등록
	if (bEnableRendering)
	{
		if (UWorld* World = GetWorld())
		{
			if (UFluidRendererSubsystem* Subsystem = World->GetSubsystem<UFluidRendererSubsystem>())
			{
				Subsystem->RegisterRenderable(this);
				
				const TCHAR* ModeStr = (RenderingMode == EKawaiiFluidRenderingMode::SSFR) ? TEXT("SSFR") :
				                       (RenderingMode == EKawaiiFluidRenderingMode::DebugMesh) ? TEXT("DebugMesh") : TEXT("Both");
				
				UE_LOG(LogTemp, Log, TEXT("KawaiiFluidDummy registered: %s (Mode: %s)"), *GetName(), ModeStr);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("KawaiiFluidDummy: Generated %d test particles at %s"),
		TestParticles.Num(), *GetActorLocation().ToString());
}

void AKawaiiFluidDummy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Subsystem에서 등록 해제
	if (UWorld* World = GetWorld())
	{
		if (UFluidRendererSubsystem* Subsystem = World->GetSubsystem<UFluidRendererSubsystem>())
		{
			Subsystem->UnregisterRenderable(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AKawaiiFluidDummy::BeginDestroy()
{
	// 렌더 리소스 정리
	if (RenderResource.IsValid())
	{
		ENQUEUE_RENDER_COMMAND(ReleaseFluidDummyRenderResource)(
			[RenderResource = MoveTemp(RenderResource)](FRHICommandListImmediate& RHICmdList) mutable
			{
				if (RenderResource.IsValid())
				{
					RenderResource->ReleaseResource();
					RenderResource.Reset();
				}
			}
		);
	}

	Super::BeginDestroy();
}

void AKawaiiFluidDummy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bEnableRendering || TestParticles.Num() == 0)
	{
		return;
	}

	// 애니메이션 모드에서만 업데이트
	if (DataMode == ETestDataMode::Animated || DataMode == ETestDataMode::Wave)
	{
		UpdateAnimatedParticles(DeltaTime);

		// GPU 버퍼 업데이트
		if (RenderResource.IsValid())
		{
			RenderResource->UpdateParticleData(TestParticles);
		}
	}

	// 디버그 메시 업데이트 (렌더링 모드에 따라 조건부)
	if (ShouldUseDebugMesh())
	{
		UpdateDebugMeshInstances();
	}
}

void AKawaiiFluidDummy::InitializeRenderResource()
{
	RenderResource = MakeShared<FKawaiiFluidRenderResource>();

	ENQUEUE_RENDER_COMMAND(InitFluidDummyRenderResource)(
		[RenderResourcePtr = RenderResource.Get()](FRHICommandListImmediate& RHICmdList)
		{
			RenderResourcePtr->InitResource(RHICmdList);
		}
	);
}

void AKawaiiFluidDummy::InitializeDebugMesh()
{
	if (!DebugMeshComponent)
	{
		return;
	}

	// 렌더링 모드에 따라 가시성 설정
	bool bVisible = ShouldUseDebugMesh();
	DebugMeshComponent->SetVisibility(bVisible);
	
	if (bVisible)
	{
		DebugMeshComponent->ClearInstances();
		UE_LOG(LogTemp, Log, TEXT("KawaiiFluidDummy: Debug Mesh enabled"));
	}
}

void AKawaiiFluidDummy::UpdateDebugMeshInstances()
{
	if (!DebugMeshComponent || TestParticles.Num() == 0)
	{
		return;
	}

	const int32 NumParticles = TestParticles.Num();
	const int32 InstanceCount = DebugMeshComponent->GetInstanceCount();

	// 인스턴스 수 조정
	if (InstanceCount < NumParticles)
	{
		for (int32 i = InstanceCount; i < NumParticles; ++i)
		{
			DebugMeshComponent->AddInstance(FTransform::Identity);
		}
	}
	else if (InstanceCount > NumParticles)
	{
		for (int32 i = InstanceCount - 1; i >= NumParticles; --i)
		{
			DebugMeshComponent->RemoveInstance(i);
		}
	}

	// 스케일 계산 (기본 Sphere는 지름 100cm = 반지름 50cm)
	const float Scale = ParticleRadius / 50.0f;
	const FVector ScaleVec(Scale, Scale, Scale);

	// 각 파티클 위치로 인스턴스 업데이트
	for (int32 i = 0; i < NumParticles; ++i)
	{
		FTransform InstanceTransform;
		InstanceTransform.SetLocation(FVector(TestParticles[i].Position));
		InstanceTransform.SetScale3D(ScaleVec);

		DebugMeshComponent->UpdateInstanceTransform(i, InstanceTransform, true, false, false);
	}

	// 일괄 업데이트
	DebugMeshComponent->MarkRenderStateDirty();
}

void AKawaiiFluidDummy::GenerateTestParticles()
{
	switch (DataMode)
	{
	case ETestDataMode::Static:
		GenerateStaticData();
		break;

	case ETestDataMode::Animated:
		GenerateStaticData();  // 초기 위치는 Static과 동일
		break;

	case ETestDataMode::GridPattern:
		GenerateGridPattern();
		break;

	case ETestDataMode::Sphere:
		GenerateSpherePattern();
		break;

	case ETestDataMode::Wave:
		GenerateGridPattern();  // Wave는 Grid 기반으로 애니메이션
		break;
	}

	// GPU 버퍼로 전송
	if (RenderResource.IsValid())
	{
		RenderResource->UpdateParticleData(TestParticles);
	}
}

void AKawaiiFluidDummy::GenerateStaticData()
{
	TestParticles.Empty();
	TestParticles.Reserve(ParticleCount);

	const FVector ActorLocation = GetActorLocation();

	for (int32 i = 0; i < ParticleCount; ++i)
	{
		FKawaiiRenderParticle Particle;

		// 랜덤 위치 생성
		FVector RandomOffset = FVector(
			FMath::FRandRange(-SpawnExtent.X, SpawnExtent.X),
			FMath::FRandRange(-SpawnExtent.Y, SpawnExtent.Y),
			FMath::FRandRange(-SpawnExtent.Z, SpawnExtent.Z)
		);

		Particle.Position = FVector3f(ActorLocation + RandomOffset);
		Particle.Velocity = FVector3f::ZeroVector;
		Particle.Radius = ParticleRadius;
		Particle.Padding = 0.0f;

		TestParticles.Add(Particle);
	}
}

void AKawaiiFluidDummy::GenerateGridPattern()
{
	TestParticles.Empty();

	const int32 GridSize = FMath::CeilToInt(FMath::Pow(ParticleCount, 1.0f / 3.0f));
	const FVector ActorLocation = GetActorLocation();
	const float Spacing = ParticleRadius * 2.5f;

	int32 Count = 0;
	for (int32 x = 0; x < GridSize && Count < ParticleCount; ++x)
	{
		for (int32 y = 0; y < GridSize && Count < ParticleCount; ++y)
		{
			for (int32 z = 0; z < GridSize && Count < ParticleCount; ++z)
			{
				FKawaiiRenderParticle Particle;

				FVector GridPos = FVector(
					(x - GridSize / 2) * Spacing,
					(y - GridSize / 2) * Spacing,
					(z - GridSize / 2) * Spacing
				);

				Particle.Position = FVector3f(ActorLocation + GridPos);
				Particle.Velocity = FVector3f::ZeroVector;
				Particle.Radius = ParticleRadius;
				Particle.Padding = 0.0f;

				TestParticles.Add(Particle);
				Count++;
			}
		}
	}
}

void AKawaiiFluidDummy::GenerateSpherePattern()
{
	TestParticles.Empty();
	TestParticles.Reserve(ParticleCount);

	const FVector ActorLocation = GetActorLocation();
	const float SphereRadius = SpawnExtent.X;

	for (int32 i = 0; i < ParticleCount; ++i)
	{
		FKawaiiRenderParticle Particle;

		// 구 표면 균등 분포 (Fibonacci Sphere)
		float Phi = FMath::Acos(1.0f - 2.0f * (i + 0.5f) / ParticleCount);
		float Theta = PI * (1.0f + FMath::Sqrt(5.0f)) * i;

		FVector SpherePos = FVector(
			FMath::Cos(Theta) * FMath::Sin(Phi),
			FMath::Sin(Theta) * FMath::Sin(Phi),
			FMath::Cos(Phi)
		) * SphereRadius;

		Particle.Position = FVector3f(ActorLocation + SpherePos);
		Particle.Velocity = FVector3f::ZeroVector;
		Particle.Radius = ParticleRadius;
		Particle.Padding = 0.0f;

		TestParticles.Add(Particle);
	}
}

void AKawaiiFluidDummy::UpdateAnimatedParticles(float DeltaTime)
{
	AnimationTime += DeltaTime * AnimationSpeed;

	const FVector ActorLocation = GetActorLocation();

	if (DataMode == ETestDataMode::Animated)
	{
		// 회전 애니메이션
		for (int32 i = 0; i < TestParticles.Num(); ++i)
		{
			FKawaiiRenderParticle& Particle = TestParticles[i];

			// 원래 위치 (Actor 기준)
			FVector LocalPos = FVector(Particle.Position) - ActorLocation;

			// Y축 중심 회전
			FRotator Rotation(0.0f, AnimationTime * 50.0f, 0.0f);
			FVector RotatedPos = Rotation.RotateVector(LocalPos);

			Particle.Position = FVector3f(ActorLocation + RotatedPos);
		}
	}
	else if (DataMode == ETestDataMode::Wave)
	{
		// 파동 애니메이션 (Grid Pattern 기반)
		for (int32 i = 0; i < TestParticles.Num(); ++i)
		{
			FKawaiiRenderParticle& Particle = TestParticles[i];

			FVector LocalPos = FVector(Particle.Position) - ActorLocation;

			// 원래 Z 위치 저장 (초기 생성 시)
			static TArray<float> OriginalZPositions;
			if (OriginalZPositions.Num() != TestParticles.Num())
			{
				OriginalZPositions.SetNum(TestParticles.Num());
				for (int32 j = 0; j < TestParticles.Num(); ++j)
				{
					OriginalZPositions[j] = FVector(TestParticles[j].Position).Z - ActorLocation.Z;
				}
			}

			// Z축 파동
			float Wave = FMath::Sin(LocalPos.X * WaveFrequency * 0.01f + AnimationTime) * WaveAmplitude;
			LocalPos.Z = OriginalZPositions[i] + Wave;

			Particle.Position = FVector3f(ActorLocation + LocalPos);
		}
	}
}

void AKawaiiFluidDummy::RegenerateTestData()
{
	GenerateTestParticles();
	UE_LOG(LogTemp, Log, TEXT("KawaiiFluidDummy: Regenerated %d particles"), TestParticles.Num());
}

void AKawaiiFluidDummy::ForceUpdateGPUBuffer()
{
	if (RenderResource.IsValid() && TestParticles.Num() > 0)
	{
		RenderResource->UpdateParticleData(TestParticles);
		UE_LOG(LogTemp, Log, TEXT("KawaiiFluidDummy: GPU buffer updated (%d particles)"), TestParticles.Num());
	}
}

bool AKawaiiFluidDummy::IsFluidRenderResourceValid() const
{
	return RenderResource.IsValid() && RenderResource->IsValid();
}
