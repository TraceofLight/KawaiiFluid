// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Core/KawaiiFluidSimulationTypes.h"
#include "Core/KawaiiFluidRenderingTypes.h"
#include "KawaiiFluidVolumeComponent.generated.h"

class UKawaiiFluidSimulationModule;
class UKawaiiFluidPresetDataAsset;
class UNiagaraSystem;

/**
 * @brief Kawaii Fluid Volume Component.
 * Defines the simulation bounds and spatial partitioning for fluid particles.
 * 
 * @param bUniformSize Use uniform (cube) size for simulation volume
 * @param UniformVolumeSize Cube dimensions in cm
 * @param VolumeSize Per-axis dimensions in cm
 * @param bUseUnlimitedSize Disable volume boundaries entirely
 * @param Preset The fluid preset defining physics and rendering
 * @param MaxParticleCount Maximum GPU buffer capacity for this volume
 * @param bUseWorldCollision Enable interaction with world geometry
 * @param bEnableStaticBoundaryParticles Use static particles for boundary density
 * @param StaticBoundaryParticleSpacing Spacing for static boundary particles
 * @param bEnableCollisionEvents Enable hit events for particles
 * @param MinVelocityForEvent Speed threshold for events
 * @param MaxEventsPerFrame Performance limit for event triggering
 * @param EventCooldownPerParticle Per-particle event rate limit
 * @param OnParticleHit Delegate fired on particle collisions
 * @param bEnableShadow Enable shadow casting via ISM
 * @param ShadowMeshQuality Polygon detail for shadow spheres
 * @param ShadowCullDistance Max distance for shadow rendering
 * @param ShadowRadiusOffset Size adjustment for shadow spheres
 * @param SplashVFX Niagara system for splash effects
 * @param SplashVelocityThreshold Speed required to trigger splash
 * @param MaxSplashVFXPerFrame Budget for splash spawning
 * @param SplashConditionMode Logic for triggering splashes
 * @param IsolationNeighborThreshold Neighbor count for isolation check
 * @param DebugDrawMode Particle visualization mode
 * @param ISMDebugColor Color for ISM debug particles
 * @param bShowStaticBoundaryParticles Visual debug for boundaries
 * @param StaticBoundaryPointSize Debug point size
 * @param StaticBoundaryColor Debug point color
 * @param bShowStaticBoundaryNormals Visual debug for boundary normals
 * @param StaticBoundaryNormalLength Normal arrow length
 * @param BrushSettings Brush settings for particle painting in editor
 * @param bBrushModeActive Brush mode active state
 * @param bShowBoundsInEditor Internal toggle for editor wireframe
 * @param bShowBoundsAtRuntime Internal toggle for runtime wireframe
 * @param BoundsColor Wireframe color
 * @param BoundsLineThickness Wireframe line thickness
 * @param bShowZOrderSpaceWireframe Visual debug for grid cells
 * @param ZOrderSpaceWireframeColor Grid wireframe color
 * @param bUseHybridTiledZOrder Enable unlimited simulation range
 * @param CellSize Derived spatial cell size
 * @param GridResolutionPreset Current grid detail level
 * @param GridAxisBits Number of bits for spatial keys
 * @param GridResolution Cells per axis
 * @param MaxCells Total grid capacity
 * @param BoundsExtent Current simulation box extent
 * @param WorldBoundsMin World space minimum bound
 * @param WorldBoundsMax World space maximum bound
 * @param RegisteredModules Fluid modules using this Volume
 */
