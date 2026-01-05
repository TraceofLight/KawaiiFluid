// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderGraphDefinitions.h"

/**
 * @brief Parameters for VSM blur pass.
 * @param BlurRadius Blur kernel radius in pixels.
 * @param NumIterations Number of blur iterations (each iteration = H + V pass).
 */
struct FFluidVSMBlurParams
{
	/** Blur radius in pixels */
	float BlurRadius = 4.0f;

	/** Number of blur iterations */
	int32 NumIterations = 1;
};

/**
 * Apply separable Gaussian blur to VSM texture.
 *
 * VSM can be safely blurred because it stores (depth, depth²),
 * and the variance calculation remains mathematically valid after filtering.
 * This helps fill holes from sparse projection and softens shadow edges.
 *
 * @param GraphBuilder RDG builder for pass registration.
 * @param InputVSMTexture Input VSM texture (RG32F: depth, depth²).
 * @param Params Blur parameters.
 * @param OutBlurredVSMTexture Output blurred VSM texture.
 */
void RenderFluidVSMBlur(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef InputVSMTexture,
	const FFluidVSMBlurParams& Params,
	FRDGTextureRef& OutBlurredVSMTexture);
