// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tests/KawaiiFluidTestMetrics.h"
#include "Core/KawaiiFluidParticle.h"

class UKawaiiFluidSimulationModule;

/**
 * @class FKawaiiFluidMetricsCollector
 * @brief Utility class for collecting simulation metrics from particle data for validation.
 */
class KAWAIIFLUIDRUNTIME_API FKawaiiFluidMetricsCollector
{
public:
	static FKawaiiFluidTestMetrics CollectFromParticles(
		const TArray<FKawaiiFluidParticle>& Particles,
		float RestDensity,
		const FBox& SimulationBounds = FBox(FVector(-1e10f), FVector(1e10f)));

	static FKawaiiFluidTestMetrics CollectFromModule(const UKawaiiFluidSimulationModule* Module);

	static float CalculateConstraintError(float Density, float RestDensity);

	static float CalculateAverageConstraintError(
		const TArray<FKawaiiFluidParticle>& Particles,
		float RestDensity);

	static float CalculateMaxConstraintError(
		const TArray<FKawaiiFluidParticle>& Particles,
		float RestDensity);

	static bool IsInEquilibrium(
		const FFluidTestMetricsHistory& History,
		float VelocityThreshold = 10.0f,
		float DensityVarianceThreshold = 5.0f,
		int32 MinSamples = 60);
};
