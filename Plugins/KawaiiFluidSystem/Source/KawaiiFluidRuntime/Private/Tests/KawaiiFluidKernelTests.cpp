// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Simulation/Physics/KawaiiFluidSPHKernels.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidKernelTest_Poly6Coefficient,
	"KawaiiFluid.Physics.Kernels.K01_Poly6Coefficient",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidKernelTest_Poly6AtOrigin,
	"KawaiiFluid.Physics.Kernels.K02_Poly6AtOrigin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidKernelTest_Poly6AtBoundary,
	"KawaiiFluid.Physics.Kernels.K03_Poly6AtBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidKernelTest_Poly6Normalization,
	"KawaiiFluid.Physics.Kernels.K04_Poly6Normalization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidKernelTest_SpikyGradientDirection,
	"KawaiiFluid.Physics.Kernels.K05_SpikyGradientDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidKernelTest_SpikyAtOrigin,
	"KawaiiFluid.Physics.Kernels.K06_SpikyAtOrigin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidKernelTest_SpikyAtBoundary,
	"KawaiiFluid.Physics.Kernels.K07_SpikyAtBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKawaiiFluidKernelTest_UnitConversion,
	"KawaiiFluid.Physics.Kernels.K08_UnitConversion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * @brief K-01: Poly6 Kernel Coefficient Test.
 * Formula: 315 / (64 * PI * h^9).
 * Expected: h=0.2m (20cm) -> ~9.77 * 10^6.
 */
bool FKawaiiFluidKernelTest_Poly6Coefficient::RunTest(const FString& Parameters)
{
	const float h_cm = 20.0f;  // Unreal units (cm)
	const float h_m = h_cm * 0.01f;  // Convert to meters

	const float h9 = FMath::Pow(h_m, 9.0f);
	const float ExpectedCoeff = 315.0f / (64.0f * PI * h9);

	const float ActualCoeff = SPHKernels::Poly6Coefficient(h_m);

	const float Tolerance = ExpectedCoeff * 0.01f;

	TestNearlyEqual(TEXT("Poly6 coefficient matches formula"), ActualCoeff, ExpectedCoeff, Tolerance);

	AddInfo(FString::Printf(TEXT("h = %.4f m, Expected Coeff = %.2e, Actual = %.2e"),
		h_m, ExpectedCoeff, ActualCoeff));

	return true;
}

/**
 * @brief K-02: Poly6 at Origin (r=0).
 * Expected: Maximum value = Coeff * h^6.
 */
bool FKawaiiFluidKernelTest_Poly6AtOrigin::RunTest(const FString& Parameters)
{
	const float h_cm = 20.0f;

	const float W_origin = SPHKernels::Poly6(0.0f, h_cm);

	const float h_m = h_cm * 0.01f;
	const float h2 = h_m * h_m;
	const float h6 = h2 * h2 * h2;
	const float Coeff = SPHKernels::Poly6Coefficient(h_m);
	const float ExpectedMax = Coeff * h6;

	const float Tolerance = ExpectedMax * 0.01f;

	TestNearlyEqual(TEXT("Poly6(0,h) equals maximum value"), W_origin, ExpectedMax, Tolerance);
	TestTrue(TEXT("Poly6 at origin is positive"), W_origin > 0.0f);

	AddInfo(FString::Printf(TEXT("W(0, %.1f cm) = %.2e, Expected Max = %.2e"),
		h_cm, W_origin, ExpectedMax));

	return true;
}

/**
 * @brief K-03: Poly6 at Boundary (r=h).
 * Expected: 0 (kernel vanishes at boundary).
 */
bool FKawaiiFluidKernelTest_Poly6AtBoundary::RunTest(const FString& Parameters)
{
	const float h_cm = 20.0f;

	const float W_boundary = SPHKernels::Poly6(h_cm, h_cm);

	TestNearlyEqual(TEXT("Poly6(h,h) equals zero"), W_boundary, 0.0f, KINDA_SMALL_NUMBER);

	const float W_outside = SPHKernels::Poly6(h_cm + 1.0f, h_cm);
	TestNearlyEqual(TEXT("Poly6(r>h,h) equals zero"), W_outside, 0.0f, KINDA_SMALL_NUMBER);

	AddInfo(FString::Printf(TEXT("W(h, h) = %.6f, W(h+1, h) = %.6f"), W_boundary, W_outside));

	return true;
}

