// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FRDGBuilder;
class FSceneView;
class FRDGTexture;
class UKawaiiFluidMetaballRenderer;
typedef FRDGTexture* FRDGTextureRef;

/**
 * @brief Fluid Depth rendering pass (Batched path).
 * Renders only the specified renderer list (for batch optimization).
 * @param OutLinearDepthTexture Output: Linear depth texture (R32F).
 * @param OutVelocityTexture Output: Screen-space velocity texture (RG16F) for flow effects.
 * @param OutOcclusionMaskTexture Output: Occlusion mask texture (R8) - 1.0=visible, 0.0=occluded.
 */
void RenderFluidDepthPass(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const TArray<UKawaiiFluidMetaballRenderer*>& Renderers,
	FRDGTextureRef SceneDepthTexture,
	FRDGTextureRef& OutLinearDepthTexture,
	FRDGTextureRef& OutVelocityTexture,
	FRDGTextureRef& OutOcclusionMaskTexture,
	FRDGTextureRef& OutHardwareDepth,
	bool bIncremental = false);
