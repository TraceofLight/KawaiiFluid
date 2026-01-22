// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Components/KawaiiFluidEmitterComponent.h"
#include "Actors/KawaiiFluidEmitter.h"
#include "Actors/KawaiiFluidVolume.h"
#include "Core/KawaiiFluidSimulatorSubsystem.h"
#include "Modules/KawaiiFluidSimulationModule.h"
#include "Data/KawaiiFluidPresetDataAsset.h"
#include "GPU/GPUFluidParticle.h"
#include "DrawDebugHelpers.h"

UKawaiiFluidEmitterComponent::UKawaiiFluidEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bTickInEditor = true;  // Enable tick in editor for wireframe visualization
}

void UKawaiiFluidEmitterComponent::BeginPlay()
{
	Super::BeginPlay();

	// Auto-find volume if not set
	if (!TargetVolume && bAutoFindVolume)
	{
		TargetVolume = FindNearestVolume();
	}

	RegisterToVolume();

	UE_LOG(LogTemp, Log, TEXT("UKawaiiFluidEmitterComponent [%s]: BeginPlay - TargetVolume=%s"),
		*GetName(), TargetVolume ? *TargetVolume->GetName() : TEXT("None"));

	// Auto spawn for Shape mode
	if (IsShapeMode() && bAutoSpawnOnBeginPlay && !bAutoSpawnExecuted)
	{
		SpawnShape();
	}
}

void UKawaiiFluidEmitterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterFromVolume();
	Super::EndPlay(EndPlayReason);
}

void UKawaiiFluidEmitterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const bool bIsGameWorld = World->IsGameWorld();

	// Wireframe visualization (editor only, or runtime if explicitly enabled)
#if WITH_EDITOR
	if (bShowSpawnVolumeWireframe && !bIsGameWorld)
	{
		DrawSpawnVolumeVisualization();
	}
#endif

	// Process continuous spawning for Stream mode (game world only)
	if (bIsGameWorld && IsStreamMode())
	{
		ProcessContinuousSpawn(DeltaTime);
	}
}

AKawaiiFluidEmitter* UKawaiiFluidEmitterComponent::GetOwnerEmitter() const
{
	return Cast<AKawaiiFluidEmitter>(GetOwner());
}

void UKawaiiFluidEmitterComponent::SetTargetVolume(AKawaiiFluidVolume* NewVolume)
{
	if (TargetVolume != NewVolume)
	{
		UnregisterFromVolume();
		TargetVolume = NewVolume;
		RegisterToVolume();
	}
}

float UKawaiiFluidEmitterComponent::GetParticleSpacing() const
{
	if (AKawaiiFluidVolume* Volume = GetTargetVolume())
	{
		return Volume->GetParticleSpacing();
	}
	return 10.0f; // Default fallback
}

void UKawaiiFluidEmitterComponent::SpawnShape()
{
	if (bAutoSpawnExecuted)
	{
		return;
	}

	AKawaiiFluidVolume* Volume = GetTargetVolume();
	if (!Volume)
	{
		UE_LOG(LogTemp, Warning, TEXT("UKawaiiFluidEmitterComponent::SpawnShape - No target Volume available"));
		return;
	}

	bAutoSpawnExecuted = true;

	const FVector SpawnCenter = GetComponentLocation() + SpawnOffset;
	const float Spacing = GetParticleSpacing();

	int32 SpawnedCount = 0;

	// Always use hexagonal pattern (only mode supported)
	switch (ShapeType)
	{
	case EKawaiiFluidEmitterShapeType::Sphere:
		SpawnedCount = SpawnParticlesSphereHexagonal(SpawnCenter, SphereRadius, Spacing, InitialVelocity);
		break;

	case EKawaiiFluidEmitterShapeType::Box:
		SpawnedCount = SpawnParticlesBoxHexagonal(SpawnCenter, BoxExtent, Spacing, InitialVelocity);
		break;

	case EKawaiiFluidEmitterShapeType::Cylinder:
		SpawnedCount = SpawnParticlesCylinderHexagonal(SpawnCenter, CylinderRadius,
			CylinderHalfHeight, Spacing, InitialVelocity);
		break;
	}

	UE_LOG(LogTemp, Log, TEXT("UKawaiiFluidEmitterComponent::SpawnShape - Spawned %d particles"), SpawnedCount);
}