/**
 * @brief K-04: Poly6 Normalization Test.
 * Numerical integral of W over a 3D sphere should be approximately 1.
 */
bool FKawaiiFluidKernelTest_Poly6Normalization::RunTest(const FString& Parameters)
{
	const float h_cm = 20.0f;
	const float h_m = h_cm * 0.01f;

	float IntegralSum = 0.0f;
	const int32 NumSteps = 1000;
	const float dr = h_m / static_cast<float>(NumSteps);

	for (int32 i = 0; i < NumSteps; ++i)
	{
		const float r_m = (static_cast<float>(i) + 0.5f) * dr;
		const float r_cm = r_m * 100.0f;

		const float W = SPHKernels::Poly6(r_cm, h_cm);
		const float ShellVolume = 4.0f * PI * r_m * r_m * dr;

		IntegralSum += W * ShellVolume;
	}

	const float Tolerance = 0.05f;

	TestNearlyEqual(TEXT("Poly6 integral is approximately 1"), IntegralSum, 1.0f, Tolerance);

	AddInfo(FString::Printf(TEXT("Numerical integral of Poly6 over sphere: %.4f (expected ~1.0)"), IntegralSum));

	return true;
}

/**
 * @brief K-05: Spiky Gradient Direction Test.
 * Gradient should point towards the neighbor due to the negative coefficient in the Spiky kernel.
 */
bool FKawaiiFluidKernelTest_SpikyGradientDirection::RunTest(const FString& Parameters)
{
	const float h_cm = 20.0f;

	const FVector r_x = FVector(10.0f, 0.0f, 0.0f);
	const FVector GradW_x = SPHKernels::SpikyGradient(r_x, h_cm);

	TestTrue(TEXT("Spiky gradient X component points toward neighbor (negative)"), GradW_x.X < 0.0);
	TestTrue(TEXT("Spiky gradient Y is zero for X-aligned r"), FMath::IsNearlyZero(GradW_x.Y, static_cast<double>(KINDA_SMALL_NUMBER)));
	TestTrue(TEXT("Spiky gradient Z is zero for X-aligned r"), FMath::IsNearlyZero(GradW_x.Z, static_cast<double>(KINDA_SMALL_NUMBER)));

	const FVector r_y = FVector(0.0f, -15.0f, 0.0f);
	const FVector GradW_y = SPHKernels::SpikyGradient(r_y, h_cm);

	TestTrue(TEXT("Spiky gradient Y component points toward neighbor (positive)"), GradW_y.Y > 0.0);

	TestTrue(TEXT("Gradient magnitude is positive"), GradW_x.Size() > 0.0);
	TestTrue(TEXT("Gradient magnitude is finite"), FMath::IsFinite(GradW_x.Size()));

	AddInfo(FString::Printf(TEXT("GradW(10,0,0) = (%.4f, %.4f, %.4f)"), GradW_x.X, GradW_x.Y, GradW_x.Z));
	AddInfo(FString::Printf(TEXT("GradW(0,-15,0) = (%.4f, %.4f, %.4f)"), GradW_y.X, GradW_y.Y, GradW_y.Z));

	return true;
}

/**
 * @brief K-06: Spiky Gradient at Origin (r->0).
 * Should return zero to avoid mathematical singularities.
 */
bool FKawaiiFluidKernelTest_SpikyAtOrigin::RunTest(const FString& Parameters)
{
	const float h_cm = 20.0f;

	const FVector GradW_origin = SPHKernels::SpikyGradient(FVector::ZeroVector, h_cm);

	TestTrue(TEXT("Spiky gradient at origin X is zero"), FMath::IsNearlyZero(GradW_origin.X, static_cast<double>(KINDA_SMALL_NUMBER)));
	TestTrue(TEXT("Spiky gradient at origin Y is zero"), FMath::IsNearlyZero(GradW_origin.Y, static_cast<double>(KINDA_SMALL_NUMBER)));
	TestTrue(TEXT("Spiky gradient at origin Z is zero"), FMath::IsNearlyZero(GradW_origin.Z, static_cast<double>(KINDA_SMALL_NUMBER)));

	const FVector r_tiny = FVector(0.001f, 0.0f, 0.0f);
	const FVector GradW_tiny = SPHKernels::SpikyGradient(r_tiny, h_cm);

	TestTrue(TEXT("Spiky gradient near origin is finite"), FMath::IsFinite(GradW_tiny.X));
	TestFalse(TEXT("Spiky gradient near origin is not NaN"), FMath::IsNaN(GradW_tiny.X));

	AddInfo(FString::Printf(TEXT("GradW(0,0,0) = (%.6f, %.6f, %.6f)"),
		GradW_origin.X, GradW_origin.Y, GradW_origin.Z));
	AddInfo(FString::Printf(TEXT("GradW(0.001,0,0) = (%.4f, %.4f, %.4f)"),
		GradW_tiny.X, GradW_tiny.Y, GradW_tiny.Z));

	return true;
}

