// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "GPU/FluidStatsCompactShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterUtils.h"
#include "GlobalShader.h"

//=============================================================================
// Shader Implementation
//=============================================================================

IMPLEMENT_GLOBAL_SHADER(FCompactStatsCS,
	"/Plugin/KawaiiFluidSystem/Private/FluidStatsCompact.usf",
	"CompactStatsCS", SF_Compute);

IMPLEMENT_GLOBAL_SHADER(FCompactStatsExCS,
	"/Plugin/KawaiiFluidSystem/Private/FluidStatsCompact.usf",
	"CompactStatsExCS", SF_Compute);
