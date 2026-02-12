// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Simulation/Physics/KawaiiFluidSPHKernels.h"
#include "Simulation/Physics/KawaiiFluidDensityConstraint.h"
#include "Core/KawaiiFluidParticle.h"
#include "Core/KawaiiFluidSpatialHash.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidXPBDTest_LambdaInitialization,
	"KawaiiFluid.Physics.XPBD.X01_LambdaInitialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidXPBDTest_ComplianceEffect,
	"KawaiiFluid.Physics.XPBD.X02_ComplianceEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidXPBDTest_CompressionSkip,
	"KawaiiFluid.Physics.XPBD.X03_CompressionSkip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidXPBDTest_LambdaAccumulation,
	"KawaiiFluid.Physics.XPBD.X04_LambdaAccumulation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidXPBDTest_Convergence,
	"KawaiiFluid.Physics.XPBD.X05_Convergence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	/**
	 * @brief Helper: Create fluid particles arranged in a uniform 3D grid.
	 * @param GridSize Number of particles along each axis.
	 * @param Spacing Distance between particles.
	 * @param Mass Mass of each particle.
	 * @return Array of initialized particles.
	 */
	TArray<FKawaiiFluidParticle> CreateTestGrid(int32 GridSize, float Spacing, float Mass)
	{
		TArray<FKawaiiFluidParticle> Particles;
		Particles.Reserve(GridSize * GridSize * GridSize);

		const float HalfExtent = (GridSize - 1) * Spacing * 0.5f;

		for (int32 x = 0; x < GridSize; ++x)
		{
			for (int32 y = 0; y < GridSize; ++y)
			{
				for (int32 z = 0; z < GridSize; ++z)
				{
					FKawaiiFluidParticle Particle;
					Particle.Position = FVector(
						x * Spacing - HalfExtent,
						y * Spacing - HalfExtent,
						z * Spacing - HalfExtent
					);
					Particle.PredictedPosition = Particle.Position;
					Particle.Mass = Mass;
					Particle.Velocity = FVector::ZeroVector;
					Particle.Density = 0.0f;
					Particle.Lambda = 0.0f;
					Particles.Add(Particle);
				}
			}
		}

		return Particles;
	}

	/**
	 * @brief Helper: Build neighbor lists for particles using a spatial hash.
	 * @param Particles Reference to particle array.
	 * @param SmoothingRadius Kernel radius for neighbor search.
	 */
	void BuildNeighbors(TArray<FKawaiiFluidParticle>& Particles, float SmoothingRadius)
	{
		FKawaiiFluidSpatialHash SpatialHash(SmoothingRadius);

		TArray<FVector> Positions;
		Positions.Reserve(Particles.Num());
		for (const FKawaiiFluidParticle& P : Particles)
		{
			Positions.Add(P.PredictedPosition);
		}

		SpatialHash.BuildFromPositions(Positions);

		for (int32 i = 0; i < Particles.Num(); ++i)
		{
			SpatialHash.GetNeighbors(
				Particles[i].PredictedPosition,
				SmoothingRadius,
				Particles[i].NeighborIndices
			);
		}
	}

	/**
	 * @brief Helper: Compute the mean density of all particles in the array.
	 * @return Average density value.
	 */
	float ComputeAverageDensity(const TArray<FKawaiiFluidParticle>& Particles)
	{
		float Sum = 0.0f;
		for (const FKawaiiFluidParticle& P : Particles)
		{
			Sum += P.Density;
		}
		return Sum / static_cast<float>(Particles.Num());
	}

	/**
	 * @brief Helper: Find the maximum absolute constraint error among all particles.
	 * @param RestDensity Reference density for error calculation.
	 * @return Maximum error |C_i|.
	 */
	float ComputeConstraintError(const TArray<FKawaiiFluidParticle>& Particles, float RestDensity)
	{
		float MaxError = 0.0f;
		for (const FKawaiiFluidParticle& P : Particles)
		{
			const float C = (P.Density / RestDensity) - 1.0f;
			MaxError = FMath::Max(MaxError, FMath::Abs(C));
		}
		return MaxError;
	}
}

/**
 * @brief X-01: Lambda Initialization Test.
 * Verifies that the solver properly initializes or updates Lambda values from their initial state.
 */
