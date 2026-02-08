// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/KawaiiFluidSimulationTypes.h"
#include "KawaiiFluidInteractionComponent.generated.h"

class UKawaiiFluidSimulatorSubsystem;
class UKawaiiFluidCollider;
class UKawaiiFluidMeshCollider;
class UKawaiiFluidPresetDataAsset;

/**
 * @brief Multicast delegate for fluid area enter events.
 * @param FluidTag Fluid tag (e.g., "Water", "Lava")
 * @param ParticleCount Number of particles in contact
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFluidEnter, FName, FluidTag, int32, ParticleCount);

/**
 * @brief Multicast delegate for fluid area exit events.
 * @param FluidTag Fluid tag (e.g., "Water", "Lava")
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFluidExit, FName, FluidTag);

/**
 * @brief Multicast delegate for fluid force updates.
 * @param Force Force vector from fluid (cm/s²)
 * @param Pressure Average pressure value
 * @param ContactCount Number of particles in contact
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnFluidForceUpdate, FVector, Force, float, Pressure, int32, ContactCount);

/**
 * @brief Multicast delegate for particle-to-bone collision events.
 * @param BoneIndex Bone index that received the collision
 * @param BoneName Bone name that received the collision
 * @param ContactCount Number of particles contacting that bone
 * @param AverageVelocity Average velocity of colliding particles
 * @param FluidName Name of the colliding fluid
 * @param ImpactOffset Offset from the bone origin to the impact location
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnBoneParticleCollision, int32, BoneIndex, FName, BoneName, int32, ContactCount, FVector, AverageVelocity, FName, FluidName, FVector, ImpactOffset);

/**
 * @brief Multicast delegate for per-bone fluid impacts.
 * @param BoneName Bone name that received the impact
 * @param ImpactSpeed Absolute fluid speed (cm/s)
 * @param ImpactForce Impact force (Newton)
 * @param ImpactDirection Impact direction (normalized)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnBoneFluidImpact, FName, BoneName, float, ImpactSpeed, float, ImpactForce, FVector, ImpactDirection);

/**
 * @brief Fluid interaction component.
 * Handles physical interaction between actors and fluid simulation.
 * 
 * @param TargetSubsystem Cached subsystem reference
 * @param bAutoCreateCollider Automatically create mesh collider
 * @param bEnableForceFeedback Enable GPU collision feedback
 * @param ForceSmoothingSpeed Smoothing speed for forces
 * @param DragCoefficient Drag coefficient (Cd)
 * @param DragForceMultiplier Gameplay force scale
 * @param bUseRelativeVelocityForForce Use relative velocity for drag
 * @param MinParticleCountForFluidEvent Threshold for tag events
 * @param CurrentFluidForce Smoothed fluid force
 * @param CurrentContactCount Total particle contacts
 * @param PreviousContactCount Previous frame contacts
 * @param CurrentAveragePressure Average fluid pressure
 * @param bEnableBoneImpactMonitoring Enable impact detection
 * @param MonitoredBones Bones to check for impacts
 * @param BoneImpactSpeedThreshold Speed threshold for impact events
 * @param bEnablePerBoneForce Enable bone-level drag
 * @param PerBoneForceSmoothingSpeed Bone force smoothing
 * @param PerBoneForceMultiplier Bone force scale
 * @param bEnableBoneCollisionEvents Enable events for Niagara
 * @param MinParticleCountForBoneEvent Bone event threshold
 * @param BoneEventCooldown Bone event rate limit
 * @param bEnableAutoPhysicsForces Enable buoyancy/drag
 * @param bApplyBuoyancy Apply upward force
 * @param bApplyDrag Apply flow resistance
 * @param BuoyancyMultiplier Buoyancy scale
 * @param PhysicsDragMultiplier Physics body drag scale
 * @param SubmergedVolumeMethod Submersion estimation method
 * @param FixedSubmersionRatio Ratio for FixedRatio method
 * @param BuoyancyDamping Vertical oscillation damping
 * @param AddedMassCoefficient Fluid inertia coefficient
 * @param FluidAngularDamping Rotational damping
 * @param FluidLinearDamping Linear drag damping
 * @param CurrentBuoyancyForce Applied buoyancy vector
 * @param EstimatedSubmergedVolume Volume in cm³
 * @param EstimatedBuoyancyCenterOffset Buoyancy center offset
 * @param AutoCollider Managed mesh collider instance
 * @param SmoothedForce Internal force accumulator
 * @param PreviousFluidTagStates Cache for enter/exit events
 * @param CurrentFluidTagCounts Current frame contacts per tag
 * @param ColliderIndex Associated collider ID
 * @param bGPUFeedbackEnabled State of feedback system
 * @param CurrentPerBoneForces Smoothed per-bone forces
 * @param SmoothedPerBoneForces Internal bone force accumulators
 * @param BoneIndexToNameCache Mapping for fast lookup
 * @param bBoneNameCacheInitialized Cache state flag
 * @param PerBoneForceDebugTimer Debug log throttler
 * @param CurrentBoneContactCounts Current contacts per bone
 * @param CurrentBoneAverageVelocities Average velocities per bone
 * @param BoneEventCooldownTimers Bone-level rate limiters
 * @param PreviousContactBones Last frame contact state
 * @param PreviousPhysicsVelocity Velocity for added mass
 * @param bEnableBoundaryParticles Enable adhesion system
 * @param BoundaryParticleSpacing Boundary density
 * @param BoundaryFrictionCoefficient Surface friction
 * @param bShowBoundaryParticles Debug visualization toggle
 * @param BoundaryParticleDebugColor Debug point color
 * @param BoundaryParticleDebugSize Debug point size
 * @param bShowBoundaryNormals Normal visualization toggle
 * @param BoundaryNormalLength Normal arrow length
 * @param BoundaryParticlePositions World positions
 * @param BoundaryParticleLocalPositions Mesh-local positions
 * @param BoundaryParticleNormals World surface normals
 * @param BoundaryParticleLocalNormals Local surface normals
 * @param BoundaryParticleBoneIndices Parent bone IDs
 * @param BoundaryParticleVertexIndices Source vertex IDs
 * @param bIsSkeletalMesh Mesh type flag
 * @param bBoundaryParticlesInitialized Initialization state
 */
