// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/KawaiiFluidParticle.h"

/**
 * @class FKawaiiFluidViscositySolver
 * @brief Solver for fluid viscosity effects using the XSPH velocity smoothing method.
 * 
 * Viscosity represents internal friction within the fluid, where particle velocities are 
 * averaged with their neighbors to simulate cohesive movement.
 */
class KAWAIIFLUIDRUNTIME_API FKawaiiFluidViscositySolver
{
public:
	FKawaiiFluidViscositySolver();

	void ApplyXSPH(TArray<FKawaiiFluidParticle>& Particles, float ViscosityCoeff, float SmoothingRadius);

};
