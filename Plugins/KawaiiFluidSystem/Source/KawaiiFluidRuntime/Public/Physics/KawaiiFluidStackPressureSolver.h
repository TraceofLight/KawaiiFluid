// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/KawaiiFluidParticle.h"

/**
 * @class FKawaiiFluidStackPressureSolver
 * @brief Solver that applies weight transfer from stacked attached particles to simulate realistic dripping behavior.
 * 
 * When particles are attached to a surface and stacked, higher particles transfer their weight 
 * to those below them, increasing the sliding speed of the lower particles.
 */
class KAWAIIFLUIDRUNTIME_API FKawaiiFluidStackPressureSolver
{
public:
	FKawaiiFluidStackPressureSolver();

	void Apply(
		TArray<FKawaiiFluidParticle>& Particles,
		const FVector& Gravity,
		float StackPressureScale,
		float SmoothingRadius,
		float DeltaTime
	);

private:
	float GetHeightDifference(
		const FKawaiiFluidParticle& ParticleI,
		const FKawaiiFluidParticle& ParticleJ,
		const FVector& TangentGravityDir
	) const;
};