void UKawaiiFluidEmitterComponent::BurstSpawn(int32 Count)
{
	if (Count <= 0 || HasReachedParticleLimit())
	{
		return;
	}

	// Clamp to remaining budget
	if (MaxParticleCount > 0)
	{
		Count = FMath::Min(Count, MaxParticleCount - SpawnedParticleCount);
	}

	// Get effective spacing
	float EffectiveSpacing = StreamParticleSpacing;
	if (EffectiveSpacing <= 0.0f)
	{
		if (AKawaiiFluidVolume* Vol = GetTargetVolume())
		{
			if (UKawaiiFluidPresetDataAsset* Pst = Vol->GetPreset())
			{
				EffectiveSpacing = Pst->SmoothingRadius * 0.5f;
			}
		}
	}
	if (EffectiveSpacing <= 0.0f)
	{
		EffectiveSpacing = 10.0f;
	}

	const FVector SpawnPos = GetComponentLocation() + SpawnOffset;
	const FVector WorldDirection = GetComponentRotation().RotateVector(SpawnDirection.GetSafeNormal());

	// Spawn layers instead of random particles
	for (int32 i = 0; i < Count; ++i)
	{
		SpawnStreamLayer(SpawnPos, WorldDirection, SpawnSpeed, StreamRadius, EffectiveSpacing);
	}
}

bool UKawaiiFluidEmitterComponent::HasReachedParticleLimit() const
{
	if (MaxParticleCount <= 0)
	{
		return false;
	}
	return SpawnedParticleCount >= MaxParticleCount;
}

void UKawaiiFluidEmitterComponent::ProcessContinuousSpawn(float DeltaTime)
{
	if (HasReachedParticleLimit() && !bRecycleOldestParticles)
	{
		return;
	}

	// Only Stream mode is supported
	ProcessStreamEmitter(DeltaTime);
}

void UKawaiiFluidEmitterComponent::ProcessStreamEmitter(float DeltaTime)
{
	// Calculate Spacing exactly like KawaiiFluidComponent does:
	// Use StreamParticleSpacing if set, otherwise Preset->SmoothingRadius * 0.5f
	float EffectiveSpacing = StreamParticleSpacing;
	if (EffectiveSpacing <= 0.0f)
	{
		if (AKawaiiFluidVolume* Vol = GetTargetVolume())
		{
			if (UKawaiiFluidPresetDataAsset* Pst = Vol->GetPreset())
			{
				EffectiveSpacing = Pst->SmoothingRadius * 0.5f;
			}
		}
	}
	if (EffectiveSpacing <= 0.0f)
	{
		EffectiveSpacing = 10.0f;  // fallback
	}
	const float LayerSpacing = EffectiveSpacing * StreamLayerSpacingRatio;

	float LayersToSpawn = 0.0f;

	// Velocity-based layer spawning (only mode supported)
	const float DistanceThisFrame = SpawnSpeed * DeltaTime;
	LayerDistanceAccumulator += DistanceThisFrame;

	if (LayerDistanceAccumulator >= LayerSpacing)
	{
		LayersToSpawn = FMath::FloorToFloat(LayerDistanceAccumulator / LayerSpacing);
		LayerDistanceAccumulator = FMath::Fmod(LayerDistanceAccumulator, LayerSpacing);
	}

	const int32 LayerCount = FMath::FloorToInt(LayersToSpawn);
	if (LayerCount <= 0)
	{
		return;
	}

	const FVector SpawnPos = GetComponentLocation() + SpawnOffset;
	const FVector WorldDirection = GetComponentRotation().RotateVector(SpawnDirection.GetSafeNormal());

	for (int32 i = 0; i < LayerCount; ++i)
	{
		// Estimate particles per layer for recycling
		const int32 EstimatedParticlesPerLayer = FMath::Max(1, FMath::CeilToInt(
			PI * FMath::Square(StreamRadius / EffectiveSpacing)));
		RecycleOldestParticlesIfNeeded(EstimatedParticlesPerLayer);

		SpawnStreamLayer(SpawnPos, WorldDirection, SpawnSpeed,
			StreamRadius, EffectiveSpacing);
	}
}