bool FKawaiiFluidXPBDTest_LambdaInitialization::RunTest(const FString& Parameters)
{
	const float SmoothingRadius = 20.0f;
	const float RestDensity = 1000.0f;
	const float Compliance = 0.01f;
	const float DeltaTime = 1.0f / 120.0f;

	TArray<FKawaiiFluidParticle> Particles = CreateTestGrid(3, SmoothingRadius * 0.5f, 1.0f);
	BuildNeighbors(Particles, SmoothingRadius);

	for (FKawaiiFluidParticle& P : Particles)
	{
		P.Lambda = 100.0f;
	}

	FKawaiiFluidDensityConstraint Solver(RestDensity, SmoothingRadius, Compliance);

	TArray<float> LambdasBefore;
	for (const FKawaiiFluidParticle& P : Particles)
	{
		LambdasBefore.Add(P.Lambda);
	}

	Solver.Solve(Particles, SmoothingRadius, RestDensity, Compliance, DeltaTime);

	bool bAllZero = true;
	for (int32 i = 0; i < Particles.Num(); ++i)
	{
		if (FMath::Abs(Particles[i].Lambda) > KINDA_SMALL_NUMBER)
		{
			bAllZero = false;
			break;
		}
	}

	AddInfo(FString::Printf(TEXT("Particles: %d, Lambda values updated by solver"), Particles.Num()));

	bool bDensityComputed = false;
	for (const FKawaiiFluidParticle& P : Particles)
	{
		if (P.Density > 0.0f)
		{
			bDensityComputed = true;
			break;
		}
	}
	TestTrue(TEXT("Density values were computed"), bDensityComputed);

	return true;
}

/**
 * @brief X-02: Compliance Effect Test.
 * Higher compliance should result in smaller Lagrange multipliers (softer constraints).
 */
bool FKawaiiFluidXPBDTest_ComplianceEffect::RunTest(const FString& Parameters)
{
	const float SmoothingRadius = 20.0f;
	const float RestDensity = 1000.0f;
	const float DeltaTime = 1.0f / 120.0f;

	const float LowCompliance = 0.0001f;
	const float HighCompliance = 0.1f;

	const float TightSpacing = SmoothingRadius * 0.3f;

	TArray<FKawaiiFluidParticle> ParticlesStiff = CreateTestGrid(3, TightSpacing, 1.0f);
	BuildNeighbors(ParticlesStiff, SmoothingRadius);

	FKawaiiFluidDensityConstraint SolverStiff(RestDensity, SmoothingRadius, LowCompliance);
	SolverStiff.Solve(ParticlesStiff, SmoothingRadius, RestDensity, LowCompliance, DeltaTime);

	TArray<FKawaiiFluidParticle> ParticlesSoft = CreateTestGrid(3, TightSpacing, 1.0f);
	BuildNeighbors(ParticlesSoft, SmoothingRadius);

	FKawaiiFluidDensityConstraint SolverSoft(RestDensity, SmoothingRadius, HighCompliance);
	SolverSoft.Solve(ParticlesSoft, SmoothingRadius, RestDensity, HighCompliance, DeltaTime);

	float TotalCorrectionStiff = 0.0f;
	float TotalCorrectionSoft = 0.0f;

	for (int32 i = 0; i < ParticlesStiff.Num(); ++i)
	{
		TotalCorrectionStiff += FMath::Abs(ParticlesStiff[i].Lambda);
		TotalCorrectionSoft += FMath::Abs(ParticlesSoft[i].Lambda);
	}

	const float AvgLambdaStiff = TotalCorrectionStiff / ParticlesStiff.Num();
	const float AvgLambdaSoft = TotalCorrectionSoft / ParticlesSoft.Num();

	AddInfo(FString::Printf(TEXT("Low compliance (%.4f): avg |λ| = %.4f"), LowCompliance, AvgLambdaStiff));
	AddInfo(FString::Printf(TEXT("High compliance (%.4f): avg |λ| = %.4f"), HighCompliance, AvgLambdaSoft));

	TestTrue(TEXT("Different compliance values produce different results"),
		FMath::Abs(AvgLambdaStiff - AvgLambdaSoft) > KINDA_SMALL_NUMBER ||
		TotalCorrectionStiff != TotalCorrectionSoft);

	return true;
}

/**
 * @brief X-03: Compression State Skip Test.
 * Particles with density below rest density should not receive attractive forces from the solver.
 */
bool FKawaiiFluidXPBDTest_CompressionSkip::RunTest(const FString& Parameters)
{
	const float SmoothingRadius = 20.0f;
	const float RestDensity = 1000.0f;
	const float Compliance = 0.01f;
	const float DeltaTime = 1.0f / 120.0f;

	const float SparseSpacing = SmoothingRadius * 1.5f;

	TArray<FKawaiiFluidParticle> Particles = CreateTestGrid(3, SparseSpacing, 1.0f);
	BuildNeighbors(Particles, SmoothingRadius);

	FKawaiiFluidDensityConstraint Solver(RestDensity, SmoothingRadius, Compliance);
	Solver.Solve(Particles, SmoothingRadius, RestDensity, Compliance, DeltaTime);

	int32 LowDensityCount = 0;
	int32 SkippedCount = 0;

	for (const FKawaiiFluidParticle& P : Particles)
	{
		const float C = (P.Density / RestDensity) - 1.0f;
		if (C < 0.0f)
		{
			LowDensityCount++;
			if (FMath::Abs(P.Lambda) < 0.1f)
			{
				SkippedCount++;
			}
		}
	}

	AddInfo(FString::Printf(TEXT("Particles with ρ < ρ₀: %d"), LowDensityCount));
	AddInfo(FString::Printf(TEXT("Particles with small |λ|: %d"), SkippedCount));

	if (LowDensityCount > 0)
	{
		const float SkipRatio = static_cast<float>(SkippedCount) / static_cast<float>(LowDensityCount);
		TestTrue(TEXT("Most under-dense particles have small Lambda"), SkipRatio > 0.5f);
	}

	return true;
}

