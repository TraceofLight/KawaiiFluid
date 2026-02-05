// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderGraphDefinitions.h"

class FSceneView;

/**
 * Normal Reconstruction Pass
 * Reconstructs world-space normals from the smoothed depth buffer.
 */
void RenderFluidNormalPass(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	FRDGTextureRef SmoothedDepthTexture,
	FRDGTextureRef& OutNormalTexture);
