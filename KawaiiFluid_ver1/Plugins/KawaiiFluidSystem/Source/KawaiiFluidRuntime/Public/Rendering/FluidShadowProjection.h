// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderGraphDefinitions.h"

class FSceneView;
struct FFluidShadowHistoryBuffer;

/**
 * @brief Output data from fluid shadow projection pass.
 * @param VSMTexture The Variance Shadow Map texture (RG32F: depth, depth²).
 * @param bIsValid Whether the output contains valid shadow data.
 */
struct FFluidShadowProjectionOutput
{
	/** VSM texture (RG32F format: R=depth, G=depth²) */
	FRDGTextureRef VSMTexture = nullptr;

	/** Whether valid shadow data was generated */
	bool bIsValid = false;
};

/**
 * @brief Parameters for fluid shadow projection.
 * @param VSMResolution Resolution of the output VSM texture.
 * @param LightViewProjectionMatrix Light's view-projection matrix for shadow mapping.
 */
struct FFluidShadowProjectionParams
{
	/** Resolution of the output VSM texture */
	FIntPoint VSMResolution = FIntPoint(1024, 1024);

	/** Light view-projection matrix */
	FMatrix44f LightViewProjectionMatrix = FMatrix44f::Identity;
};

/**
 * Project fluid depth from previous frame into light space for VSM shadow generation.
 *
 * This function takes the history buffer containing the previous frame's SSFR depth
 * and projects it into light space to generate a Variance Shadow Map (VSM).
 *
 * Pipeline:
 * 1. ClearAtomicBufferCS - Initialize atomic depth buffer
 * 2. ProjectFluidShadowCS - Project each camera pixel to light space
 * 3. FinalizeVSMCS - Convert atomic buffer to VSM format (depth, depth²)
 *
 * @param GraphBuilder RDG builder for pass registration.
 * @param View Current frame's scene view.
 * @param HistoryBuffer Previous frame's depth and matrix data.
 * @param Params Shadow projection parameters.
 * @param OutProjection Output shadow projection data.
 */
void RenderFluidShadowProjection(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FFluidShadowHistoryBuffer& HistoryBuffer,
	const FFluidShadowProjectionParams& Params,
	FFluidShadowProjectionOutput& OutProjection);
