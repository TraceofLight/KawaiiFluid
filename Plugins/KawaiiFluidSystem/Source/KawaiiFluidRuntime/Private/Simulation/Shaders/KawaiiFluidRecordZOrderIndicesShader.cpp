// Copyright 2026 Team_Bruteforce. All Rights Reserved.
// Shader Implementation for KawaiiFluidZOrderDebug.usf

#include "Simulation/Shaders/KawaiiFluidRecordZOrderIndicesShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterUtils.h"
#include "GlobalShader.h"

//=============================================================================
// Shader Implementation
//=============================================================================

IMPLEMENT_GLOBAL_SHADER(FRecordZOrderIndicesCS,
	"/Plugin/KawaiiFluidSystem/Private/Sorting/KawaiiFluidZOrderDebug.usf",
	"RecordZOrderIndicesCS", SF_Compute);

/**
 * @brief Check if a shader permutation should be compiled.
 */
bool FRecordZOrderIndicesCS::ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
{
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
}

/**
 * @brief Modify the shader compilation environment.
 */
void FRecordZOrderIndicesCS::ModifyCompilationEnvironment(
	const FGlobalShaderPermutationParameters& Parameters,
	FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.SetDefine(TEXT("THREAD_GROUP_SIZE"), ThreadGroupSize);
}