int32 UKawaiiFluidEmitterComponent::SpawnParticlesSphereHexagonal(FVector Center, float Radius, float Spacing, FVector InInitialVelocity)
{
	AKawaiiFluidVolume* Volume = GetTargetVolume();
	if (!Volume || Spacing <= 0.0f || Radius <= 0.0f) return 0;

	TArray<FVector> Positions;
	TArray<FVector> Velocities;

	// HCP density compensation (matches KawaiiFluidSimulationModule exactly)
	const float HCPCompensation = 1.122f;
	const float AdjustedSpacing = Spacing * HCPCompensation;

	// Hexagonal close packing offsets
	const float RowSpacingY = AdjustedSpacing * 0.866025f;   // sqrt(3)/2
	const float LayerSpacingZ = AdjustedSpacing * 0.816497f; // sqrt(2/3)
	const float RadiusSq = Radius * Radius;
	const float JitterRange = AdjustedSpacing * JitterAmount;

	// Integer-based grid (matches SimulationModule)
	const int32 GridSize = FMath::CeilToInt(Radius / AdjustedSpacing) + 1;
	const int32 GridSizeY = FMath::CeilToInt(Radius / RowSpacingY) + 1;
	const int32 GridSizeZ = FMath::CeilToInt(Radius / LayerSpacingZ) + 1;

	const float EstimatedCount = (4.0f / 3.0f) * PI * Radius * Radius * Radius / (AdjustedSpacing * AdjustedSpacing * AdjustedSpacing);
	Positions.Reserve(FMath::CeilToInt(EstimatedCount));
	Velocities.Reserve(FMath::CeilToInt(EstimatedCount));

	for (int32 z = -GridSizeZ; z <= GridSizeZ; ++z)
	{
		// Z-layer offset pattern (mod 3) - ABCABC stacking for proper HCP
		const int32 ZMod = ((z + GridSizeZ) % 3);
		const float ZLayerOffsetX = (ZMod == 1) ? AdjustedSpacing * 0.5f : ((ZMod == 2) ? AdjustedSpacing * 0.25f : 0.0f);
		const float ZLayerOffsetY = (ZMod == 1) ? RowSpacingY / 3.0f : ((ZMod == 2) ? RowSpacingY * 2.0f / 3.0f : 0.0f);

		for (int32 y = -GridSizeY; y <= GridSizeY; ++y)
		{
			const float RowOffsetX = (((y + GridSizeY) % 2) == 1) ? AdjustedSpacing * 0.5f : 0.0f;

			for (int32 x = -GridSize; x <= GridSize; ++x)
			{
				FVector LocalPos(
					x * AdjustedSpacing + RowOffsetX + ZLayerOffsetX,
					y * RowSpacingY + ZLayerOffsetY,
					z * LayerSpacingZ
				);

				// Check sphere bounds
				if (LocalPos.SizeSquared() > RadiusSq)
				{
					continue;
				}

				FVector WorldPos = Center + LocalPos;

				// Apply jitter
				if (bUseJitter && JitterRange > 0.0f)
				{
					WorldPos += FVector(
						FMath::FRandRange(-JitterRange, JitterRange),
						FMath::FRandRange(-JitterRange, JitterRange),
						FMath::FRandRange(-JitterRange, JitterRange)
					);
				}

				Positions.Add(WorldPos);
				Velocities.Add(InInitialVelocity);
			}
		}
	}

	QueueSpawnRequest(Positions, Velocities);
	return Positions.Num();
}

