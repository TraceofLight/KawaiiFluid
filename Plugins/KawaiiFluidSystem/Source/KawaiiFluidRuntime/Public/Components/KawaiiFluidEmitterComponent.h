// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "KawaiiFluidEmitterComponent.generated.h"

class AKawaiiFluidEmitter;
class AKawaiiFluidVolume;
class UKawaiiFluidSimulationModule;
class UBillboardComponent;
class APawn;

/**
 * @brief Emitter type for KawaiiFluidEmitterComponent.
 */
UENUM(BlueprintType)
enum class EKawaiiFluidEmitterMode : uint8
{
	Fill UMETA(DisplayName = "Fill"),
	Stream UMETA(DisplayName = "Stream")
};

/**
 * @brief Shape type for Shape emitter mode.
 */
UENUM(BlueprintType)
enum class EKawaiiFluidEmitterShapeType : uint8
{
	Sphere UMETA(DisplayName = "Sphere"),
	Cube UMETA(DisplayName = "Cube"),
	Cylinder UMETA(DisplayName = "Cylinder")
};

/**
 * @brief Kawaii Fluid Emitter Component.
 * Handles particle spawning logic including Fill and Stream modes with hexagonal packing.
 * 
 * @param bEnabled Enable or disable particle emission
 * @param TargetVolume The target volume to emit particles into
 * @param EmitterMode Current emission mode (Fill or Stream)
 * @param ShapeType Shape type for Fill mode
 * @param SphereRadius Radius for sphere shape
 * @param CubeHalfSize Half-size for cube shape
 * @param CylinderRadius Radius for cylinder shape
 * @param CylinderHalfHeight Half-height for cylinder shape
 * @param StreamRadius Cross-sectional radius for stream emission
 * @param LayersPerSecond Target spawn rate for stream mode
 * @param bUseStreamJitter Whether to apply random offset to stream particles
 * @param StreamJitterAmount Max random offset fraction (0.0 ~ 0.5)
 * @param bUseWorldSpaceVelocity Whether velocity direction is world or local space
 * @param InitialVelocityDirection Direction vector for spawned particles
 * @param InitialSpeed Initial speed in cm/s
 * @param MaxParticleCount Particle budget for this emitter (0 = unlimited)
 * @param bRecycleOldestParticles Whether to recycle particles when limit is reached
 * @param bAutoStartSpawning Start spawning automatically on BeginPlay
 * @param bUseDistanceOptimization Only spawn when reference actor is in range
 * @param DistanceReferenceActor Actor used for distance check (default: Player)
 * @param ActivationDistance Range at which emitter activates
 * @param bAutoRespawnOnReentry Automatically re-fill when re-entering range
 * @param SpawnAccumulator Accumulated time for rate-based spawning
 * @param SpawnedParticleCount Total particles spawned by this emitter
 * @param bAutoSpawnExecuted Whether auto spawn has been executed (Fill mode)
 * @param bStreamSpawning Whether stream is currently spawning (Stream mode)
 * @param bJustCleared Flag to track if particles were just cleared
 * @param bPendingVolumeSearch Whether we need to search for volume in next tick
 * @param bDistanceActivated Current activation state based on player distance
 * @param CachedPlayerPawn Cached player pawn reference
 * @param DistanceCheckAccumulator Timer for distance check interval
 * @param bNeedsRespawnOnReentry Track if Fill mode needs re-spawn on reentry
 * @param BillboardComponent Billboard icon for editor visualization
 * @param VelocityArrow Velocity direction arrow (editor only)
 * @param bShowSpawnVolumeWireframe Internal toggle for wireframe
 * @param SpawnVolumeWireframeColor Wireframe color
 * @param WireframeThickness Wireframe thickness
 * @param bAutoFindVolume Internal toggle for auto volume search
 * @param bAutoCalculateParticleCount Internal toggle for count calculation
 * @param ParticleCount Manual particle count setting
 * @param bUseJitter Internal toggle for jitter
 * @param JitterAmount Jitter magnitude
 * @param SpawnOffset World position offset
 * @param SpawnDirection Direction vector for stream
 * @param StreamParticleSpacing Internal spacing cache
 * @param StreamLayerSpacingRatio Internal HCP ratio for stream
 * @param CachedSourceID Unique ID allocated from Subsystem
 */
