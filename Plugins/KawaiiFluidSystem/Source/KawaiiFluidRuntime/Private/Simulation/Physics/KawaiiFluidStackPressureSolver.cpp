// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "Simulation/Physics/KawaiiFluidStackPressureSolver.h"
#include "Simulation/Physics/KawaiiFluidSPHKernels.h"
#include "Async/ParallelFor.h"

/**
 * @brief Default constructor for FKawaiiFluidStackPressureSolver.
 */
FKawaiiFluidStackPressureSolver::FKawaiiFluidStackPressureSolver()
{
}

/**
 * @brief Apply stack pressure forces to attached particles.
 * @param Particles Array of fluid particles.
 * @param Gravity World gravity vector (cm/s²).
 * @param StackPressureScale Global multiplier for the weight transfer effect.
 * @param SmoothingRadius Radius for identifying neighboring stacked particles.
 * @param DeltaTime Simulation time step.
 */
void FKawaiiFluidStackPressureSolver::Apply(
	TArray<FKawaiiFluidParticle>& Particles,
	const FVector& Gravity,
	float StackPressureScale,
	float SmoothingRadius,
	float DeltaTime)
{
	if (StackPressureScale <= 0.0f || Particles.Num() == 0 || DeltaTime <= 0.0f)
	{
		return;
	}

	const float RadiusSq = SmoothingRadius * SmoothingRadius;
	const int32 ParticleCount = Particles.Num();

	TArray<FVector> StackForces;
	StackForces.SetNumZeroed(ParticleCount);

	ParallelFor(ParticleCount, [&](int32 i)
	{
		FKawaiiFluidParticle& Particle = Particles[i];

		if (!Particle.bIsAttached)
		{
			return;
		}

		const FVector& SurfaceNormal = Particle.AttachedSurfaceNormal;

		float NormalComponent = FVector::DotProduct(Gravity, SurfaceNormal);
		FVector TangentGravity = Gravity - NormalComponent * SurfaceNormal;

		float TangentMag = TangentGravity.Size();

		if (TangentMag < 0.1f)
		{
			return;
		}

		FVector TangentDir = TangentGravity / TangentMag;
		FVector UpDir = -TangentDir;

		float StackWeight = 0.0f;

		for (int32 NeighborIdx : Particle.NeighborIndices)
		{
			if (NeighborIdx == i || NeighborIdx < 0 || NeighborIdx >= ParticleCount)
			{
				continue;
			}

			const FKawaiiFluidParticle& Neighbor = Particles[NeighborIdx];

			if (!Neighbor.bIsAttached)
			{
				continue;
			}

			if (Neighbor.AttachedActor != Particle.AttachedActor)
			{
				continue;
			}

			FVector ToNeighbor = Neighbor.Position - Particle.Position;
			float DistSq = ToNeighbor.SizeSquared();

			if (DistSq > RadiusSq || DistSq < KINDA_SMALL_NUMBER)
			{
				continue;
			}

			float HeightDiff = FVector::DotProduct(ToNeighbor, UpDir);

			if (HeightDiff > 0.0f)
			{
				float Dist = FMath::Sqrt(DistSq);
				float KernelWeight = SPHKernels::Poly6(Dist, SmoothingRadius);
				float HeightFactor = HeightDiff / Dist;
				StackWeight += Neighbor.Mass * KernelWeight * HeightFactor;
			}
		}

		if (StackWeight > 0.0f)
		{
			StackForces[i] = TangentDir * StackWeight * StackPressureScale;
		}

	}, EParallelForFlags::Unbalanced);

	ParallelFor(ParticleCount, [&](int32 i)
	{
		if (Particles[i].bIsAttached && !StackForces[i].IsNearlyZero())
		{
			Particles[i].Velocity += StackForces[i] * DeltaTime;
		}
	});
}

/**
 * @brief Calculate the height difference between two particles relative to the sliding direction.
 * @param ParticleI The particle receiving weight.
 * @param ParticleJ The neighbor particle potentially providing weight.
 * @param TangentGravityDir Normalized direction of sliding movement.
 * @return Height difference value (positive if J is above I).
 */
float FKawaiiFluidStackPressureSolver::GetHeightDifference(
	const FKawaiiFluidParticle& ParticleI,
	const FKawaiiFluidParticle& ParticleJ,
	const FVector& TangentGravityDir) const
{
	FVector UpDir = -TangentGravityDir;
	FVector ToNeighbor = ParticleJ.Position - ParticleI.Position;

	return FVector::DotProduct(ToNeighbor, UpDir);
}