UCLASS(ClassGroup=(KawaiiFluid), meta=(BlueprintSpawnableComponent, DisplayName="Kawaii Fluid Interaction"))
class KAWAIIFLUIDRUNTIME_API UKawaiiFluidInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKawaiiFluidInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(Transient)
	TObjectPtr<UKawaiiFluidSimulatorSubsystem> TargetSubsystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction")
	bool bAutoCreateCollider;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Drag Force Feedback")
	bool bEnableForceFeedback = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Drag Force Feedback",
	          meta = (EditCondition = "bEnableForceFeedback", ClampMin = "0.1", ClampMax = "50.0"))
	float ForceSmoothingSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Drag Force Feedback",
	          meta = (EditCondition = "bEnableForceFeedback", ClampMin = "0.1", ClampMax = "3.0"))
	float DragCoefficient = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Drag Force Feedback",
	          meta = (EditCondition = "bEnableForceFeedback", ClampMin = "0.0001", ClampMax = "10.0"))
	float DragForceMultiplier = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Drag Force Feedback",
	          meta = (EditCondition = "bEnableForceFeedback"))
	bool bUseRelativeVelocityForForce = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Drag Force Feedback",
	          meta = (EditCondition = "bEnableForceFeedback", ClampMin = "1", ClampMax = "100"))
	int32 MinParticleCountForFluidEvent = 5;

	UPROPERTY(BlueprintReadOnly, Category = "Fluid Interaction|Drag Force Feedback")
	FVector CurrentFluidForce;

	UPROPERTY(BlueprintReadOnly, Category = "Fluid Interaction|Drag Force Feedback")
	int32 CurrentContactCount;

	int32 PreviousContactCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Fluid Interaction|Drag Force Feedback")
	float CurrentAveragePressure;

	UPROPERTY(BlueprintAssignable, Category = "Fluid Interaction|Events")
	FOnFluidEnter OnFluidEnter;

	UPROPERTY(BlueprintAssignable, Category = "Fluid Interaction|Events")
	FOnFluidExit OnFluidExit;

	UPROPERTY(BlueprintAssignable, Category = "Fluid Interaction|Events")
	FOnFluidForceUpdate OnFluidForceUpdate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Bone Impact Monitoring")
	bool bEnableBoneImpactMonitoring = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Bone Impact Monitoring",
	          meta = (EditCondition = "bEnableBoneImpactMonitoring"))
	TArray<FName> MonitoredBones;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Bone Impact Monitoring",
	          meta = (EditCondition = "bEnableBoneImpactMonitoring", ClampMin = "0.0", ClampMax = "5000.0"))
	float BoneImpactSpeedThreshold = 500.0f;

	UPROPERTY(BlueprintAssignable, Category = "Fluid Interaction|Events")
	FOnBoneFluidImpact OnBoneFluidImpact;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Per-Bone Force",
	          meta = (EditCondition = "bEnableForceFeedback"))
	bool bEnablePerBoneForce = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Per-Bone Force",
	          meta = (EditCondition = "bEnablePerBoneForce", ClampMin = "0.1", ClampMax = "50.0"))
	float PerBoneForceSmoothingSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Per-Bone Force",
	          meta = (EditCondition = "bEnablePerBoneForce", ClampMin = "0.0001", ClampMax = "100.0"))
	float PerBoneForceMultiplier = 1.0f;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Per-Bone Force")
	FVector GetFluidForceForBone(int32 BoneIndex) const;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Per-Bone Force")
	FVector GetFluidForceForBoneByName(FName BoneName) const;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Per-Bone Force")
	TMap<int32, FVector> GetAllBoneForces() const { return CurrentPerBoneForces; }

	UFUNCTION(BlueprintCallable, Category = "Fluid Interaction|Per-Bone Force")
	void GetActiveBoneIndices(TArray<int32>& OutBoneIndices) const;

	UFUNCTION(BlueprintCallable, Category = "Fluid Interaction|Per-Bone Force")
	bool GetStrongestBoneForce(int32& OutBoneIndex, FVector& OutForce) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Bone Collision Events",
	          meta = (EditCondition = "bEnablePerBoneForce"))
	bool bEnableBoneCollisionEvents = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Bone Collision Events",
	          meta = (EditCondition = "bEnableBoneCollisionEvents", ClampMin = "1", ClampMax = "50"))
	int32 MinParticleCountForBoneEvent = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Bone Collision Events",
	          meta = (EditCondition = "bEnableBoneCollisionEvents", ClampMin = "0.0", ClampMax = "2.0"))
	float BoneEventCooldown = 0.1f;

	UPROPERTY(BlueprintAssignable, Category = "Fluid Interaction|Events")
	FOnBoneParticleCollision OnBoneParticleCollision;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Bone Collision Events")
	int32 GetBoneContactCount(int32 BoneIndex) const;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Bone Collision Events")
	TMap<int32, int32> GetAllBoneContactCounts() const { return CurrentBoneContactCounts; }

	UFUNCTION(BlueprintCallable, Category = "Fluid Interaction|Bone Collision Events")
	void GetBonesWithContacts(TArray<int32>& OutBoneIndices) const;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Bone Collision Events")
	FName GetBoneNameFromIndex(int32 BoneIndex) const;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Bone Collision Events")
	class USkeletalMeshComponent* GetOwnerSkeletalMesh() const;

	UFUNCTION(BlueprintCallable, Category = "Fluid Interaction|Bone Collision Events")
	bool GetMostContactedBone(int32& OutBoneIndex, int32& OutContactCount) const;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Drag Force Feedback")
	FVector GetCurrentFluidForce() const { return CurrentFluidForce; }

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Drag Force Feedback")
	float GetCurrentFluidPressure() const { return CurrentAveragePressure; }

	UFUNCTION(BlueprintCallable, Category = "Fluid Interaction|Drag Force Feedback")
	void ApplyFluidForceToCharacterMovement(float ForceScale = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Drag Force Feedback")
	bool IsCollidingWithFluidTag(FName FluidTag) const;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Drag Force Feedback")
	float GetFluidImpactSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Drag Force Feedback")
	float GetFluidImpactSpeedForBone(FName BoneName) const;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Drag Force Feedback")
	float GetFluidImpactForceMagnitude() const;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Drag Force Feedback")
	float GetFluidImpactForceMagnitudeForBone(FName BoneName) const;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Drag Force Feedback")
	FVector GetFluidImpactDirection() const;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Drag Force Feedback")
	FVector GetFluidImpactDirectionForBone(FName BoneName) const;

	UFUNCTION(BlueprintCallable, Category = "Fluid Interaction")
	void DetachAllFluid();

	UFUNCTION(BlueprintCallable, Category = "Fluid Interaction")
	void PushFluid(FVector Direction, float Force);

	UFUNCTION(BlueprintCallable, Category = "Fluid Interaction")
	bool HasValidTarget() const { return TargetSubsystem != nullptr; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Auto Physics Forces")
	bool bEnableAutoPhysicsForces = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Auto Physics Forces",
	          meta = (EditCondition = "bEnableAutoPhysicsForces"))
	bool bApplyBuoyancy = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Auto Physics Forces",
	          meta = (EditCondition = "bEnableAutoPhysicsForces"))
	bool bApplyDrag = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Auto Physics Forces",
	          meta = (EditCondition = "bEnableAutoPhysicsForces && bApplyBuoyancy", ClampMin = "0.0", ClampMax = "5.0"))
	float BuoyancyMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Auto Physics Forces",
	          meta = (EditCondition = "bEnableAutoPhysicsForces && bApplyDrag", ClampMin = "0.0", ClampMax = "10.0"))
	float PhysicsDragMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Auto Physics Forces",
	          meta = (EditCondition = "bEnableAutoPhysicsForces && bApplyBuoyancy"))
	ESubmergedVolumeMethod SubmergedVolumeMethod = ESubmergedVolumeMethod::ContactBased;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Auto Physics Forces",
	          meta = (EditCondition = "bEnableAutoPhysicsForces && bApplyBuoyancy && SubmergedVolumeMethod == ESubmergedVolumeMethod::FixedRatio",
	                  ClampMin = "0.0", ClampMax = "1.0"))
	float FixedSubmersionRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Auto Physics Forces",
	          meta = (EditCondition = "bEnableAutoPhysicsForces && bApplyBuoyancy", ClampMin = "0.0", ClampMax = "20.0"))
	float BuoyancyDamping = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Auto Physics Forces",
	          meta = (EditCondition = "bEnableAutoPhysicsForces", ClampMin = "0.0", ClampMax = "2.0"))
	float AddedMassCoefficient = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Auto Physics Forces",
	          meta = (EditCondition = "bEnableAutoPhysicsForces", ClampMin = "0.0", ClampMax = "5.0"))
	float FluidAngularDamping = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Auto Physics Forces",
	          meta = (EditCondition = "bEnableAutoPhysicsForces", ClampMin = "0.0", ClampMax = "5.0"))
	float FluidLinearDamping = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Fluid Interaction|Auto Physics Forces")
	FVector CurrentBuoyancyForce = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Fluid Interaction|Auto Physics Forces")
	float EstimatedSubmergedVolume = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Fluid Interaction|Auto Physics Forces")
	FVector EstimatedBuoyancyCenterOffset = FVector::ZeroVector;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Auto Physics Forces")
	FVector GetCurrentBuoyancyForce() const { return CurrentBuoyancyForce; }

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Auto Physics Forces")
	float GetEstimatedSubmergedVolume() const { return EstimatedSubmergedVolume; }

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TObjectPtr<UKawaiiFluidMeshCollider> AutoCollider;

	void CreateAutoCollider();
	void RegisterWithSimulator();
	void UnregisterFromSimulator();

	FVector SmoothedForce = FVector::ZeroVector;

	TMap<FName, bool> PreviousFluidTagStates;

	TMap<FName, int32> CurrentFluidTagCounts;

	int32 ColliderIndex = -1;

	bool bGPUFeedbackEnabled = false;

	TMap<int32, FVector> CurrentPerBoneForces;

	TMap<int32, FVector> SmoothedPerBoneForces;

	TMap<int32, FName> BoneIndexToNameCache;

	bool bBoneNameCacheInitialized = false;

	float PerBoneForceDebugTimer = 0.0f;

	TMap<int32, int32> CurrentBoneContactCounts;

	TMap<int32, FVector> CurrentBoneAverageVelocities;

	TMap<int32, float> BoneEventCooldownTimers;

	TSet<int32> PreviousContactBones;

	void ProcessBoneCollisionEvents(float DeltaTime, const TArray<struct FGPUCollisionFeedback>& AllFeedback, int32 FeedbackCount);

	void InitializeBoneNameCache();

	void ProcessPerBoneForces(float DeltaTime, const TArray<struct FGPUCollisionFeedback>& AllFeedback, int32 FeedbackCount, float ParticleRadius);

	void ProcessCollisionFeedback(float DeltaTime);

	void UpdateFluidTagEvents();

	void CheckBoneImpacts();

	void EnableGPUCollisionFeedbackIfNeeded();

	class UPrimitiveComponent* FindPhysicsBody() const;

	float CalculateSubmergedVolumeFromContacts(int32 ContactCount, float ParticleRadius) const;

	FVector CalculateBuoyancyForce(float SubmergedVolume, float FluidDensity, const FVector& Gravity) const;

	float GetCurrentFluidDensity() const;

	float GetCurrentParticleRadius() const;

	FVector GetCurrentGravity() const;

	void ApplyAutoPhysicsForces(float DeltaTime);

	FVector PreviousPhysicsVelocity = FVector::ZeroVector;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Boundary Particles")
	bool bEnableBoundaryParticles = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Boundary Particles",
	          meta = (EditCondition = "bEnableBoundaryParticles", ClampMin = "1.0", ClampMax = "50.0"))
	float BoundaryParticleSpacing = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Boundary Particles",
	          meta = (EditCondition = "bEnableBoundaryParticles", ClampMin = "0.0", ClampMax = "2.0"))
	float BoundaryFrictionCoefficient = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Boundary Particles",
	          meta = (EditCondition = "bEnableBoundaryParticles"))
	bool bShowBoundaryParticles = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Boundary Particles",
	          meta = (EditCondition = "bShowBoundaryParticles"))
	FColor BoundaryParticleDebugColor = FColor::Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Boundary Particles",
	          meta = (EditCondition = "bShowBoundaryParticles", ClampMin = "0.5", ClampMax = "10.0"))
	float BoundaryParticleDebugSize = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Boundary Particles",
	          meta = (EditCondition = "bShowBoundaryParticles"))
	bool bShowBoundaryNormals = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Interaction|Boundary Particles",
	          meta = (EditCondition = "bShowBoundaryParticles && bShowBoundaryNormals", ClampMin = "1.0", ClampMax = "50.0"))
	float BoundaryNormalLength = 10.0f;

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Boundary Particles")
	int32 GetBoundaryParticleCount() const { return BoundaryParticlePositions.Num(); }

	UFUNCTION(BlueprintPure, Category = "Fluid Interaction|Boundary Particles")
	const TArray<FVector>& GetBoundaryParticlePositions() const { return BoundaryParticlePositions; }

	UFUNCTION(BlueprintCallable, Category = "Fluid Interaction|Boundary Particles")
	void RegenerateBoundaryParticles();

	void CollectGPUBoundaryParticles(struct FGPUBoundaryParticles& OutBoundaryParticles) const;

	void CollectLocalBoundaryParticles(TArray<struct FGPUBoundaryParticleLocal>& OutLocalParticles, float Psi, float Friction) const;

	void CollectBoneTransformsForBoundary(TArray<FMatrix>& OutBoneTransforms, FMatrix& OutComponentTransform) const;

	int32 GetBoundaryOwnerID() const { return GetUniqueID(); }

	bool HasLocalBoundaryParticles() const { return bEnableBoundaryParticles && bBoundaryParticlesInitialized && BoundaryParticleLocalPositions.Num() > 0; }

	bool HasInitializedBoundaryParticles() const { return bBoundaryParticlesInitialized && BoundaryParticleLocalPositions.Num() > 0; }

	bool IsBoundaryAdhesionEnabled() const { return bEnableBoundaryParticles && bBoundaryParticlesInitialized && BoundaryParticlePositions.Num() > 0; }

