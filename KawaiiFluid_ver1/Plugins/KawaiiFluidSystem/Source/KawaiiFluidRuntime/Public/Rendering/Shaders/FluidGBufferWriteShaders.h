// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphDefinitions.h"

/**
 * @brief Shared parameter structure for GBuffer write shaders
 *
 * Used by both vertex and pixel shaders for GBuffer write pass.
 */
BEGIN_SHADER_PARAMETER_STRUCT(FFluidGBufferWriteParameters, )
	// Input textures from shared passes
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SmoothedDepthTexture)
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, NormalTexture)
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ThicknessTexture)
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, FluidSceneDepthTexture)

	// Samplers
	SHADER_PARAMETER_SAMPLER(SamplerState, PointClampSampler)
	SHADER_PARAMETER_SAMPLER(SamplerState, BilinearClampSampler)

	// Material parameters
	SHADER_PARAMETER(FVector3f, FluidBaseColor)
	SHADER_PARAMETER(float, Metallic)
	SHADER_PARAMETER(float, Roughness)
	SHADER_PARAMETER(float, SubsurfaceOpacity)
	SHADER_PARAMETER(float, AbsorptionCoefficient)

	// View uniforms
	SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)

	// Output: MRT
	RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

/**
 * @brief Vertex shader for GBuffer write (fullscreen triangle)
 */
class FFluidGBufferWriteVS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFluidGBufferWriteVS);
	SHADER_USE_PARAMETER_STRUCT(FFluidGBufferWriteVS, FGlobalShader);

	using FParameters = FFluidGBufferWriteParameters;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

/**
 * @brief Pixel shader for writing fluid surface to GBuffer
 *
 * Outputs to Multiple Render Targets (GBufferA/B/C/D) for integration
 * with Unreal's deferred rendering pipeline (Lumen, VSM, GI).
 */
class FFluidGBufferWritePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FFluidGBufferWritePS);
	SHADER_USE_PARAMETER_STRUCT(FFluidGBufferWritePS, FGlobalShader);

	using FParameters = FFluidGBufferWriteParameters;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("BACKGROUND_DEPTH_THRESH"), 3.0e30f);
	}
};
