// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FluidAnisotropy.generated.h"

/**
 * @brief Anisotropy calculation mode for ellipsoid rendering.
 * Based on NVIDIA FleX and Yu & Turk 2013 paper:
 * "Reconstructing surfaces of particle-based fluids using anisotropic kernels"
 */
UENUM(BlueprintType)
enum class EFluidAnisotropyMode : uint8
{
	/** No anisotropy - render as spheres */
	None UMETA(DisplayName = "None (Spheres)"),

	/** Stretch ellipsoids along velocity direction */
	VelocityBased UMETA(DisplayName = "Velocity Based"),

	/** Calculate from neighbor particle distribution (covariance matrix) */
	DensityBased UMETA(DisplayName = "Density Based"),

	/** Combine velocity and density-based approaches */
	Hybrid UMETA(DisplayName = "Hybrid")
};

/**
 * @brief GPU Anisotropy mode (must match shader defines).
 */
enum class EGPUAnisotropyMode : uint8
{
	VelocityBased = 0,
	DensityBased = 1,
	Hybrid = 2
};

/**
 * @brief Parameters for anisotropy calculation.
 * @param bEnabled Enable anisotropy calculation.
 * @param Mode Calculation mode (velocity, density, hybrid).
 * @param AnisotropyScale Overall scale factor for ellipsoid stretching.
 * @param AnisotropyMin Minimum scale to prevent too thin ellipsoids.
 * @param AnisotropyMax Maximum scale to prevent excessive stretching.
 * @param VelocityStretchFactor How much velocity affects stretching.
 * @param DensityWeight Weight for density-based component in hybrid mode.
 */
USTRUCT(BlueprintType)
struct KAWAIIFLUIDRUNTIME_API FFluidAnisotropyParams
{
	GENERATED_BODY()

	/** Enable anisotropy calculation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy")
	bool bEnabled = false;

	/** Calculation mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy")
	EFluidAnisotropyMode Mode = EFluidAnisotropyMode::DensityBased;

	/** Overall anisotropy scale (higher = more stretched ellipsoids) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy", meta = (ClampMin = "0.5", ClampMax = "3", UIMin = "0.5", UIMax = "3"))
	float AnisotropyScale = 1.0f;

	/** Minimum ellipsoid scale (prevents too thin shapes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy", meta = (ClampMin = "0.1", ClampMax = "1", UIMin = "0.1", UIMax = "1"))
	float AnisotropyMin = 0.2f;

	/** Maximum ellipsoid scale (prevents excessive stretching) - FleX recommends 1.0~2.0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy", meta = (ClampMin = "1", ClampMax = "3", UIMin = "1", UIMax = "3"))
	float AnisotropyMax = 2.0f;

	/** Velocity stretch factor (velocity-based mode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy", meta = (ClampMin = "0", ClampMax = "0.1", UIMin = "0", UIMax = "0.1", EditCondition = "Mode == EFluidAnisotropyMode::VelocityBased || Mode == EFluidAnisotropyMode::Hybrid"))
	float VelocityStretchFactor = 0.01f;

	/** Weight for density-based component in hybrid mode (0 = velocity only, 1 = density only) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", EditCondition = "Mode == EFluidAnisotropyMode::Hybrid"))
	float DensityWeight = 0.5f;

	/** Update interval in frames (1 = every frame, 2 = every other frame, etc.)
	 *  Higher values reduce GPU cost but may cause visual lag on fast movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy|Optimization", meta = (ClampMin = "1", ClampMax = "10", UIMin = "1", UIMax = "10"))
	int32 UpdateInterval = 1;

	//=========================================================================
	// Isolated Particle Handling (Yu & Turk style + extensions)
	//=========================================================================

	/** Minimum neighbor count for anisotropy calculation (Yu & Turk default: 25)
	 *  Below this threshold, particles are treated as "isolated" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy|Isolation", meta = (ClampMin = "3", ClampMax = "30", UIMin = "3", UIMax = "30"))
	int32 MinNeighborsForAnisotropy = 8;

	/** Enable size fade for isolated particles (particles shrink as they become more isolated) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy|Isolation")
	bool bFadeIsolatedParticles = true;

	/** Minimum scale for isolated particles (0 = invisible, 1 = no fade)
	 *  Only used when bFadeIsolatedParticles is true */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy|Isolation", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1", EditCondition = "bFadeIsolatedParticles"))
	float MinIsolatedScale = 0.3f;

	/** Enable velocity-based stretching for isolated particles
	 *  Isolated particles will stretch along velocity direction (splash effect) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy|Isolation")
	bool bStretchIsolatedByVelocity = true;

	/** Enable speed-based additional fade for isolated particles
	 *  Slow isolated particles fade more (simulates droplet absorption) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy|Isolation", meta = (EditCondition = "bFadeIsolatedParticles"))
	bool bFadeSlowIsolated = false;

	/** Speed threshold for slow isolated particle fade (cm/s)
	 *  Particles below this speed will fade more when isolated */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anisotropy|Isolation", meta = (ClampMin = "1", ClampMax = "100", UIMin = "1", UIMax = "100", EditCondition = "bFadeIsolatedParticles && bFadeSlowIsolated"))
	float IsolationFadeSpeed = 10.0f;
};

// FAnisotropyComputeParams is defined in GPU/FluidAnisotropyComputeShader.h
// to avoid including RenderGraphResources.h before .generated.h