private:
	TArray<FVector> BoundaryParticlePositions;

	TArray<FVector> BoundaryParticleLocalPositions;

	TArray<FVector> BoundaryParticleNormals;

	TArray<FVector> BoundaryParticleLocalNormals;

	TArray<int32> BoundaryParticleBoneIndices;

	TArray<int32> BoundaryParticleVertexIndices;

	bool bIsSkeletalMesh = false;

	bool bBoundaryParticlesInitialized = false;

	void GenerateBoundaryParticles();

	void UpdateBoundaryParticlePositions();

	void DrawDebugBoundaryParticles();

	void SampleTriangleSurface(const FVector& V0, const FVector& V1, const FVector& V2,
	                           float Spacing, TArray<FVector>& OutPoints);

	void SampleSphereSurface(const struct FKSphereElem& Sphere, int32 BoneIndex, const FTransform& LocalTransform);

	void SampleCapsuleSurface(const struct FKSphylElem& Capsule, int32 BoneIndex);

	void SampleBoxSurface(const struct FKBoxElem& Box, int32 BoneIndex);

	void SampleConvexSurface(const struct FKConvexElem& Convex, int32 BoneIndex);

	void SampleHemisphere(const FTransform& Transform, float Radius, float ZOffset,
	                      int32 ZDirection, int32 BoneIndex, int32 NumSamples);

	void SampleAggGeomSurfaces(const struct FKAggregateGeom& AggGeom, int32 BoneIndex);
};
