// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FRDGBuilder;
class FSceneView;
class UFluidRendererSubsystem;
class FRDGTexture;
typedef FRDGTexture* FRDGTextureRef;

/**
 * Fluid Thickness 렌더링 패스
 * 파티클의 두께를 누적하여 렌더링
 */
void RenderFluidThicknessPass(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	UFluidRendererSubsystem* Subsystem,
	FRDGTextureRef& OutThicknessTexture);
