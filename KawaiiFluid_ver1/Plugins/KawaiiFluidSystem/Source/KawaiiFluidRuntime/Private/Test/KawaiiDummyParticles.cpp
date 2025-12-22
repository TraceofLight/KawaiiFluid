// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Test/KawaiiDummyParticles.h"
#include "Rendering/KawaiiFluidRenderResource.h"
#include "Rendering/FluidRendererSubsystem.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

AKawaiiDummyParticles::AKawaiiDummyParticles()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	// ✅ RootComponent 생성 (에디터에서 이동 가능)
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = RootSceneComponent;
}

AKawaiiDummyParticles::~AKawaiiDummyParticles() = default;

void AKawaiiDummyParticles::BeginPlay()
{
	Super::BeginPlay();

	InitializeRenderResource();
	GenerateDummyParticles();

	// ✅ Subsystem에 등록
	if (bEnableRendering)
	{
		if (UWorld* World = GetWorld())
		{
			if (UFluidRendererSubsystem* Subsystem = World->GetSubsystem<UFluidRendererSubsystem>())
			{
				Subsystem->RegisterRenderable(this);
				UE_LOG(LogTemp, Log, TEXT("KawaiiDummyParticles: Registered to Subsystem"));
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("KawaiiDummyParticles: Generated %d dummy particles at %s"),
		DummyParticles.Num(), *GetActorLocation().ToString());
}

void AKawaiiDummyParticles::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ✅ Subsystem에서 등록 해제
	if (UWorld* World = GetWorld())
	{
		if (UFluidRendererSubsystem* Subsystem = World->GetSubsystem<UFluidRendererSubsystem>())
		{
			Subsystem->UnregisterRenderable(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AKawaiiDummyParticles::BeginDestroy()
{
	// 렌더 리소스 정리
	if (RenderResource.IsValid())
	{
		ENQUEUE_RENDER_COMMAND(ReleaseDummyParticlesRenderResource)(
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

void AKawaiiDummyParticles::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bEnableRendering || DummyParticles.Num() == 0)
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
			RenderResource->UpdateParticleData(DummyParticles);
		}
	}
}

void AKawaiiDummyParticles::InitializeRenderResource()
{
	RenderResource = MakeShared<FKawaiiFluidRenderResource>();

	ENQUEUE_RENDER_COMMAND(InitDummyParticlesRenderResource)(
		[RenderResourcePtr = RenderResource.Get()](FRHICommandListImmediate& RHICmdList)
		{
			RenderResourcePtr->InitResource(RHICmdList);
		}
	);
}

void AKawaiiDummyParticles::GenerateDummyParticles()
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
		RenderResource->UpdateParticleData(DummyParticles);
	}
}

void AKawaiiDummyParticles::GenerateStaticData()
{
	DummyParticles.Empty();
	DummyParticles.Reserve(ParticleCount);

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

		DummyParticles.Add(Particle);
	}
}

void AKawaiiDummyParticles::GenerateGridPattern()
{
	DummyParticles.Empty();

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

				DummyParticles.Add(Particle);
				Count++;
			}
		}
	}
}

void AKawaiiDummyParticles::GenerateSpherePattern()
{
	DummyParticles.Empty();
	DummyParticles.Reserve(ParticleCount);

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

		DummyParticles.Add(Particle);
	}
}

void AKawaiiDummyParticles::UpdateAnimatedParticles(float DeltaTime)
{
	AnimationTime += DeltaTime * AnimationSpeed;

	const FVector ActorLocation = GetActorLocation();

	if (DataMode == ETestDataMode::Animated)
	{
		// 회전 애니메이션
		for (int32 i = 0; i < DummyParticles.Num(); ++i)
		{
			FKawaiiRenderParticle& Particle = DummyParticles[i];

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
		for (int32 i = 0; i < DummyParticles.Num(); ++i)
		{
			FKawaiiRenderParticle& Particle = DummyParticles[i];

			FVector LocalPos = FVector(Particle.Position) - ActorLocation;

			// 원래 Z 위치 저장 (초기 생성 시)
			static TArray<float> OriginalZPositions;
			if (OriginalZPositions.Num() != DummyParticles.Num())
			{
				OriginalZPositions.SetNum(DummyParticles.Num());
				for (int32 j = 0; j < DummyParticles.Num(); ++j)
				{
					OriginalZPositions[j] = FVector(DummyParticles[j].Position).Z - ActorLocation.Z;
				}
			}

			// Z축 파동
			float Wave = FMath::Sin(LocalPos.X * WaveFrequency * 0.01f + AnimationTime) * WaveAmplitude;
			LocalPos.Z = OriginalZPositions[i] + Wave;

			Particle.Position = FVector3f(ActorLocation + LocalPos);
		}
	}
}

void AKawaiiDummyParticles::RegenerateDummyData()
{
	GenerateDummyParticles();
	UE_LOG(LogTemp, Log, TEXT("KawaiiDummyParticles: Regenerated %d particles"), DummyParticles.Num());
}

void AKawaiiDummyParticles::ForceUpdateGPUBuffer()
{
	if (RenderResource.IsValid() && DummyParticles.Num() > 0)
	{
		RenderResource->UpdateParticleData(DummyParticles);
		UE_LOG(LogTemp, Log, TEXT("KawaiiDummyParticles: GPU buffer updated (%d particles)"), DummyParticles.Num());
	}
}

bool AKawaiiDummyParticles::IsFluidRenderResourceValid() const
{
	return RenderResource.IsValid() && RenderResource->IsValid();
}
