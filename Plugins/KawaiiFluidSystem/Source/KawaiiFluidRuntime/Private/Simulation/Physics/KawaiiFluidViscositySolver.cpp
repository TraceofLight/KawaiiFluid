// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "Simulation/Physics/KawaiiFluidViscositySolver.h"
#include "Simulation/Physics/KawaiiFluidSPHKernels.h"
#include "Async/ParallelFor.h"

/** Constants for unit conversion within the viscosity solver */
namespace ViscosityConstants
{
	constexpr float CM_TO_M = 0.01f;
	constexpr float CM_TO_M_SQ = CM_TO_M * CM_TO_M;
}

/**
 * @brief Default constructor for FKawaiiFluidViscositySolver.
 */
FKawaiiFluidViscositySolver::FKawaiiFluidViscositySolver()
{
}

/**
 * @brief Apply XSPH viscosity smoothing to the particle system.
 * @param Particles Particle array to modify.
 * @param ViscosityCoeff Viscosity coefficient (0.0 to 1.0).
 * @param SmoothingRadius Kernel interaction radius.
 * 
 * Formula: v_i = v_i + c * Σ(v_j - v_i) * W(r_ij, h)
 */
void FKawaiiFluidViscositySolver::ApplyXSPH(TArray<FKawaiiFluidParticle>& Particles, float ViscosityCoeff, float SmoothingRadius)
{
	if (ViscosityCoeff <= 0.0f)
	{
		return;
	}

	const int32 ParticleCount = Particles.Num();
	if (ParticleCount == 0)
	{
		return;
	}

	SPHKernels::FKernelCoefficients KernelCoeffs;
	KernelCoeffs.Precompute(SmoothingRadius);

	const float RadiusSquared = SmoothingRadius * SmoothingRadius;

	TArray<FVector> NewVelocities;
	NewVelocities.SetNum(ParticleCount);

	ParallelFor(ParticleCount, [&](int32 i)
	{
		const FKawaiiFluidParticle& Particle = Particles[i];
		FVector VelocityCorrection = FVector::ZeroVector;
		float WeightSum = 0.0f;

		for (int32 NeighborIdx : Particle.NeighborIndices)
		{
			if (NeighborIdx == i)
			{
				continue;
			}

			const FKawaiiFluidParticle& Neighbor = Particles[NeighborIdx];
			const FVector r = Particle.Position - Neighbor.Position;

			const float rSquared = r.SizeSquared();
			if (rSquared > RadiusSquared)
			{
				continue;
			}

			const float h2_m = KernelCoeffs.h2;
			const float r2_m = rSquared * ViscosityConstants::CM_TO_M_SQ;
			const float diff = h2_m - r2_m;
			const float Weight = (diff > 0.0f) ? KernelCoeffs.Poly6Coeff * diff * diff * diff : 0.0f;

			const FVector VelocityDiff = Neighbor.Velocity - Particle.Velocity;

			VelocityCorrection += VelocityDiff * Weight;
			WeightSum += Weight;
		}

		if (WeightSum > 0.0f)
		{
			VelocityCorrection /= WeightSum;
		}

		NewVelocities[i] = Particle.Velocity + ViscosityCoeff * VelocityCorrection;

	}, EParallelForFlags::Unbalanced);

	for (int32 i = 0; i < ParticleCount; ++i)
	{
		Particles[i].Velocity = NewVelocities[i];
	}
}