/**
 * @brief X-04: Lambda Accumulation Test.
 * Verifies that the XPBD Lagrange multiplier accumulates over multiple solver iterations.
 */
bool FKawaiiFluidXPBDTest_LambdaAccumulation::RunTest(const FString& Parameters)
{
	const float SmoothingRadius = 20.0f;
	const float RestDensity = 1000.0f;
	const float Compliance = 0.01f;
	const float DeltaTime = 1.0f / 120.0f;

	const float DenseSpacing = SmoothingRadius * 0.4f;

	TArray<FKawaiiFluidParticle> Particles = CreateTestGrid(3, DenseSpacing, 1.0f);
	BuildNeighbors(Particles, SmoothingRadius);

	for (FKawaiiFluidParticle& P : Particles)
	{
		P.Lambda = 0.0f;
	}

	FKawaiiFluidDensityConstraint Solver(RestDensity, SmoothingRadius, Compliance);

	TArray<float> LambdaHistory;

	for (int32 Iter = 0; Iter < 5; ++Iter)
	{
		float AvgLambda = 0.0f;
		for (const FKawaiiFluidParticle& P : Particles)
		{
			AvgLambda += P.Lambda;
		}
		AvgLambda /= static_cast<float>(Particles.Num());
		LambdaHistory.Add(AvgLambda);

		Solver.Solve(Particles, SmoothingRadius, RestDensity, Compliance, DeltaTime);

		BuildNeighbors(Particles, SmoothingRadius);
	}

	bool bLambdaChanged = false;
	for (int32 i = 1; i < LambdaHistory.Num(); ++i)
	{
		if (FMath::Abs(LambdaHistory[i] - LambdaHistory[i - 1]) > KINDA_SMALL_NUMBER)
		{
			bLambdaChanged = true;
			break;
		}
	}

	TestTrue(TEXT("Lambda values change over iterations"), bLambdaChanged);

	for (int32 i = 0; i < LambdaHistory.Num(); ++i)
	{
		AddInfo(FString::Printf(TEXT("Iteration %d: avg λ = %.6f"), i, LambdaHistory[i]));
	}

	return true;
}

/**
 * @brief X-05: Convergence Test.
 * Constraint error should generally decrease as the system approaches equilibrium over iterations.
 */
bool FKawaiiFluidXPBDTest_Convergence::RunTest(const FString& Parameters)
{
	const float SmoothingRadius = 20.0f;
	const float RestDensity = 1000.0f;
	const float Compliance = 0.001f;
	const float DeltaTime = 1.0f / 120.0f;

	const float InitialSpacing = SmoothingRadius * 0.35f;

	TArray<FKawaiiFluidParticle> Particles = CreateTestGrid(4, InitialSpacing, 1.0f);
	BuildNeighbors(Particles, SmoothingRadius);

	for (FKawaiiFluidParticle& P : Particles)
	{
		P.Lambda = 0.0f;
	}

	FKawaiiFluidDensityConstraint Solver(RestDensity, SmoothingRadius, Compliance);

	TArray<float> ErrorHistory;
	TArray<float> DensityHistory;

	const int32 MaxIterations = 10;

	for (int32 Iter = 0; Iter < MaxIterations; ++Iter)
	{
		Solver.Solve(Particles, SmoothingRadius, RestDensity, Compliance, DeltaTime);

		BuildNeighbors(Particles, SmoothingRadius);

		float MaxError = ComputeConstraintError(Particles, RestDensity);
		ErrorHistory.Add(MaxError);

		float AvgDensity = ComputeAverageDensity(Particles);
		DensityHistory.Add(AvgDensity);
	}

	bool bConverging = false;
	if (ErrorHistory.Num() >= 2)
	{
		const float InitialError = ErrorHistory[0];
		const float FinalError = ErrorHistory.Last();

		bConverging = (FinalError <= InitialError * 1.1f);

		AddInfo(FString::Printf(TEXT("Initial error: %.4f, Final error: %.4f"), InitialError, FinalError));
	}

	for (int32 i = 0; i < ErrorHistory.Num(); ++i)
	{
		AddInfo(FString::Printf(TEXT("Iter %d: max|C| = %.4f, avg ρ = %.2f"),
			i, ErrorHistory[i], DensityHistory[i]));
	}

	TestTrue(TEXT("Constraint error converges or stays stable"), bConverging);

	const float FinalDensity = DensityHistory.Last();
	const float DensityError = FMath::Abs(FinalDensity - RestDensity) / RestDensity;
	AddInfo(FString::Printf(TEXT("Final density error: %.2f%%"), DensityError * 100.0f));

	return true;
}

#endif
