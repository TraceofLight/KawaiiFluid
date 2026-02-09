// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * @brief Collection of SPH kernel functions used for density, pressure, and secondary forces.
 */
namespace SPHKernels
{
	KAWAIIFLUIDRUNTIME_API float Poly6Coefficient(float h);

	KAWAIIFLUIDRUNTIME_API float Poly6(float r, float h);

	KAWAIIFLUIDRUNTIME_API float Poly6(const FVector& r, float h);

	KAWAIIFLUIDRUNTIME_API float SpikyGradientCoefficient(float h);

	KAWAIIFLUIDRUNTIME_API FVector SpikyGradient(const FVector& r, float h);

	KAWAIIFLUIDRUNTIME_API float ViscosityLaplacianCoefficient(float h);

	KAWAIIFLUIDRUNTIME_API float ViscosityLaplacian(float r, float h);

	KAWAIIFLUIDRUNTIME_API float Adhesion(float r, float h);

	KAWAIIFLUIDRUNTIME_API float Cohesion(float r, float h);

	/**
	 * @struct FKernelCoefficients
	 * @brief Precomputed kernel coefficients to optimize runtime performance by eliminating redundant calculations.
	 * 
	 * @param Poly6Coeff Precomputed coefficient for the Poly6 kernel.
	 * @param SpikyGradCoeff Precomputed coefficient for the Spiky gradient kernel.
	 * @param ViscosityLapCoeff Precomputed coefficient for the Viscosity Laplacian kernel.
	 * @param h Interaction radius in meters.
	 * @param h2 Squared interaction radius (h²).
	 * @param h6 Sixth power of interaction radius (h⁶).
	 * @param h9 Ninth power of interaction radius (h⁹).
	 */
	struct FKernelCoefficients
	{
		float Poly6Coeff;
		float SpikyGradCoeff;
		float ViscosityLapCoeff;
		float h;
		float h2;
		float h6;
		float h9;

		FKernelCoefficients() : Poly6Coeff(0), SpikyGradCoeff(0), ViscosityLapCoeff(0), h(0), h2(0), h6(0), h9(0) {}
		void Precompute(float SmoothingRadius);
	};
}
