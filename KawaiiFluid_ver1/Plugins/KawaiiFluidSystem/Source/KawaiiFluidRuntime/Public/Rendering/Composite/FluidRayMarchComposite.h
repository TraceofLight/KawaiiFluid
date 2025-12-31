// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "IFluidCompositePass.h"
#include "RenderGraphResources.h"

/**
 * Ray Marching SDF rendering pass
 *
 * Implements ray marching through metaball SDF field for smooth fluid surfaces.
 * Best suited for slime-like fluids with:
 * - Fresnel reflection
 * - Subsurface scattering (SSS) for jelly effect
 * - Refraction
 * - Specular highlights
 *
 * Unlike Custom/GBuffer modes, this doesn't use intermediate Depth/Normal/Thickness
 * passes - everything is computed in a single ray marching pass.
 *
 * Supports two rendering modes:
 * - Direct particle iteration (bUseSDFVolume=false): O(N) per ray step
 * - SDF Volume texture (bUseSDFVolume=true): O(1) per ray step (optimized)
 */
class FFluidRayMarchComposite : public IFluidCompositePass
{
public:
	virtual ~FFluidRayMarchComposite() = default;

	/**
	 * Set particle data for SDF calculation (legacy mode)
	 * Must be called before RenderComposite
	 *
	 * @param InParticleBufferSRV Particle positions buffer (StructuredBuffer<FVector3f>)
	 * @param InParticleCount Number of particles
	 * @param InParticleRadius Particle radius for SDF
	 */
	void SetParticleData(
		FRDGBufferSRVRef InParticleBufferSRV,
		int32 InParticleCount,
		float InParticleRadius);

	/**
	 * Set SDF volume data for optimized rendering
	 * When set, uses O(1) volume texture sampling instead of O(N) particle iteration
	 *
	 * @param InSDFVolumeSRV Pre-baked 3D SDF texture
	 * @param InVolumeMin World-space minimum corner
	 * @param InVolumeMax World-space maximum corner
	 * @param InVolumeResolution Volume texture resolution
	 */
	void SetSDFVolumeData(
		FRDGTextureSRVRef InSDFVolumeSRV,
		const FVector3f& InVolumeMin,
		const FVector3f& InVolumeMax,
		const FIntVector& InVolumeResolution);

	/** Enable/disable SDF volume optimization */
	void SetUseSDFVolume(bool bEnable) { bUseSDFVolume = bEnable; }
	bool GetUseSDFVolume() const { return bUseSDFVolume; }

	virtual void RenderComposite(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FFluidRenderingParameters& RenderParams,
		const FFluidIntermediateTextures& IntermediateTextures,
		FRDGTextureRef SceneDepthTexture,
		FRDGTextureRef SceneColorTexture,
		FScreenPassRenderTarget Output) override;

	virtual ESSFRRenderingMode GetRenderingMode() const override
	{
		return ESSFRRenderingMode::RayMarching;
	}

private:
	/** Particle buffer SRV for shader access (legacy mode) */
	FRDGBufferSRVRef ParticleBufferSRV = nullptr;

	/** Number of particles */
	int32 ParticleCount = 0;

	/** Particle radius for SDF calculation */
	float ParticleRadius = 5.0f;

	/** SDF Volume optimization data */
	bool bUseSDFVolume = false;
	FRDGTextureSRVRef SDFVolumeSRV = nullptr;
	FVector3f VolumeMin = FVector3f::ZeroVector;
	FVector3f VolumeMax = FVector3f::ZeroVector;
	FIntVector VolumeResolution = FIntVector(64, 64, 64);
};