int32 UKawaiiFluidEmitterComponent::SpawnParticlesBoxHexagonal(FVector Center, FVector Extent, float Spacing, FVector InInitialVelocity)
{
	AKawaiiFluidVolume* Volume = GetTargetVolume();
	if (!Volume || Spacing <= 0.0f) return 0;

	TArray<FVector> Positions;
	TArray<FVector> Velocities;

	// HCP density compensation (matches KawaiiFluidSimulationModule exactly)
	const float HCPCompensation = 1.122f;
	const float AdjustedSpacing = Spacing * HCPCompensation;

	// Hexagonal Close Packing constants
	const float RowSpacingY = AdjustedSpacing * 0.866025f;   // sqrt(3)/2
	const float LayerSpacingZ = AdjustedSpacing * 0.816497f; // sqrt(2/3)
	const float JitterRange = AdjustedSpacing * JitterAmount;

	// Calculate grid counts (matches SimulationModule)
	const int32 CountX = FMath::Max(1, FMath::CeilToInt(Extent.X * 2.0f / AdjustedSpacing));
	const int32 CountY = FMath::Max(1, FMath::CeilToInt(Extent.Y * 2.0f / RowSpacingY));
	const int32 CountZ = FMath::Max(1, FMath::CeilToInt(Extent.Z * 2.0f / LayerSpacingZ));

	const int32 EstimatedTotal = CountX * CountY * CountZ;
	Positions.Reserve(EstimatedTotal);
	Velocities.Reserve(EstimatedTotal);

	// Start position (bottom-left-back corner with half-spacing offset)
	const FVector LocalStart(-Extent.X + AdjustedSpacing * 0.5f, -Extent.Y + RowSpacingY * 0.5f, -Extent.Z + LayerSpacingZ * 0.5f);

	for (int32 z = 0; z < CountZ; ++z)
	{
		// Z layer offset for HCP (ABC stacking pattern - mod 3)
		const float ZLayerOffsetX = (z % 3 == 1) ? AdjustedSpacing * 0.5f : ((z % 3 == 2) ? AdjustedSpacing * 0.25f : 0.0f);
		const float ZLayerOffsetY = (z % 3 == 1) ? RowSpacingY / 3.0f : ((z % 3 == 2) ? RowSpacingY * 2.0f / 3.0f : 0.0f);

		for (int32 y = 0; y < CountY; ++y)
		{
			// Row offset for hexagonal pattern in XY plane
			const float RowOffsetX = (y % 2 == 1) ? AdjustedSpacing * 0.5f : 0.0f;

			for (int32 x = 0; x < CountX; ++x)
			{
				FVector LocalPos(
					LocalStart.X + x * AdjustedSpacing + RowOffsetX + ZLayerOffsetX,
					LocalStart.Y + y * RowSpacingY + ZLayerOffsetY,
					LocalStart.Z + z * LayerSpacingZ
				);

				// Check bounds
				if (FMath::Abs(LocalPos.X) > Extent.X ||
				    FMath::Abs(LocalPos.Y) > Extent.Y ||
				    FMath::Abs(LocalPos.Z) > Extent.Z)
				{
					continue;
				}

				FVector WorldPos = Center + LocalPos;

				// Apply jitter
				if (bUseJitter && JitterRange > 0.0f)
				{
					WorldPos += FVector(
						FMath::FRandRange(-JitterRange, JitterRange),
						FMath::FRandRange(-JitterRange, JitterRange),
						FMath::FRandRange(-JitterRange, JitterRange)
					);
				}

				Positions.Add(WorldPos);
				Velocities.Add(InInitialVelocity);
			}
		}
	}

	QueueSpawnRequest(Positions, Velocities);
	return Positions.Num();
}