UCLASS(ClassGroup=(KawaiiFluid), meta=(BlueprintSpawnableComponent, DisplayName="Kawaii Fluid Volume"))
class KAWAIIFLUIDRUNTIME_API UKawaiiFluidVolumeComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UKawaiiFluidVolumeComponent();

	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume",
		meta = (DisplayName = "Uniform Size", EditCondition = "!bUseUnlimitedSize", EditConditionHides))
	bool bUniformSize = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume",
		meta = (EditCondition = "bUniformSize && !bUseUnlimitedSize", EditConditionHides, DisplayName = "Size", ClampMin = "10.0"))
	float UniformVolumeSize = 2560.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume",
		meta = (EditCondition = "!bUniformSize && !bUseUnlimitedSize", EditConditionHides, DisplayName = "Size"))
	FVector VolumeSize = FVector(2560.0f, 2560.0f, 2560.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume",
		meta = (DisplayName = "Use Unlimited Size", EditCondition = "bUseHybridTiledZOrder", EditConditionHides))
	bool bUseUnlimitedSize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume")
	TObjectPtr<UKawaiiFluidPresetDataAsset> Preset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume", meta = (ClampMin = "1"))
	int32 MaxParticleCount = 200000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Collision")
	bool bUseWorldCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Collision",
		meta = (DisplayName = "Enable Static Boundary Particles",
		        EditCondition = "bUseWorldCollision && !bUseUnlimitedSize", EditConditionHides))
	bool bEnableStaticBoundaryParticles = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Collision",
		meta = (EditCondition = "bUseWorldCollision && bEnableStaticBoundaryParticles && !bUseUnlimitedSize", EditConditionHides,
		        ClampMin = "1.0", ClampMax = "50.0", DisplayName = "Boundary Particle Spacing"))
	float StaticBoundaryParticleSpacing = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Collision|Events")
	bool bEnableCollisionEvents = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Collision|Events",
	          meta = (ClampMin = "0.0", EditCondition = "bEnableCollisionEvents"))
	float MinVelocityForEvent = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Collision|Events",
	          meta = (ClampMin = "0", EditCondition = "bEnableCollisionEvents"))
	int32 MaxEventsPerFrame = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Collision|Events",
	          meta = (ClampMin = "0.0", EditCondition = "bEnableCollisionEvents"))
	float EventCooldownPerParticle = 0.1f;

	UPROPERTY(BlueprintAssignable, Category = "Fluid Volume|Collision|Events")
	FOnFluidParticleHitComponent OnParticleHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Rendering")
	bool bEnableShadow = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Rendering", meta = (EditCondition = "bEnableShadow"))
	EFluidShadowMeshQuality ShadowMeshQuality = EFluidShadowMeshQuality::Medium;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Rendering", meta = (EditCondition = "bEnableShadow", ClampMin = "0"))
	float ShadowCullDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Rendering", meta = (EditCondition = "bEnableShadow", ClampMin = "-50.0", ClampMax = "50.0"))
	float ShadowRadiusOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|VFX")
	TObjectPtr<UNiagaraSystem> SplashVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|VFX",
	          meta = (ClampMin = "0", EditCondition = "SplashVFX != nullptr"))
	float SplashVelocityThreshold = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|VFX",
	          meta = (ClampMin = "1", ClampMax = "50", EditCondition = "SplashVFX != nullptr"))
	int32 MaxSplashVFXPerFrame = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|VFX",
	          meta = (EditCondition = "SplashVFX != nullptr"))
	ESplashConditionMode SplashConditionMode = ESplashConditionMode::VelocityAndIsolation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|VFX",
	          meta = (ClampMin = "0", ClampMax = "10", EditCondition = "SplashVFX != nullptr && SplashConditionMode != ESplashConditionMode::VelocityOnly"))
	int32 IsolationNeighborThreshold = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Debug|Draw Mode")
	EKawaiiFluidDebugDrawMode DebugDrawMode = EKawaiiFluidDebugDrawMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Debug|Draw Mode",
	          meta = (EditCondition = "DebugDrawMode == EKawaiiFluidDebugDrawMode::ISM", EditConditionHides))
	FLinearColor ISMDebugColor = FLinearColor(0.2f, 0.5f, 1.0f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Debug|Boundary")
	bool bShowStaticBoundaryParticles = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Debug|Boundary",
	          meta = (EditCondition = "bShowStaticBoundaryParticles", ClampMin = "1.0", ClampMax = "50.0"))
	float StaticBoundaryPointSize = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Debug|Boundary",
	          meta = (EditCondition = "bShowStaticBoundaryParticles"))
	FColor StaticBoundaryColor = FColor::Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Debug|Boundary",
	          meta = (EditCondition = "bShowStaticBoundaryParticles"))
	bool bShowStaticBoundaryNormals = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Debug|Boundary",
	          meta = (EditCondition = "bShowStaticBoundaryParticles && bShowStaticBoundaryNormals", ClampMin = "1.0", ClampMax = "100.0"))
	float StaticBoundaryNormalLength = 10.0f;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Brush Editor")
	FFluidBrushSettings BrushSettings;

	bool bBrushModeActive = false;
