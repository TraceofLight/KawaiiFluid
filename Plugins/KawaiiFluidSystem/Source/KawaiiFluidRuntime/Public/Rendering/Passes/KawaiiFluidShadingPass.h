// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderGraphResources.h"
#include "ScreenPass.h"

struct FKawaiiFluidRenderingParameters;
struct FMetaballIntermediateTextures;
class FSceneView;

namespace KawaiiFluidRenderer
{
	/**
	 * @namespace KawaiiFluidRenderer
	 * @brief Stateless shading functions for the ScreenSpace fluid rendering pipeline.
	 * 
	 * Implements custom post-process shading including Blinn-Phong lighting, Fresnel reflections, 
	 * and Beer's Law based absorption for volumetric fluid appearance.
	 */
	void RenderShadingPass(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FKawaiiFluidRenderingParameters& RenderParams,
		const FMetaballIntermediateTextures& IntermediateTextures,
		FRDGTextureRef SceneDepthTexture,
		FRDGTextureRef SceneColorTexture,
		FScreenPassRenderTarget Output);
}