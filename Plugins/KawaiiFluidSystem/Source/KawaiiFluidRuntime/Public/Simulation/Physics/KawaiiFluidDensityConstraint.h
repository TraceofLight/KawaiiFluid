// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/KawaiiFluidParticle.h"

/**
 * @struct FTensileInstabilityParams
 * @brief Parameters for correcting tensile instability using artificial pressure (PBF Eq.13-14).
 * 
 * s_corr = -k * (W(r)/W(Δq))^n. This correction prevents particle clustering at surface or splash regions.
 * 
 * @param bEnabled Toggle for enabling the scorr correction term.
 * @param K Strength coefficient for the correction force (default 0.1).
 * @param N Sharpness exponent controlling the falloff (default 4).
 * @param DeltaQ Reference distance ratio (Δq/h) relative to smoothing radius.
 * @param W_DeltaQ Precomputed kernel value W(Δq, h) for runtime efficiency.
 */
struct FTensileInstabilityParams
{
	bool bEnabled = false;
	float K = 0.1f;
	int32 N = 4;
	float DeltaQ = 0.2f;
	float W_DeltaQ = 0.0f;
};

/**
 * @struct FSPHKernelCoeffs
 * @brief Precomputed SPH kernel coefficients used to eliminate expensive Pow() and division calls.
 * 
 * @param h Interaction kernel radius in meters.
 * @param h2 Squared kernel radius (h²).
 * @param Poly6Coeff Precomputed normalization constant 315 / (64πh⁹).
 * @param SpikyCoeff Precomputed normalization constant -45 / (πh⁶).
 * @param InvRestDensity Reciprocal of the target rest density (1 / ρ₀).
 * @param SmoothingRadiusSq Squared kernel radius in cm².
 * @param TensileParams Copied instability correction parameters for the current pass.
 */
struct FSPHKernelCoeffs
{
	float h;
	float h2;
	float Poly6Coeff;
	float SpikyCoeff;
	float InvRestDensity;
	float SmoothingRadiusSq;

	FTensileInstabilityParams TensileParams;
};

/**
 * @class FKawaiiFluidDensityConstraint
 * @brief Solver for enforcing fluid incompressibility using Position-Based Fluids (PBF) constraints.
 *
 * Enforces the density constraint: C_i = (ρ_i / ρ_0) - 1 = 0 by iteratively correcting particle positions.
 * 
 * @param RestDensity Target rest density of the fluid (kg/m³).
 * @param Epsilon Stability constant / XPBD compliance factor (α̃ = α / dt²).
 * @param SmoothingRadius Effective kernel radius in centimeters.
 * @param PosX Array of particle X coordinates (Structure of Arrays format).
 * @param PosY Array of particle Y coordinates (SoA format).
 * @param PosZ Array of particle Z coordinates (SoA format).
 * @param Masses Array of particle masses (SoA format).
 * @param Densities Array of calculated particle densities (SoA format).
 * @param Lambdas Array of Lagrange multipliers for constraint solving (SoA format).
 * @param DeltaPX Array of calculated position X corrections (SoA format).
 * @param DeltaPY Array of calculated position Y corrections (SoA format).
 * @param DeltaPZ Array of calculated position Z corrections (SoA format).
 */
class KAWAIIFLUIDRUNTIME_API FKawaiiFluidDensityConstraint
{
public:
	FKawaiiFluidDensityConstraint();
	FKawaiiFluidDensityConstraint(float InRestDensity, float InSmoothingRadius, float InEpsilon);

	void Solve(TArray<FKawaiiFluidParticle>& Particles, float InSmoothingRadius, float InRestDensity, float InCompliance, float DeltaTime);

	void SolveWithTensileCorrection(
		TArray<FKawaiiFluidParticle>& Particles,
		float InSmoothingRadius,
		float InRestDensity,
		float InCompliance,
		float DeltaTime,
		const FTensileInstabilityParams& TensileParams);

	void SetRestDensity(float NewRestDensity);
	void SetEpsilon(float NewEpsilon);

private:
	float RestDensity;
	float Epsilon;
	float SmoothingRadius;

	TArray<float> PosX, PosY, PosZ;
	TArray<float> Masses;
	TArray<float> Densities;
	TArray<float> Lambdas;
	TArray<float> DeltaPX, DeltaPY, DeltaPZ;

	void ResizeSoAArrays(int32 NumParticles);
	void CopyToSoA(const TArray<FKawaiiFluidParticle>& Particles);
	void ApplyFromSoA(TArray<FKawaiiFluidParticle>& Particles);

	void ComputeDensityAndLambda_SIMD(
		const TArray<FKawaiiFluidParticle>& Particles,
		const FSPHKernelCoeffs& Coeffs);

	void ComputeDeltaP_SIMD(
		const TArray<FKawaiiFluidParticle>& Particles,
		const FSPHKernelCoeffs& Coeffs);

	//========================================
	// Legacy Functions
	//========================================
	void ComputeDensities(TArray<FKawaiiFluidParticle>& Particles);
	void ComputeLambdas(TArray<FKawaiiFluidParticle>& Particles);
	void ApplyPositionCorrection(TArray<FKawaiiFluidParticle>& Particles);
	float ComputeParticleDensity(const FKawaiiFluidParticle& Particle, const TArray<FKawaiiFluidParticle>& Particles);
	float ComputeParticleLambda(const FKawaiiFluidParticle& Particle, const TArray<FKawaiiFluidParticle>& Particles);
	FVector ComputeDeltaPosition(int32 ParticleIndex, const TArray<FKawaiiFluidParticle>& Particles);
};