UCLASS(ClassGroup = (KawaiiFluid), meta = (BlueprintSpawnableComponent, DisplayName = "Kawaii Fluid Emitter"))
class KAWAIIFLUIDRUNTIME_API UKawaiiFluidEmitterComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UKawaiiFluidEmitterComponent();

	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter")
	TObjectPtr<AKawaiiFluidVolume> TargetVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter")
	EKawaiiFluidEmitterMode EmitterMode = EKawaiiFluidEmitterMode::Stream;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Fill Shape",
		meta = (EditCondition = "EmitterMode == EKawaiiFluidEmitterMode::Fill", EditConditionHides))
	EKawaiiFluidEmitterShapeType ShapeType = EKawaiiFluidEmitterShapeType::Sphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Fill Shape",
		meta = (EditCondition = "EmitterMode == EKawaiiFluidEmitterMode::Fill && ShapeType == EKawaiiFluidEmitterShapeType::Sphere", EditConditionHides, ClampMin = "1.0"))
	float SphereRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Fill Shape",
		meta = (EditCondition = "EmitterMode == EKawaiiFluidEmitterMode::Fill && ShapeType == EKawaiiFluidEmitterShapeType::Cube", EditConditionHides))
	FVector CubeHalfSize = FVector(50.0f, 50.0f, 50.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Fill Shape",
		meta = (EditCondition = "EmitterMode == EKawaiiFluidEmitterMode::Fill && ShapeType == EKawaiiFluidEmitterShapeType::Cylinder", EditConditionHides, ClampMin = "1.0"))
	float CylinderRadius = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Fill Shape",
		meta = (EditCondition = "EmitterMode == EKawaiiFluidEmitterMode::Fill && ShapeType == EKawaiiFluidEmitterShapeType::Cylinder", EditConditionHides, ClampMin = "1.0"))
	float CylinderHalfHeight = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Stream",
		meta = (EditCondition = "EmitterMode == EKawaiiFluidEmitterMode::Stream", EditConditionHides, ClampMin = "1.0"))
	float StreamRadius = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Stream",
		meta = (DisplayName = "Layers Per Second",
		        EditCondition = "EmitterMode == EKawaiiFluidEmitterMode::Stream", 
		        EditConditionHides, ClampMin = "1.0", ClampMax = "300.0"))
	float LayersPerSecond = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Stream",
		meta = (EditCondition = "EmitterMode == EKawaiiFluidEmitterMode::Stream", EditConditionHides))
	bool bUseStreamJitter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Stream",
		meta = (EditCondition = "EmitterMode == EKawaiiFluidEmitterMode::Stream && bUseStreamJitter", 
		        EditConditionHides, ClampMin = "0.0", ClampMax = "0.5"))
	float StreamJitterAmount = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Velocity")
	bool bUseWorldSpaceVelocity = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Velocity")
	FVector InitialVelocityDirection = FVector(0, 0, -1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Velocity", meta = (ClampMin = "0.0"))
	float InitialSpeed = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Limits", meta = (ClampMin = "0"))
	int32 MaxParticleCount = 100000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Limits",
		meta = (DisplayName = "Continuous Spawn", EditCondition = "MaxParticleCount > 0 && EmitterMode == EKawaiiFluidEmitterMode::Stream", EditConditionHides))
	bool bRecycleOldestParticles = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter")
	bool bAutoStartSpawning = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Optimization")
	bool bUseDistanceOptimization = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Optimization",
		meta = (EditCondition = "bUseDistanceOptimization", EditConditionHides))
	TObjectPtr<AActor> DistanceReferenceActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Optimization",
		meta = (EditCondition = "bUseDistanceOptimization", EditConditionHides, ClampMin = "100.0"))
	float ActivationDistance = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Emitter|Optimization",
		meta = (EditCondition = "bUseDistanceOptimization && EmitterMode == EKawaiiFluidEmitterMode::Fill", EditConditionHides))
	bool bAutoRespawnOnReentry = true;

	UFUNCTION(BlueprintPure, Category = "Target")
	AKawaiiFluidVolume* GetTargetVolume() const { return TargetVolume; }

	UFUNCTION(BlueprintCallable, Category = "Target")
	void SetTargetVolume(AKawaiiFluidVolume* NewVolume);

	UFUNCTION(BlueprintPure, Category = "Emitter")
	AKawaiiFluidEmitter* GetOwnerEmitter() const;

	UFUNCTION(BlueprintPure, Category = "Emitter")
	float GetParticleSpacing() const;

	UFUNCTION(BlueprintCallable, Category = "Emitter")
	void SpawnFill();

	UFUNCTION(BlueprintCallable, Category = "Emitter")
	void BurstSpawn(int32 Count);

	UFUNCTION(BlueprintPure, Category = "Emitter")
	int32 GetSpawnedParticleCount() const { return SpawnedParticleCount; }

	UFUNCTION(BlueprintCallable, Category = "Emitter")
	void ClearSpawnedParticles();

	UFUNCTION(BlueprintPure, Category = "Emitter")
	bool HasReachedParticleLimit() const;

	UFUNCTION(BlueprintPure, Category = "Emitter")
	bool IsFillMode() const { return EmitterMode == EKawaiiFluidEmitterMode::Fill; }

	UFUNCTION(BlueprintPure, Category = "Emitter")
	bool IsStreamMode() const { return EmitterMode == EKawaiiFluidEmitterMode::Stream; }

	UFUNCTION(BlueprintCallable, Category = "Emitter")
	void StartStreamSpawn();

	UFUNCTION(BlueprintCallable, Category = "Emitter")
	void StopStreamSpawn();

	UFUNCTION(BlueprintPure, Category = "Emitter")
	bool IsStreamSpawning() const { return bStreamSpawning; }