int32 UKawaiiFluidEmitterComponent::SpawnParticlesCylinderHexagonal(FVector Center, float Radius, float HalfHeight, float Spacing, FVector InInitialVelocity)
{
	AKawaiiFluidVolume* Volume = GetTargetVolume();
	if (!Volume || Spacing <= 0.0f || Radius <= 0.0f || HalfHeight <= 0.0f) return 0;

	TArray<FVector> Positions;
	TArray<FVector> Velocities;

	// HCP density compensation (matches KawaiiFluidSimulationModule exactly)
	const float HCPCompensation = 1.122f;
	const float AdjustedSpacing = Spacing * HCPCompensation;

	// Hexagonal Close Packing constants
	const float RowSpacingY = AdjustedSpacing * 0.866025f;   // sqrt(3)/2
	const float LayerSpacingZ = AdjustedSpacing * 0.816497f; // sqrt(2/3)
	const float JitterRange = AdjustedSpacing * JitterAmount;
	const float RadiusSq = Radius * Radius;

	// Integer-based grid (matches SimulationModule)
	const int32 GridSizeXY = FMath::CeilToInt(Radius / AdjustedSpacing) + 1;
	const int32 GridSizeY = FMath::CeilToInt(Radius / RowSpacingY) + 1;
	const int32 GridSizeZ = FMath::CeilToInt(HalfHeight / LayerSpacingZ);

	const float EstimatedCount = PI * Radius * Radius * HalfHeight * 2.0f / (AdjustedSpacing * AdjustedSpacing * AdjustedSpacing);
	Positions.Reserve(FMath::CeilToInt(EstimatedCount));
	Velocities.Reserve(FMath::CeilToInt(EstimatedCount));

	for (int32 z = -GridSizeZ; z <= GridSizeZ; ++z)
	{
		// Z layer offset for HCP (ABC stacking pattern - mod 3)
		const int32 ZMod = ((z + GridSizeZ) % 3);
		const float ZLayerOffsetX = (ZMod == 1) ? AdjustedSpacing * 0.5f : ((ZMod == 2) ? AdjustedSpacing * 0.25f : 0.0f);
		const float ZLayerOffsetY = (ZMod == 1) ? RowSpacingY / 3.0f : ((ZMod == 2) ? RowSpacingY * 2.0f / 3.0f : 0.0f);

		for (int32 y = -GridSizeY; y <= GridSizeY; ++y)
		{
			const float RowOffsetX = (((y + GridSizeY) % 2) == 1) ? AdjustedSpacing * 0.5f : 0.0f;

			for (int32 x = -GridSizeXY; x <= GridSizeXY; ++x)
			{
				FVector LocalPos(
					x * AdjustedSpacing + RowOffsetX + ZLayerOffsetX,
					y * RowSpacingY + ZLayerOffsetY,
					z * LayerSpacingZ
				);

				// Check cylinder bounds (XY plane for radius, Z for height)
				const float XYDistSq = LocalPos.X * LocalPos.X + LocalPos.Y * LocalPos.Y;
				if (XYDistSq > RadiusSq || FMath::Abs(LocalPos.Z) > HalfHeight)
				{
					continue;
				}

				FVector WorldPos = Center + LocalPos;

				// Apply jitter
				if (bUseJitter && JitterRange > 0.0f)
				{
					WorldPos += FVector(
						FMath::FRandRange(-JitterRange, JitterRange),
						FMath::FRandRange(-JitterRange, JitterRange),
						FMath::FRandRange(-JitterRange, JitterRange)
					);
				}

				Positions.Add(WorldPos);
				Velocities.Add(InInitialVelocity);
			}
		}
	}

	QueueSpawnRequest(Positions, Velocities);
	return Positions.Num();
}

