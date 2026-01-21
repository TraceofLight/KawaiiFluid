// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderGraphResources.h"
#include "ScreenPass.h"

struct FFluidRenderingParameters;
struct FMetaballIntermediateTextures;
class FSceneView;

/**
 * ScreenSpace Pipeline Shading Implementation
 *
 * Contains shading functions for the ScreenSpace rendering pipeline.
 * These are stateless functions called by FKawaiiMetaballScreenSpacePipeline.
 *
 * Supports PostProcess shading mode (custom shading with Blinn-Phong, Fresnel, Beer's Law).
 */
namespace KawaiiScreenSpaceShading
{
	/**
	 * Render PostProcess shading pass
	 *
	 * Applies Blinn-Phong lighting, Fresnel, and Beer's Law absorption
	 * using intermediate textures (depth, normal, thickness) from ScreenSpace pipeline.
	 *
	 * @param GraphBuilder - RDG builder
	 * @param View - Scene view
	 * @param RenderParams - Fluid rendering parameters
	 * @param IntermediateTextures - Cached textures from PrepareForTonemap (depth, normal, thickness)
	 * @param SceneDepthTexture - Scene depth for depth comparison
	 * @param SceneColorTexture - Scene color for background sampling
	 * @param Output - Render target to composite onto
	 */
	void RenderPostProcessShading(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FFluidRenderingParameters& RenderParams,
		const FMetaballIntermediateTextures& IntermediateTextures,
		FRDGTextureRef SceneDepthTexture,
		FRDGTextureRef SceneColorTexture,
		FScreenPassRenderTarget Output);
}
