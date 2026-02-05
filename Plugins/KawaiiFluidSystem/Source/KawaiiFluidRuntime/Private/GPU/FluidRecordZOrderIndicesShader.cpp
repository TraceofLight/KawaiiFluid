// Copyright 2026 Team_Bruteforce. All Rights Reserved.
// Shader Implementation for FluidRecordZOrderIndices.usf

#include "GPU/FluidRecordZOrderIndicesShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterUtils.h"
#include "GlobalShader.h"

//=============================================================================
// Shader Implementation
//=============================================================================

IMPLEMENT_GLOBAL_SHADER(FRecordZOrderIndicesCS,
	"/Plugin/KawaiiFluidSystem/Private/FluidRecordZOrderIndices.usf",
	"RecordZOrderIndicesCS", SF_Compute);
