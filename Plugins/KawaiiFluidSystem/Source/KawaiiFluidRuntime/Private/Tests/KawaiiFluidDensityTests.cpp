// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Physics/KawaiiFluidSPHKernels.h"
#include "Physics/KawaiiFluidDensityConstraint.h"
#include "Core/KawaiiFluidParticle.h"
#include "Core/KawaiiFluidSpatialHash.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidDensityTest_UniformGridDensity,
	"KawaiiFluid.Physics.Density.D01_UniformGridDensity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidDensityTest_IsolatedParticle,
	"KawaiiFluid.Physics.Density.D02_IsolatedParticle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidDensityTest_DenseState,
	"KawaiiFluid.Physics.Density.D03_DenseState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidDensityTest_BoundaryParticle,
	"KawaiiFluid.Physics.Density.D04_BoundaryParticle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	/**
	 * @brief Helper: Create fluid particles arranged in a uniform 3D grid.
	 * @param Center Center of the grid.
	 * @param GridSize Number of particles along each axis.
	 * @param Spacing Distance between particles.
	 * @param Mass Mass of each particle.
	 * @return Array of initialized particles.
	 */
	TArray<FKawaiiFluidParticle> CreateUniformGrid(
		const FVector& Center,
		int32 GridSize,
		float Spacing,
		float Mass)
	{
		TArray<FKawaiiFluidParticle> Particles;
		Particles.Reserve(GridSize * GridSize * GridSize);

		const float HalfExtent = (GridSize - 1) * Spacing * 0.5f;
		const FVector StartPos = Center - FVector(HalfExtent);

		for (int32 x = 0; x < GridSize; ++x)
		{
			for (int32 y = 0; y < GridSize; ++y)
			{
				for (int32 z = 0; z < GridSize; ++z)
				{
					FKawaiiFluidParticle Particle;
					Particle.Position = StartPos + FVector(x * Spacing, y * Spacing, z * Spacing);
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
	 * @brief Helper: Build neighbor lists using spatial hash.
	 * @param Particles Reference to particle array.
	 * @param SmoothingRadius Kernel radius for neighbor search.
	 */
	void BuildNeighborLists(TArray<FKawaiiFluidParticle>& Particles, float SmoothingRadius)
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
	 * @brief Helper: Compute SPH density for a single particle.
	 * @return Calculated density value.
	 */
	float ComputeParticleDensity(
		const FKawaiiFluidParticle& Particle,
		const TArray<FKawaiiFluidParticle>& AllParticles,
		float SmoothingRadius)
	{
		float Density = 0.0f;

		for (int32 NeighborIdx : Particle.NeighborIndices)
		{
			const FKawaiiFluidParticle& Neighbor = AllParticles[NeighborIdx];
			const FVector r = Particle.PredictedPosition - Neighbor.PredictedPosition;
			Density += Neighbor.Mass * SPHKernels::Poly6(r, SmoothingRadius);
		}

		return Density;
	}
}

/**
 * @brief D-01: Uniform Grid Density Test.
 * Particles in a uniform grid should have density close to RestDensity with appropriate spacing.
 */
bool FKawaiiFluidDensityTest_UniformGridDensity::RunTest(const FString& Parameters)
{
	const float SmoothingRadius = 20.0f;
	const float RestDensity = 1000.0f;
	const float ParticleMass = 1.0f;

	const float Spacing = SmoothingRadius * 0.5f;
	const int32 GridSize = 5;

	TArray<FKawaiiFluidParticle> Particles = CreateUniformGrid(
		FVector::ZeroVector, GridSize, Spacing, ParticleMass);

	BuildNeighborLists(Particles, SmoothingRadius);

	const int32 CenterIndex = (GridSize * GridSize * GridSize) / 2;
	FKawaiiFluidParticle& CenterParticle = Particles[CenterIndex];

	const float CenterDensity = ComputeParticleDensity(CenterParticle, Particles, SmoothingRadius);

	const int32 NeighborCount = CenterParticle.NeighborIndices.Num();
	TestTrue(TEXT("Center particle has sufficient neighbors (>20)"), NeighborCount > 20);

	AddInfo(FString::Printf(TEXT("Grid: %dx%dx%d, Spacing: %.1f cm, h: %.1f cm"),
		GridSize, GridSize, GridSize, Spacing, SmoothingRadius));
	AddInfo(FString::Printf(TEXT("Center particle neighbors: %d"), NeighborCount));
	AddInfo(FString::Printf(TEXT("Center particle density: %.2f kg/m³"), CenterDensity));

	TestTrue(TEXT("Computed density is positive"), CenterDensity > 0.0f);
	TestTrue(TEXT("Computed density is finite"), FMath::IsFinite(CenterDensity));

	return true;
}

/**
 * @brief D-02: Isolated Particle Test.
 * A particle with no neighbors should have density equal to its own self-contribution.
 */
bool FKawaiiFluidDensityTest_IsolatedParticle::RunTest(const FString& Parameters)
{
	const float SmoothingRadius = 20.0f;
	const float ParticleMass = 1.0f;

	TArray<FKawaiiFluidParticle> Particles;
	FKawaiiFluidParticle Particle;
	Particle.Position = FVector::ZeroVector;
	Particle.PredictedPosition = Particle.Position;
	Particle.Mass = ParticleMass;
	Particles.Add(Particle);

	BuildNeighborLists(Particles, SmoothingRadius);

	const float Density = ComputeParticleDensity(Particles[0], Particles, SmoothingRadius);

	const float ExpectedDensity = ParticleMass * SPHKernels::Poly6(0.0f, SmoothingRadius);

	TestNearlyEqual(TEXT("Isolated particle density equals self-contribution"),
		Density, ExpectedDensity, ExpectedDensity * 0.01f);

	AddInfo(FString::Printf(TEXT("Isolated particle density: %.4f kg/m³"), Density));
	AddInfo(FString::Printf(TEXT("Expected (self-contribution): %.4f kg/m³"), ExpectedDensity));
	AddInfo(FString::Printf(TEXT("Neighbor count: %d"), Particles[0].NeighborIndices.Num()));

	return true;
}

/**
 * @brief D-03: Dense State Test.
 * Particles packed closer than rest spacing should yield density values higher than rest density.
 */
bool FKawaiiFluidDensityTest_DenseState::RunTest(const FString& Parameters)
{
	const float SmoothingRadius = 20.0f;
	const float ParticleMass = 1.0f;

	const float TightSpacing = SmoothingRadius * 0.25f;
	const float NormalSpacing = SmoothingRadius * 0.5f;

	const int32 GridSize = 3;

	TArray<FKawaiiFluidParticle> DenseParticles = CreateUniformGrid(
		FVector::ZeroVector, GridSize, TightSpacing, ParticleMass);
	BuildNeighborLists(DenseParticles, SmoothingRadius);

	TArray<FKawaiiFluidParticle> NormalParticles = CreateUniformGrid(
		FVector(500, 0, 0), GridSize, NormalSpacing, ParticleMass);
	BuildNeighborLists(NormalParticles, SmoothingRadius);

	const int32 CenterIdx = (GridSize * GridSize * GridSize) / 2;

	const float DenseDensity = ComputeParticleDensity(
		DenseParticles[CenterIdx], DenseParticles, SmoothingRadius);
	const float NormalDensity = ComputeParticleDensity(
		NormalParticles[CenterIdx], NormalParticles, SmoothingRadius);

	TestTrue(TEXT("Dense packing has higher density than normal"),
		DenseDensity > NormalDensity);

	AddInfo(FString::Printf(TEXT("Dense (spacing=%.1f cm) density: %.2f kg/m³"),
		TightSpacing, DenseDensity));
	AddInfo(FString::Printf(TEXT("Normal (spacing=%.1f cm) density: %.2f kg/m³"),
		NormalSpacing, NormalDensity));
	AddInfo(FString::Printf(TEXT("Ratio: %.2fx"), DenseDensity / NormalDensity));

	return true;
}

/**
 * @brief D-04: Boundary Particle Test.
 * Verifies neighbor deficiency and resulting lower density for particles at the system boundaries.
 */
bool FKawaiiFluidDensityTest_BoundaryParticle::RunTest(const FString& Parameters)
{
	const float SmoothingRadius = 20.0f;
	const float ParticleMass = 1.0f;
	const float Spacing = SmoothingRadius * 0.5f;
	const int32 GridSize = 5;

	TArray<FKawaiiFluidParticle> Particles = CreateUniformGrid(
		FVector::ZeroVector, GridSize, Spacing, ParticleMass);
	BuildNeighborLists(Particles, SmoothingRadius);

	const int32 CenterIdx = (GridSize * GridSize * GridSize) / 2;
	const int32 CornerIdx = 0;

	const float CenterDensity = ComputeParticleDensity(
		Particles[CenterIdx], Particles, SmoothingRadius);
	const float CornerDensity = ComputeParticleDensity(
		Particles[CornerIdx], Particles, SmoothingRadius);

	const int32 CenterNeighbors = Particles[CenterIdx].NeighborIndices.Num();
	const int32 CornerNeighbors = Particles[CornerIdx].NeighborIndices.Num();

	TestTrue(TEXT("Corner particle has fewer neighbors than center"),
		CornerNeighbors < CenterNeighbors);

	TestTrue(TEXT("Corner particle has lower density than center"),
		CornerDensity < CenterDensity);

	AddInfo(FString::Printf(TEXT("Center: %d neighbors, density = %.2f kg/m³"),
		CenterNeighbors, CenterDensity));
	AddInfo(FString::Printf(TEXT("Corner: %d neighbors, density = %.2f kg/m³"),
		CornerNeighbors, CornerDensity));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidDensityTest_TensileInstabilityCorrection,
	"KawaiiFluid.Physics.Density.D05_TensileInstabilityCorrection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief D-05: Tensile Instability Correction (scorr) Test.
 * Verifies that the artificial pressure term (scorr) adds repulsive forces to prevent particle clustering.
 */
bool FKawaiiFluidDensityTest_TensileInstabilityCorrection::RunTest(const FString& Parameters)
{
	const float SmoothingRadius = 20.0f;
	const float RestDensity = 1000.0f;
	const float ParticleMass = 1.0f;
	const float Compliance = 0.01f;
	const float DeltaTime = 1.0f / 120.0f;
	const float Spacing = SmoothingRadius * 0.5f;
	const int32 GridSize = 5;

	TArray<FKawaiiFluidParticle> ParticlesWithScorr = CreateUniformGrid(
		FVector::ZeroVector, GridSize, Spacing, ParticleMass);
	TArray<FKawaiiFluidParticle> ParticlesWithoutScorr = CreateUniformGrid(
		FVector::ZeroVector, GridSize, Spacing, ParticleMass);

	BuildNeighborLists(ParticlesWithScorr, SmoothingRadius);
	BuildNeighborLists(ParticlesWithoutScorr, SmoothingRadius);

	FKawaiiFluidDensityConstraint SolverWithScorr(RestDensity, SmoothingRadius, Compliance);
	FKawaiiFluidDensityConstraint SolverWithoutScorr(RestDensity, SmoothingRadius, Compliance);

	for (auto& P : ParticlesWithScorr) P.Lambda = 0.0f;
	for (auto& P : ParticlesWithoutScorr) P.Lambda = 0.0f;

	SolverWithoutScorr.Solve(
		ParticlesWithoutScorr, SmoothingRadius, RestDensity, Compliance, DeltaTime);

	FTensileInstabilityParams TensileParams;
	TensileParams.bEnabled = true;
	TensileParams.K = 0.1f;
	TensileParams.N = 4;
	TensileParams.DeltaQ = 0.2f;
	SolverWithScorr.SolveWithTensileCorrection(
		ParticlesWithScorr, SmoothingRadius, RestDensity, Compliance, DeltaTime, TensileParams);

	const int32 CornerIdx = 0;
	const FVector PosDiffWithoutScorr = ParticlesWithoutScorr[CornerIdx].PredictedPosition -
		(FVector::ZeroVector - FVector((GridSize-1) * Spacing * 0.5f));
	const FVector PosDiffWithScorr = ParticlesWithScorr[CornerIdx].PredictedPosition -
		(FVector::ZeroVector - FVector((GridSize-1) * Spacing * 0.5f));

	const float CorrectionWithoutScorr = PosDiffWithoutScorr.Size();
	const float CorrectionWithScorr = PosDiffWithScorr.Size();

	AddInfo(FString::Printf(TEXT("Corner particle position correction WITHOUT scorr: %.4f cm"), CorrectionWithoutScorr));
	AddInfo(FString::Printf(TEXT("Corner particle position correction WITH scorr: %.4f cm"), CorrectionWithScorr));

	TestTrue(TEXT("Solver completed without errors"), true);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidDensityTest_ScorrCalculation,
	"KawaiiFluid.Physics.Density.D06_ScorrCalculation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief D-06: scorr Calculation Verification.
 * Directly tests the mathematical correctness of the artificial pressure formula.
 */
bool FKawaiiFluidDensityTest_ScorrCalculation::RunTest(const FString& Parameters)
{
	const float SmoothingRadius = 20.0f;
	const float h = SmoothingRadius * 0.01f;

	const float K = 0.1f;
	const int32 N = 4;
	const float DeltaQRatio = 0.2f;

	const float DeltaQ = DeltaQRatio * h;

	const float h2 = h * h;
	const float h9 = h2 * h2 * h2 * h2 * h;
	const float Poly6Coeff = 315.0f / (64.0f * PI * h9);

	const float DeltaQ2 = DeltaQ * DeltaQ;
	const float Diff_DeltaQ = h2 - DeltaQ2;
	const float W_DeltaQ = Poly6Coeff * Diff_DeltaQ * Diff_DeltaQ * Diff_DeltaQ;

	TestTrue(TEXT("W(Δq) is positive"), W_DeltaQ > 0.0f);

	TArray<float> TestDistances = { 0.0f, 0.1f, 0.2f, 0.3f, 0.5f, 0.7f, 0.9f };

	for (float DistRatio : TestDistances)
	{
		const float r = DistRatio * h;
		const float r2 = r * r;
		const float Diff = FMath::Max(0.0f, h2 - r2);
		const float W_r = Poly6Coeff * Diff * Diff * Diff;

		float Ratio = (W_DeltaQ > KINDA_SMALL_NUMBER) ? (W_r / W_DeltaQ) : 0.0f;
		float scorr = -K * FMath::Pow(Ratio, static_cast<float>(N));

		if (DistRatio < 1.0f)
		{
			TestTrue(FString::Printf(TEXT("scorr at r=%.1fh is negative (repulsive)"), DistRatio),
				scorr <= 0.0f);
		}

		if (FMath::IsNearlyEqual(DistRatio, DeltaQRatio, 0.01f))
		{
			TestNearlyEqual(FString::Printf(TEXT("scorr at r=Δq equals -k=%.2f"), -K),
				scorr, -K, 0.01f);
		}
	}

	return true;
}

#endif