#endif

	bool bShowBoundsInEditor = true;

	bool bShowBoundsAtRuntime = false;

	FColor BoundsColor = FColor::Cyan;

	float BoundsLineThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Debug|Z-Order Space", meta = (DisplayName = "Show Z-Order Space Wireframe"))
	bool bShowZOrderSpaceWireframe = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Debug|Z-Order Space",
		meta = (EditCondition = "bShowZOrderSpaceWireframe", EditConditionHides, DisplayName = "Z-Order Space Wireframe Color"))
	FColor ZOrderSpaceWireframeColor = FColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Volume|Z-Order Space",
		meta = (DisplayName = "Enable Unlimited Simulation Range"))
	bool bUseHybridTiledZOrder = true;

	float CellSize = 20.0f;

	EGridResolutionPreset GridResolutionPreset = EGridResolutionPreset::Medium;

	int32 GridAxisBits = 7;

	int32 GridResolution = 128;

	int32 MaxCells = 2097152;

	float BoundsExtent = 2560.0f;

	FVector WorldBoundsMin = FVector(-1280.0f, -1280.0f, -1280.0f);

	FVector WorldBoundsMax = FVector(1280.0f, 1280.0f, 1280.0f);

	UFUNCTION(BlueprintCallable, Category = "Fluid Volume")
	void RecalculateBounds();

	UFUNCTION(BlueprintCallable, Category = "Fluid Volume")
	bool IsPositionInBounds(const FVector& WorldPosition) const;

	UFUNCTION(BlueprintCallable, Category = "Fluid Volume")
	void GetSimulationBounds(FVector& OutMin, FVector& OutMax) const;

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	FVector GetWorldBoundsMin() const { return WorldBoundsMin; }

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	FVector GetWorldBoundsMax() const { return WorldBoundsMax; }

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	FVector GetEffectiveVolumeSize() const { return bUniformSize ? FVector(UniformVolumeSize) : VolumeSize; }

	FVector GetVolumeHalfExtent() const { return GetEffectiveVolumeSize() * 0.5f; }

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	float GetWallBounce() const;

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	float GetWallFriction() const;

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	float GetCellSize() const { return CellSize; }

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	float GetBoundsExtent() const { return BoundsExtent; }

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	EGridResolutionPreset GetGridResolutionPreset() const { return GridResolutionPreset; }

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	int32 GetGridAxisBits() const { return GridAxisBits; }

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	bool IsStaticBoundaryParticlesEnabled() const { return !bUseUnlimitedSize && bEnableStaticBoundaryParticles; }

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	float GetStaticBoundaryParticleSpacing() const { return StaticBoundaryParticleSpacing; }

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	UKawaiiFluidPresetDataAsset* GetPreset() const { return Preset; }

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	float GetParticleSpacing() const;

	UFUNCTION(BlueprintCallable, Category = "Fluid Volume|Debug")
	void SetDebugDrawMode(EKawaiiFluidDebugDrawMode Mode);

	UFUNCTION(BlueprintPure, Category = "Fluid Volume|Debug")
	EKawaiiFluidDebugDrawMode GetDebugDrawMode() const { return DebugDrawMode; }

	UFUNCTION(BlueprintCallable, Category = "Fluid Volume|Debug")
	void DisableDebugDraw();

	const TArray<TWeakObjectPtr<UKawaiiFluidSimulationModule>>& GetRegisteredModules() const { return RegisteredModules; }

	void RegisterModule(UKawaiiFluidSimulationModule* Module);

	void UnregisterModule(UKawaiiFluidSimulationModule* Module);

	UFUNCTION(BlueprintPure, Category = "Fluid Volume")
	int32 GetRegisteredModuleCount() const { return RegisteredModules.Num(); }

private:
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UKawaiiFluidSimulationModule>> RegisteredModules;

	void RegisterToSubsystem();
	void UnregisterFromSubsystem();

	void DrawBoundsVisualization();
};