void UKawaiiFluidEmitterComponent::SpawnStreamLayer(FVector Position, FVector Direction, float Speed, float Radius, float Spacing)
{
	AKawaiiFluidVolume* Volume = GetTargetVolume();
	if (!Volume || Spacing <= 0.0f || Radius <= 0.0f) return;

	TArray<FVector> Positions;
	TArray<FVector> Velocities;

	// Normalize direction (matches SimulationModule::SpawnParticleDirectionalHexLayerBatch)
	FVector Dir = Direction.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		Dir = FVector(0, 0, -1);  // Default: downward
	}

	// Create local coordinate system (same method as SimulationModule)
	FVector Right, Up;
	Dir.FindBestAxisVectors(Right, Up);

	// NO HCP compensation for 2D hexagonal layer!
	// This matches SimulationModule::SpawnParticleDirectionalHexLayerBatch exactly
	const float RowSpacing = Spacing * FMath::Sqrt(3.0f) * 0.5f;  // ~0.866 * Spacing
	const float RadiusSq = Radius * Radius;

	// Jitter setup (matches SimulationModule)
	const float Jitter = FMath::Clamp(StreamJitter, 0.0f, 0.5f);
	const float MaxJitterOffset = Spacing * Jitter;
	const bool bApplyJitter = Jitter > KINDA_SMALL_NUMBER;

	const FVector SpawnVel = Dir * Speed;

	// Row count calculation (matches SimulationModule)
	const int32 NumRows = FMath::CeilToInt(Radius / RowSpacing) * 2 + 1;
	const int32 HalfRows = NumRows / 2;

	// Estimate and reserve
	const int32 EstimatedCount = FMath::CeilToInt((PI * RadiusSq) / (Spacing * Spacing));
	Positions.Reserve(EstimatedCount);
	Velocities.Reserve(EstimatedCount);

	// Hexagonal grid iteration (matches SimulationModule exactly)
	for (int32 RowIdx = -HalfRows; RowIdx <= HalfRows; ++RowIdx)
	{
		const float LocalY = RowIdx * RowSpacing;
		const float LocalYSq = LocalY * LocalY;

		// Skip rows outside the circle
		if (LocalYSq > RadiusSq)
		{
			continue;
		}

		const float MaxX = FMath::Sqrt(RadiusSq - LocalYSq);

		// Odd rows get X offset (Hexagonal Packing) - uses Abs like SimulationModule
		const float XOffset = (FMath::Abs(RowIdx) % 2 != 0) ? Spacing * 0.5f : 0.0f;

		// Calculate column count
		const int32 NumCols = FMath::FloorToInt(MaxX / Spacing);

		for (int32 ColIdx = -NumCols; ColIdx <= NumCols; ++ColIdx)
		{
			float LocalX = ColIdx * Spacing + XOffset;
			float LocalYFinal = LocalY;

			// Apply jitter
			if (bApplyJitter)
			{
				LocalX += FMath::FRandRange(-MaxJitterOffset, MaxJitterOffset);
				LocalYFinal += FMath::FRandRange(-MaxJitterOffset, MaxJitterOffset);
			}

			// Check inside circle (after jitter)
			if (LocalX * LocalX + LocalYFinal * LocalYFinal <= RadiusSq)
			{
				FVector SpawnPos = Position + Right * LocalX + Up * LocalYFinal;
				Positions.Add(SpawnPos);
				Velocities.Add(SpawnVel);
			}
		}
	}

	QueueSpawnRequest(Positions, Velocities);
}

void UKawaiiFluidEmitterComponent::QueueSpawnRequest(const TArray<FVector>& Positions, const TArray<FVector>& Velocities)
{
	AKawaiiFluidVolume* Volume = GetTargetVolume();
	if (!Volume || Positions.Num() == 0)
	{
		return;
	}

	// Get SourceID from owner emitter
	int32 SourceID = -1;
	if (AKawaiiFluidEmitter* Emitter = GetOwnerEmitter())
	{
		SourceID = Emitter->GetUniqueID();
	}

	// Queue spawn requests to Volume's batch queue
	Volume->QueueSpawnRequests(Positions, Velocities, SourceID);

	SpawnedParticleCount += Positions.Num();
}

