// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FluidParticle.generated.h"

/**
 * Fluid particle structure
 * Base unit of PBF simulation
 */
USTRUCT(BlueprintType)
struct KAWAIIFLUIDRUNTIME_API FFluidParticle
{
	GENERATED_BODY()

	// Position
	UPROPERTY(BlueprintReadWrite, Category = "Particle")
	FVector Position;

	// Predicted position (used by solver)
	UPROPERTY(BlueprintReadOnly, Category = "Particle")
	FVector PredictedPosition;

	// Velocity
	UPROPERTY(BlueprintReadWrite, Category = "Particle")
	FVector Velocity;

	// Mass
	UPROPERTY(BlueprintReadWrite, Category = "Particle")
	float Mass;

	// Density (calculated every frame)
	UPROPERTY(BlueprintReadOnly, Category = "Particle")
	float Density;

	// Lagrange multiplier (for density constraint)
	float Lambda;

	// Adhesion state
	UPROPERTY(BlueprintReadOnly, Category = "Particle")
	bool bIsAttached;

	// Attached actor
	UPROPERTY(BlueprintReadOnly, Category = "Particle")
	TWeakObjectPtr<AActor> AttachedActor;

	// Attached bone name (for skeletal mesh)
	UPROPERTY(BlueprintReadOnly, Category = "Particle")
	FName AttachedBoneName;

	// Relative position in bone local coordinates (for bone motion tracking)
	FVector AttachedLocalOffset;

	// Surface normal of attached surface (for surface slip calculation)
	FVector AttachedSurfaceNormal;

	// Detached this frame (prevents reattachment in same frame)
	bool bJustDetached;

	// Near ground (for reduced adhesion maintenance margin)
	bool bNearGround;

	// Near boundary particle (for debug visualization, doesn't affect physics)
	bool bNearBoundary;

	// Particle ID
	UPROPERTY(BlueprintReadOnly, Category = "Particle")
	int32 ParticleID;

	// Neighbor particle indices (for caching)
	TArray<int32> NeighborIndices;

	//========================================
	// Source Identification
	//========================================

	// Source ID (PresetIndex | ComponentIndex << 16)
	UPROPERTY(BlueprintReadOnly, Category = "Particle")
	int32 SourceID;

	// Whether this is a surface particle (used for surface-only rendering optimization)
	UPROPERTY(BlueprintReadOnly, Category = "Particle")
	bool bIsSurfaceParticle;

	// Surface normal (for surface tension calculation)
	FVector SurfaceNormal;

	// Trail spawned (prevents duplicate spawning)
	bool bTrailSpawned;

	FFluidParticle()
		: Position(FVector::ZeroVector)
		, PredictedPosition(FVector::ZeroVector)
		, Velocity(FVector::ZeroVector)
		, Mass(1.0f)
		, Density(0.0f)
		, Lambda(0.0f)
		, bIsAttached(false)
		, AttachedBoneName(NAME_None)
		, AttachedLocalOffset(FVector::ZeroVector)
		, AttachedSurfaceNormal(FVector::UpVector)
		, bJustDetached(false)
		, bNearGround(false)
		, bNearBoundary(false)
		, ParticleID(-1)
		, SourceID(-1)
		, bIsSurfaceParticle(false)
		, SurfaceNormal(FVector::ZeroVector)
		, bTrailSpawned(false)
	{
	}

	FFluidParticle(const FVector& InPosition, int32 InID)
		: Position(InPosition)
		, PredictedPosition(InPosition)
		, Velocity(FVector::ZeroVector)
		, Mass(1.0f)
		, Density(0.0f)
		, Lambda(0.0f)
		, bIsAttached(false)
		, AttachedBoneName(NAME_None)
		, AttachedLocalOffset(FVector::ZeroVector)
		, AttachedSurfaceNormal(FVector::UpVector)
		, bJustDetached(false)
		, bNearGround(false)
		, bNearBoundary(false)
		, ParticleID(InID)
		, SourceID(-1)
		, bIsSurfaceParticle(false)
		, SurfaceNormal(FVector::ZeroVector)
		, bTrailSpawned(false)
	{
	}
};