/**
 * @brief K-07: Spiky Gradient at Boundary (r=h).
 * Should return zero as the kernel derivative vanishes at the boundary.
 */
bool FKawaiiFluidKernelTest_SpikyAtBoundary::RunTest(const FString& Parameters)
{
	const float h_cm = 20.0f;

	const FVector r_boundary = FVector(h_cm, 0.0f, 0.0f);
	const FVector GradW_boundary = SPHKernels::SpikyGradient(r_boundary, h_cm);

	TestTrue(TEXT("Spiky gradient at boundary is zero"),
		FMath::IsNearlyZero(GradW_boundary.Size(), static_cast<double>(KINDA_SMALL_NUMBER)));

	const FVector r_outside = FVector(h_cm + 5.0f, 0.0f, 0.0f);
	const FVector GradW_outside = SPHKernels::SpikyGradient(r_outside, h_cm);

	TestTrue(TEXT("Spiky gradient outside boundary is zero"),
		FMath::IsNearlyZero(GradW_outside.Size(), static_cast<double>(KINDA_SMALL_NUMBER)));

	AddInfo(FString::Printf(TEXT("GradW at boundary |r|=h: (%.6f, %.6f, %.6f)"),
		GradW_boundary.X, GradW_boundary.Y, GradW_boundary.Z));

	return true;
}

/**
 * @brief K-08: Unit Conversion Test (cm <-> m).
 * Verifies consistency of internal unit conversions between Unreal (cm) and SI (m).
 */
bool FKawaiiFluidKernelTest_UnitConversion::RunTest(const FString& Parameters)
{
	const float h_cm = 20.0f;

	const float r_half_cm = 10.0f;
	const float W_half = SPHKernels::Poly6(r_half_cm, h_cm);

	TestTrue(TEXT("Poly6 at half radius is positive"), W_half > 0.0f);

	const float W_max = SPHKernels::Poly6(0.0f, h_cm);
	TestTrue(TEXT("Poly6 at half radius is less than maximum"), W_half < W_max);

	const float r1 = 5.0f;
	const float r2 = 10.0f;
	const float r3 = 15.0f;

	const float W1 = SPHKernels::Poly6(r1, h_cm);
	const float W2 = SPHKernels::Poly6(r2, h_cm);
	const float W3 = SPHKernels::Poly6(r3, h_cm);

	TestTrue(TEXT("Poly6 decreases monotonically: W(5cm) > W(10cm)"), W1 > W2);
	TestTrue(TEXT("Poly6 decreases monotonically: W(10cm) > W(15cm)"), W2 > W3);

	SPHKernels::FKernelCoefficients Coeffs;
	Coeffs.Precompute(h_cm);

	const float h_m = h_cm * 0.01f;
	TestNearlyEqual(TEXT("Precomputed h equals h_m"), Coeffs.h, h_m, 0.0001f);
	TestNearlyEqual(TEXT("Precomputed h2 equals h^2"), Coeffs.h2, h_m * h_m, 0.0001f);

	AddInfo(FString::Printf(TEXT("W(5cm)=%.4e, W(10cm)=%.4e, W(15cm)=%.4e"), W1, W2, W3));
	AddInfo(FString::Printf(TEXT("Precomputed: h=%.4f, h2=%.6f, Poly6Coeff=%.2e"),
		Coeffs.h, Coeffs.h2, Coeffs.Poly6Coeff));

	return true;
}

#endif
