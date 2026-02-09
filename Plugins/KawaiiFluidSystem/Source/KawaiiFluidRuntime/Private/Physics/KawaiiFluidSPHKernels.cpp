// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "Physics/KawaiiFluidSPHKernels.h"

namespace SPHKernels
{
	/** Constant for converting Unreal units (cm) to meters (m) */
	constexpr float CM_TO_M = 0.01f;

	/**
	 * @brief Calculate the Poly6 kernel coefficient for a given radius.
	 * @param h Interaction radius.
	 * @return Precomputed coefficient value.
	 */
	float Poly6Coefficient(float h)
	{
		return 315.0f / (64.0f * PI * FMath::Pow(h, 9.0f));
	}

	/**
	 * @brief Calculate the Poly6 kernel value for density estimation.
	 * @param r Distance between particles.
	 * @param h Interaction radius.
	 * @return Kernel weight value.
	 */
	float Poly6(float r, float h)
	{
		if (r < 0.0f || r > h)
		{
			return 0.0f;
		}

		float r_m = r * CM_TO_M;
		float h_m = h * CM_TO_M;

		float h2 = h_m * h_m;
		float r_sq = r_m * r_m;
		float diff = h2 - r_sq;

		return Poly6Coefficient(h_m) * diff * diff * diff;
	}

	/**
	 * @brief Calculate the Poly6 kernel value using a displacement vector.
	 * @param r Displacement vector between particles.
	 * @param h Interaction radius.
	 * @return Kernel weight value.
	 */
	float Poly6(const FVector& r, float h)
	{
		return Poly6(r.Size(), h);
	}

	/**
	 * @brief Calculate the Spiky kernel gradient coefficient.
	 * @param h Interaction radius.
	 * @return Precomputed coefficient value.
	 */
	float SpikyGradientCoefficient(float h)
	{
		return -45.0f / (PI * FMath::Pow(h, 6.0f));
	}

	/**
	 * @brief Calculate the Spiky kernel gradient for pressure force calculation.
	 * @param r Displacement vector between particles.
	 * @param h Interaction radius.
	 * @return Gradient vector.
	 */
	FVector SpikyGradient(const FVector& r, float h)
	{
		float rLen = r.Size();

		if (rLen <= 0.0f || rLen > h)
		{
			return FVector::ZeroVector;
		}

		float rLen_m = rLen * CM_TO_M;
		float h_m = h * CM_TO_M;

		float diff = h_m - rLen_m;
		float coeff = SpikyGradientCoefficient(h_m) * diff * diff;

		FVector rNorm = r / rLen;

		return coeff * rNorm * CM_TO_M;
	}

	/**
	 * @brief Calculate the Viscosity kernel Laplacian coefficient.
	 * @param h Interaction radius.
	 * @return Precomputed coefficient value.
	 */
	float ViscosityLaplacianCoefficient(float h)
	{
		return 45.0f / (PI * FMath::Pow(h, 6.0f));
	}

	/**
	 * @brief Calculate the Viscosity kernel Laplacian for velocity smoothing.
	 * @param r Distance between particles.
	 * @param h Interaction radius.
	 * @return Laplacian weight value.
	 */
	float ViscosityLaplacian(float r, float h)
	{
		if (r < 0.0f || r > h)
		{
			return 0.0f;
		}

		float r_m = r * CM_TO_M;
		float h_m = h * CM_TO_M;

		return ViscosityLaplacianCoefficient(h_m) * (h_m - r_m);
	}

	/**
	 * @brief Calculate the adhesion kernel value (Akinci et al. 2013).
	 * @param r Distance to the surface.
	 * @param h Adhesion radius.
	 * @return Kernel weight value.
	 */
	float Adhesion(float r, float h)
	{
		if (r < 0.5f * h || r > h)
		{
			return 0.0f;
		}

		float r_m = r * CM_TO_M;
		float h_m = h * CM_TO_M;

		float coeff = 0.007f / FMath::Pow(h_m, 3.25f);
		float inner = -4.0f * r_m * r_m / h_m + 6.0f * r_m - 2.0f * h_m;

		if (inner <= 0.0f)
		{
			return 0.0f;
		}

		return coeff * FMath::Pow(inner, 0.25f);
	}

	/**
	 * @brief Calculate the cohesion kernel value (Akinci et al. 2013).
	 * @param r Distance between particles.
	 * @param h Interaction radius.
	 * @return Kernel weight value.
	 */
	float Cohesion(float r, float h)
	{
		if (r < 0.0f || r > h)
		{
			return 0.0f;
		}

		float r_m = r * CM_TO_M;
		float h_m = h * CM_TO_M;

		float coeff = 32.0f / (PI * FMath::Pow(h_m, 9.0f));
		float half_h = h_m * 0.5f;

		if (r_m <= half_h)
		{
			float diff1 = h_m - r_m;
			float diff2 = diff1 * diff1 * diff1;
			float r3 = r_m * r_m * r_m;
			return coeff * 2.0f * diff2 * r3 - (FMath::Pow(h_m, 6.0f) / 64.0f);
		}
		else
		{
			float diff = h_m - r_m;
			return coeff * diff * diff * diff * r_m * r_m * r_m;
		}
	}

	/**
	 * @brief Precompute all kernel coefficients for a specific smoothing radius.
	 * @param SmoothingRadius The interaction radius in centimeters.
	 */
	void FKernelCoefficients::Precompute(float SmoothingRadius)
	{
		h = SmoothingRadius * CM_TO_M;
		h2 = h * h;
		h6 = h2 * h2 * h2;
		h9 = h6 * h2 * h;

		Poly6Coeff = 315.0f / (64.0f * PI * h9);
		SpikyGradCoeff = -45.0f / (PI * h6);
		ViscosityLapCoeff = 45.0f / (PI * h6);
	}
}