protected:
	float SpawnAccumulator = 0.0f;

	int32 SpawnedParticleCount = 0;

	bool bAutoSpawnExecuted = false;

	bool bStreamSpawning = false;

	bool bJustCleared = false;

	bool bPendingVolumeSearch = false;

	bool bDistanceActivated = true;

	TWeakObjectPtr<APawn> CachedPlayerPawn;

	float DistanceCheckAccumulator = 0.0f;

	static constexpr float DistanceCheckInterval = 0.1f;

	bool bNeedsRespawnOnReentry = false;

	void ProcessContinuousSpawn(float DeltaTime);

	void ProcessStreamEmitter(float DeltaTime);

	int32 SpawnParticlesSphereHexagonal(FVector Center, FQuat Rotation, float Radius, float Spacing, FVector InInitialVelocity);

	int32 SpawnParticlesCubeHexagonal(FVector Center, FQuat Rotation, FVector HalfSize, float Spacing, FVector InInitialVelocity);

	int32 SpawnParticlesCylinderHexagonal(FVector Center, FQuat Rotation, float Radius, float HalfHeight, float Spacing, FVector InInitialVelocity);

	void SpawnStreamLayer(FVector Position, FVector LayerDirection, FVector VelocityDirection, float Speed, float Radius, float Spacing);

	void SpawnStreamLayerBatch(FVector Position, FVector LayerDirection, FVector VelocityDirection, 
	                           float Speed, float Radius, float Spacing,
	                           TArray<FVector>& OutPositions, TArray<FVector>& OutVelocities);

	void QueueSpawnRequest(const TArray<FVector>& Positions, const TArray<FVector>& Velocities);

	UKawaiiFluidSimulationModule* GetSimulationModule() const;

	void RecycleOldestParticlesIfNeeded(int32 NewParticleCount);

	void UpdateVelocityArrowVisualization();

	void UpdateDistanceOptimization(float DeltaTime);

	void OnDistanceActivationChanged(bool bNewState);

	void DespawnAllParticles();

	APawn* GetPlayerPawn();

	FORCEINLINE float GetHysteresisDistance() const { return ActivationDistance * 0.1f; }

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TObjectPtr<UBillboardComponent> BillboardComponent;

	UPROPERTY(Transient)
	TObjectPtr<class UArrowComponent> VelocityArrow;
#endif

	bool bShowSpawnVolumeWireframe = true;
	FColor SpawnVolumeWireframeColor = FColor::Cyan;
	float WireframeThickness = 2.0f;

	bool bAutoFindVolume = true;
	bool bAutoCalculateParticleCount = true;
	int32 ParticleCount = 500;
	bool bUseJitter = true;
	float JitterAmount = 0.2f;
	
	FVector SpawnOffset = FVector::ZeroVector;
	FVector SpawnDirection = FVector(0, 0, -1);
	float StreamParticleSpacing = 0.0f;
	float StreamLayerSpacingRatio = 0.816f;

	int32 CachedSourceID = -1;

	void RegisterToVolume();

	void UnregisterFromVolume();

	AKawaiiFluidVolume* FindNearestVolume() const;

#if WITH_EDITOR
	void DrawSpawnVolumeVisualization();

	void DrawDistanceVisualization();
#endif
};
