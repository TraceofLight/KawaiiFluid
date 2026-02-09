// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "KawaiiFluidTestMetrics.generated.h"

/**
 * @struct FKawaiiFluidTestMetrics
 * @brief Snapshot of simulation metrics used for automated testing and validation.
 * 
 * Based on Position Based Fluids (Macklin & Müller, 2013) expected behaviors.
 * 
 * @param AverageDensity Average density of all particles (kg/m³).
 * @param MaxDensity Maximum density recorded (kg/m³).
 * @param MinDensity Minimum density recorded (kg/m³).
 * @param DensityStdDev Standard deviation of density across all particles.
 * @param DensityVariance Variance of density values.
 * @param DensityRatio Average density relative to RestDensity (1.0 = ideal).
 * @param CenterOfMass Calculated world-space center of mass (cm).
 * @param TotalVolume Estimated total volume of the fluid system (cm³).
 * @param ParticleBounds Axis-Aligned Bounding Box containing all particles.
 * @param TotalMass Total mass of the simulation (kg).
 * @param ParticlesOutOfBounds Count of particles that have escaped simulation boundaries.
 * @param InvalidParticles Count of particles with NaN or Infinite states.
 * @param MaxVelocity Maximum velocity magnitude found in the system (cm/s).
 * @param AverageVelocity Mean velocity magnitude (cm/s).
 * @param MaxAcceleration Peak acceleration experienced by any particle (cm/s²).
 * @param AverageLambda Mean Lagrange multiplier value.
 * @param MaxLambda Maximum absolute Lagrange multiplier value.
 * @param AverageConstraintError Mean absolute constraint violation |C_i|.
 * @param MaxConstraintError Peak constraint violation.
 * @param SolverIterations Number of iterations performed by the PBF solver.
 * @param AverageNeighborCount Mean number of neighbors per particle.
 * @param MaxNeighborCount Highest neighbor count recorded.
 * @param MinNeighborCount Lowest neighbor count (excluding isolated particles).
 * @param IsolatedParticleCount Number of particles with zero neighbors.
 * @param SimulationTimeMs Total processing time for the simulation frame (ms).
 * @param NeighborSearchTimeMs Time spent on spatial partitioning and neighbor lookup (ms).
 * @param DensitySolveTimeMs Time spent in the XPBD density solver pass (ms).
 * @param ParticleCount Number of particles included in the metrics.
 * @param FrameNumber The frame index at which these metrics were captured.
 * @param SimulationElapsedTime Total simulation time elapsed in seconds.
 */
USTRUCT(BlueprintType)
struct KAWAIIFLUIDRUNTIME_API FKawaiiFluidTestMetrics
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Density")
	float AverageDensity = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Density")
	float MaxDensity = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Density")
	float MinDensity = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Density")
	float DensityStdDev = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Density")
	float DensityVariance = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Density")
	float DensityRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Conservation")
	FVector CenterOfMass = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Conservation")
	float TotalVolume = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Conservation")
	FBox ParticleBounds = FBox(EForceInit::ForceInit);

	UPROPERTY(BlueprintReadOnly, Category = "Conservation")
	float TotalMass = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Stability")
	int32 ParticlesOutOfBounds = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stability")
	int32 InvalidParticles = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stability")
	float MaxVelocity = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Stability")
	float AverageVelocity = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Stability")
	float MaxAcceleration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Solver")
	float AverageLambda = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Solver")
	float MaxLambda = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Solver")
	float AverageConstraintError = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Solver")
	float MaxConstraintError = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Solver")
	int32 SolverIterations = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Neighbors")
	float AverageNeighborCount = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Neighbors")
	int32 MaxNeighborCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Neighbors")
	int32 MinNeighborCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Neighbors")
	int32 IsolatedParticleCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	float SimulationTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	float NeighborSearchTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	float DensitySolveTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	int32 ParticleCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	int32 FrameNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	float SimulationElapsedTime = 0.0f;

	void Reset()
	{
		*this = FKawaiiFluidTestMetrics();
	}

	bool IsDensityStable(float RestDensity, float TolerancePercent = 10.0f) const
	{
		const float LowerBound = RestDensity * (1.0f - TolerancePercent / 100.0f);
		const float UpperBound = RestDensity * (1.0f + TolerancePercent / 100.0f);
		return AverageDensity >= LowerBound && AverageDensity <= UpperBound;
	}

	bool IsNumericallyStable() const
	{
		return InvalidParticles == 0 &&
		       FMath::IsFinite(MaxVelocity) &&
		       MaxVelocity < 100000.0f;
	}

	bool IsVolumeConserved(float InitialVolume, float TolerancePercent = 20.0f) const
	{
		if (InitialVolume <= 0.0f) return false;
		const float Ratio = TotalVolume / InitialVolume;
		return Ratio >= (1.0f - TolerancePercent / 100.0f) &&
		       Ratio <= (1.0f + TolerancePercent / 100.0f);
	}

	FString GetSummary() const
	{
		return FString::Printf(
			TEXT("Particles: %d | Density: %.1f (±%.1f) | MaxVel: %.1f cm/s | Lambda: %.4f | Time: %.2fms"),
			ParticleCount,
			AverageDensity,
			DensityStdDev,
			MaxVelocity,
			AverageLambda,
			SimulationTimeMs
		);
	}
};

/**
 * @struct FFluidTestMetricsHistory
 * @brief Stores a time-series of metrics for trend analysis and stabilization checking.
 * 
 * @param MaxSamples Maximum number of historical frames to store.
 * @param Samples Array of metric snapshots.
 */
USTRUCT(BlueprintType)
struct KAWAIIFLUIDRUNTIME_API FFluidTestMetricsHistory
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
	int32 MaxSamples = 300;

	UPROPERTY(BlueprintReadOnly, Category = "History")
	TArray<FKawaiiFluidTestMetrics> Samples;

	void AddSample(const FKawaiiFluidTestMetrics& Metrics)
	{
		if (Samples.Num() >= MaxSamples)
		{
			Samples.RemoveAt(0);
		}
		Samples.Add(Metrics);
	}

	float GetAverageDensityOverTime() const
	{
		if (Samples.Num() == 0) return 0.0f;
		float Sum = 0.0f;
		for (const auto& S : Samples) Sum += S.AverageDensity;
		return Sum / Samples.Num();
	}

	float GetMaxVelocityEver() const
	{
		float Max = 0.0f;
		for (const auto& S : Samples)
		{
			Max = FMath::Max(Max, S.MaxVelocity);
		}
		return Max;
	}

	bool HasDensityStabilized(int32 RecentSampleCount = 60, float VarianceThreshold = 10.0f) const
	{
		if (Samples.Num() < RecentSampleCount) return false;

		float Sum = 0.0f;
		const int32 StartIdx = Samples.Num() - RecentSampleCount;
		for (int32 i = StartIdx; i < Samples.Num(); ++i)
		{
			Sum += Samples[i].AverageDensity;
		}
		const float Mean = Sum / RecentSampleCount;

		float VarianceSum = 0.0f;
		for (int32 i = StartIdx; i < Samples.Num(); ++i)
		{
			const float Diff = Samples[i].AverageDensity - Mean;
			VarianceSum += Diff * Diff;
		}
		const float Variance = VarianceSum / RecentSampleCount;

		return Variance < VarianceThreshold;
	}

	void Clear()
	{
		Samples.Empty();
	}
};
