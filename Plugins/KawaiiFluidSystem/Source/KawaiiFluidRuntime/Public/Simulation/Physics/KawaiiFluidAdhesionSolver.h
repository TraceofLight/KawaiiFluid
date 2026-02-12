// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/KawaiiFluidParticle.h"

class UKawaiiFluidCollider;

/**
 * @class FKawaiiFluidAdhesionSolver
 * @brief Solver for fluid particle adhesion to surfaces and inter-particle cohesion.
 *
 * Implements adhesion forces based on Akinci et al. 2013 "Versatile Surface Tension and Adhesion for SPH Fluids".
 * Handles attraction to boundary surfaces (characters, walls) and manages particle attachment states.
 */
class KAWAIIFLUIDRUNTIME_API FKawaiiFluidAdhesionSolver
{
public:
	FKawaiiFluidAdhesionSolver();

	void Apply(
		TArray<FKawaiiFluidParticle>& Particles,
		const TArray<TObjectPtr<UKawaiiFluidCollider>>& Colliders,
		float AdhesionStrength,
		float AdhesionRadius,
		float DetachThreshold,
		float ColliderContactOffset
	);

	void ApplyCohesion(
		TArray<FKawaiiFluidParticle>& Particles,
		float CohesionStrength,
		float SmoothingRadius
	);

private:
	FVector ComputeAdhesionForce(
		const FVector& ParticlePos,
		const FVector& SurfacePoint,
		const FVector& SurfaceNormal,
		float Distance,
		float AdhesionStrength,
		float AdhesionRadius
	);

	void UpdateAttachmentState(
		FKawaiiFluidParticle& Particle,
		AActor* Collider,
		float Force,
		float DetachThreshold,
		FName BoneName,
		const FTransform& BoneTransform,
		const FVector& ParticlePosition,
		const FVector& SurfaceNormal
	);
};