UKawaiiFluidSimulationModule* UKawaiiFluidEmitterComponent::GetSimulationModule() const
{
	if (AKawaiiFluidVolume* Volume = GetTargetVolume())
	{
		return Volume->GetSimulationModule();
	}
	return nullptr;
}

void UKawaiiFluidEmitterComponent::RecycleOldestParticlesIfNeeded(int32 NewParticleCount)
{
	if (!bRecycleOldestParticles || MaxParticleCount <= 0)
	{
		return;
	}

	UKawaiiFluidSimulationModule* Module = GetSimulationModule();
	if (!Module)
	{
		return;
	}

	const int32 CurrentCount = Module->GetParticleCount();
	const int32 ExcessCount = (CurrentCount + NewParticleCount) - MaxParticleCount;

	if (ExcessCount > 0)
	{
		Module->RemoveOldestParticles(ExcessCount);
	}
}

//========================================
// Volume Registration
//========================================

void UKawaiiFluidEmitterComponent::RegisterToVolume()
{
	if (TargetVolume)
	{
		if (AKawaiiFluidEmitter* Emitter = GetOwnerEmitter())
		{
			TargetVolume->RegisterEmitter(Emitter);
		}
	}
}

void UKawaiiFluidEmitterComponent::UnregisterFromVolume()
{
	if (TargetVolume)
	{
		if (AKawaiiFluidEmitter* Emitter = GetOwnerEmitter())
		{
			TargetVolume->UnregisterEmitter(Emitter);
		}
	}
}

AKawaiiFluidVolume* UKawaiiFluidEmitterComponent::FindNearestVolume() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Get subsystem to find registered volumes
	UKawaiiFluidSimulatorSubsystem* Subsystem = World->GetSubsystem<UKawaiiFluidSimulatorSubsystem>();
	if (!Subsystem)
	{
		return nullptr;
	}

	const FVector EmitterLocation = GetComponentLocation();
	AKawaiiFluidVolume* NearestVolume = nullptr;
	float NearestDistSq = FLT_MAX;

	for (const TObjectPtr<AKawaiiFluidVolume>& Volume : Subsystem->GetAllVolumes())
	{
		if (Volume)
		{
			const float DistSq = FVector::DistSquared(EmitterLocation, Volume->GetActorLocation());
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				NearestVolume = Volume;
			}
		}
	}

	return NearestVolume;
}

#if WITH_EDITOR
void UKawaiiFluidEmitterComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ?
		PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UKawaiiFluidEmitterComponent, TargetVolume))
	{
		// Re-register when target volume changes
		if (HasBegunPlay())
		{
			UnregisterFromVolume();
			RegisterToVolume();
		}
	}
}

