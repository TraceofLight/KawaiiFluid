// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderGraphDefinitions.h"
#include "ScreenPass.h"

/**
 * @brief Parameters for fluid shadow receiver pass.
 * @param ShadowIntensity How dark the shadow appears (0-1).
 * @param ShadowBias Depth bias to prevent self-shadowing.
 * @param MinVariance Minimum variance for VSM calculation.
 * @param LightBleedReduction Reduces light bleeding artifacts (0-1).
 * @param bDebugVisualization Enable debug overlay visualization.
 */
struct FFluidShadowReceiverParams
{
	/** Shadow intensity (0 = no shadow, 1 = full shadow) */
	float ShadowIntensity = 0.8f;

	/** Depth bias to prevent self-shadowing artifacts */
	float ShadowBias = 0.001f;

	/** Minimum variance for Chebyshev's inequality (prevents division by zero) */
	float MinVariance = 0.00001f;

	/** Light bleed reduction factor (0-1) */
	float LightBleedReduction = 0.2f;

	/** Enable debug visualization overlay */
	bool bDebugVisualization = false;
};

/**
 * Render fluid shadow receiver pass.
 *
 * This pass applies VSM shadows from fluid onto the scene.
 * It samples the fluid VSM texture and darkens scene pixels that are
 * occluded by the fluid from the light's perspective.
 *
 * @param GraphBuilder RDG builder for pass registration.
 * @param View Current scene view.
 * @param SceneColorTexture Scene color texture to apply shadows to.
 * @param SceneDepthTexture Scene depth texture for world position reconstruction.
 * @param FluidVSMTexture Fluid VSM texture (RG32F: depth, depth squared).
 * @param LightViewProjectionMatrix Transform from world to light clip space.
 * @param Params Shadow receiver parameters.
 * @param Output Output render target for shadowed scene color.
 */
void RenderFluidShadowReceiver(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	FRDGTextureRef SceneColorTexture,
	FRDGTextureRef SceneDepthTexture,
	FRDGTextureRef FluidVSMTexture,
	const FMatrix44f& LightViewProjectionMatrix,
	const FFluidShadowReceiverParams& Params,
	FScreenPassRenderTarget& Output);