void UKawaiiFluidEmitterComponent::DrawSpawnVolumeVisualization()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Location = GetComponentLocation() + SpawnOffset;
	const FQuat Rotation = GetComponentRotation().Quaternion();
	const FColor SpawnColor = SpawnVolumeWireframeColor;
	const float Duration = -1.0f;  // Redraw each frame
	const uint8 DepthPriority = 0;
	const float Thickness = WireframeThickness;

	if (IsShapeMode())
	{
		// Shape Volume visualization
		switch (ShapeType)
		{
		case EKawaiiFluidEmitterShapeType::Sphere:
			// Sphere is rotation-independent
			DrawDebugSphere(World, Location, SphereRadius, 24, SpawnColor, false, Duration, DepthPriority, Thickness);
			break;

		case EKawaiiFluidEmitterShapeType::Box:
			// Box with rotation
			DrawDebugBox(World, Location, BoxExtent, Rotation, SpawnColor, false, Duration, DepthPriority, Thickness);
			break;

		case EKawaiiFluidEmitterShapeType::Cylinder:
			{
				const float Radius = CylinderRadius;
				const float HalfHeight = CylinderHalfHeight;

				// Local coordinate cylinder vertices, then apply rotation
				const FVector LocalTopCenter = FVector(0, 0, HalfHeight);
				const FVector LocalBottomCenter = FVector(0, 0, -HalfHeight);

				const int32 NumSegments = 24;
				for (int32 i = 0; i < NumSegments; ++i)
				{
					const float Angle1 = (float)i / NumSegments * 2.0f * PI;
					const float Angle2 = (float)(i + 1) / NumSegments * 2.0f * PI;

					// Calculate local positions
					const FVector LocalTopP1 = LocalTopCenter + FVector(FMath::Cos(Angle1), FMath::Sin(Angle1), 0) * Radius;
					const FVector LocalTopP2 = LocalTopCenter + FVector(FMath::Cos(Angle2), FMath::Sin(Angle2), 0) * Radius;
					const FVector LocalBottomP1 = LocalBottomCenter + FVector(FMath::Cos(Angle1), FMath::Sin(Angle1), 0) * Radius;
					const FVector LocalBottomP2 = LocalBottomCenter + FVector(FMath::Cos(Angle2), FMath::Sin(Angle2), 0) * Radius;

					// Apply rotation and transform to world position
					const FVector TopP1 = Location + Rotation.RotateVector(LocalTopP1);
					const FVector TopP2 = Location + Rotation.RotateVector(LocalTopP2);
					const FVector BottomP1 = Location + Rotation.RotateVector(LocalBottomP1);
					const FVector BottomP2 = Location + Rotation.RotateVector(LocalBottomP2);

					DrawDebugLine(World, TopP1, TopP2, SpawnColor, false, Duration, DepthPriority, Thickness);
					DrawDebugLine(World, BottomP1, BottomP2, SpawnColor, false, Duration, DepthPriority, Thickness);
				}

				// Vertical lines (4 lines connecting top and bottom)
				for (int32 i = 0; i < 4; ++i)
				{
					const float Angle = (float)i / 4 * 2.0f * PI;
					const FVector LocalTopP = LocalTopCenter + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Radius;
					const FVector LocalBottomP = LocalBottomCenter + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Radius;

					const FVector TopP = Location + Rotation.RotateVector(LocalTopP);
					const FVector BottomP = Location + Rotation.RotateVector(LocalBottomP);
					DrawDebugLine(World, TopP, BottomP, SpawnColor, false, Duration, DepthPriority, Thickness);
				}
			}
			break;
		}
	}
	else // Stream mode
	{
		// Direction arrow (apply component rotation)
		const FVector WorldDir = Rotation.RotateVector(SpawnDirection.GetSafeNormal());
		const float ArrowLength = 100.0f;
		const FVector EndPoint = Location + WorldDir * ArrowLength;

		DrawDebugDirectionalArrow(World, Location, EndPoint, 20.0f, SpawnColor, false, Duration, DepthPriority, Thickness);

		// Stream radius circle
		if (StreamRadius > 0.0f)
		{
			FVector Right, Up;
			WorldDir.FindBestAxisVectors(Right, Up);

			const int32 NumSegments = 24;
			for (int32 i = 0; i < NumSegments; ++i)
			{
				const float Angle1 = (float)i / NumSegments * 2.0f * PI;
				const float Angle2 = (float)(i + 1) / NumSegments * 2.0f * PI;

				const FVector P1 = Location + (Right * FMath::Cos(Angle1) + Up * FMath::Sin(Angle1)) * StreamRadius;
				const FVector P2 = Location + (Right * FMath::Cos(Angle2) + Up * FMath::Sin(Angle2)) * StreamRadius;

				DrawDebugLine(World, P1, P2, SpawnColor, false, Duration, DepthPriority, Thickness);
			}
		}
	}
}
#endif
