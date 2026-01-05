// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Rendering/FluidVSMBlur.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "ShaderCore.h"

// Maximum kernel radius (must match shader)
static constexpr int32 MAX_KERNEL_RADIUS = 16;
// Number of FVector4f needed to pack MAX_KERNEL_RADIUS+1 floats (17 floats = 5 float4s)
static constexpr int32 GAUSSIAN_WEIGHTS_VECTOR_COUNT = (MAX_KERNEL_RADIUS + 1 + 3) / 4;

//=============================================================================
// VSM Blur Compute Shader
//=============================================================================

class FFluidVSMBlurCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FFluidVSMBlurCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidVSMBlurCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float2>, InputVSM)
		SHADER_PARAMETER(FVector2f, TextureSize)
		SHADER_PARAMETER(FVector2f, BlurDirection)
		SHADER_PARAMETER(float, BlurRadius)
		SHADER_PARAMETER(int32, KernelRadius)
		SHADER_PARAMETER_ARRAY(FVector4f, GaussianWeights, [GAUSSIAN_WEIGHTS_VECTOR_COUNT])
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float2>, OutputVSM)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters,
	                                         FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_X"), 8);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZE_Y"), 8);
		OutEnvironment.SetDefine(TEXT("MAX_KERNEL_RADIUS"), MAX_KERNEL_RADIUS);
		OutEnvironment.SetDefine(TEXT("GAUSSIAN_WEIGHTS_VECTOR_COUNT"), GAUSSIAN_WEIGHTS_VECTOR_COUNT);
	}
};

IMPLEMENT_GLOBAL_SHADER(FFluidVSMBlurCS,
                        "/Plugin/KawaiiFluidSystem/Private/FluidVSMBlur.usf",
                        "VSMBlurCS",
                        SF_Compute);

//=============================================================================
// Helper Functions
//=============================================================================

/**
 * @brief Compute Gaussian weight for a given distance.
 * @param Distance Distance from center.
 * @param Sigma Standard deviation.
 * @return Gaussian weight.
 */
static float ComputeGaussianWeight(float Distance, float Sigma)
{
	if (Sigma <= 0.0001f)
	{
		return 0.0f;
	}
	const float Exponent = -(Distance * Distance) / (2.0f * Sigma * Sigma);
	return FMath::Exp(Exponent);
}

/**
 * @brief Precompute Gaussian weights for blur kernel.
 * @param Radius Kernel radius.
 * @param OutWeights Output weight array (size = MAX_KERNEL_RADIUS + 1).
 */
static void ComputeGaussianWeights(int32 Radius, float* OutWeights)
{
	// Sigma is typically radius / 2.5 for good quality
	const float Sigma = FMath::Max(1.0f, Radius / 2.5f);

	for (int32 i = 0; i <= MAX_KERNEL_RADIUS; ++i)
	{
		if (i <= Radius)
		{
			OutWeights[i] = ComputeGaussianWeight(static_cast<float>(i), Sigma);
		}
		else
		{
			OutWeights[i] = 0.0f;
		}
	}
}

//=============================================================================
// Render Function Implementation
//=============================================================================

/**
 * @brief Apply separable Gaussian blur to VSM texture.
 * @param GraphBuilder RDG builder for pass registration.
 * @param InputVSMTexture Input VSM texture.
 * @param Params Blur parameters.
 * @param OutBlurredVSMTexture Output blurred VSM texture.
 */
void RenderFluidVSMBlur(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef InputVSMTexture,
	const FFluidVSMBlurParams& Params,
	FRDGTextureRef& OutBlurredVSMTexture)
{
	RDG_EVENT_SCOPE(GraphBuilder, "FluidVSMBlur");

	if (!InputVSMTexture)
	{
		OutBlurredVSMTexture = nullptr;
		return;
	}

	FIntPoint TextureSize = InputVSMTexture->Desc.Extent;
	int32 NumIterations = FMath::Clamp(Params.NumIterations, 1, 5);
	int32 KernelRadius = FMath::Clamp(static_cast<int32>(Params.BlurRadius), 1, MAX_KERNEL_RADIUS);

	// Precompute Gaussian weights and pack into FVector4f array
	float GaussianWeightsRaw[MAX_KERNEL_RADIUS + 1];
	ComputeGaussianWeights(KernelRadius, GaussianWeightsRaw);

	// Pack floats into FVector4f array (each float4 holds 4 weights)
	FVector4f GaussianWeightsPacked[GAUSSIAN_WEIGHTS_VECTOR_COUNT];
	FMemory::Memzero(GaussianWeightsPacked, sizeof(GaussianWeightsPacked));
	for (int32 i = 0; i <= MAX_KERNEL_RADIUS; ++i)
	{
		int32 VecIndex = i / 4;
		int32 CompIndex = i % 4;
		GaussianWeightsPacked[VecIndex][CompIndex] = GaussianWeightsRaw[i];
	}

	// Create intermediate texture for ping-pong
	FRDGTextureDesc IntermediateDesc = FRDGTextureDesc::Create2D(
		TextureSize,
		PF_G32R32F,
		FClearValueBinding::None,
		TexCreate_ShaderResource | TexCreate_UAV);

	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FFluidVSMBlurCS> ComputeShader(GlobalShaderMap);

	FRDGTextureRef CurrentInput = InputVSMTexture;

	for (int32 Iteration = 0; Iteration < NumIterations; ++Iteration)
	{
		// Create textures for this iteration
		FRDGTextureRef HorizontalOutput = GraphBuilder.CreateTexture(
			IntermediateDesc, TEXT("VSMBlurHorizontal"));
		FRDGTextureRef VerticalOutput = GraphBuilder.CreateTexture(
			IntermediateDesc, TEXT("VSMBlurVertical"));

		//=====================================================================
		// Horizontal Pass
		//=====================================================================
		{
			auto* PassParameters = GraphBuilder.AllocParameters<FFluidVSMBlurCS::FParameters>();
			PassParameters->InputVSM = CurrentInput;
			PassParameters->TextureSize = FVector2f(TextureSize.X, TextureSize.Y);
			PassParameters->BlurDirection = FVector2f(1.0f, 0.0f); // Horizontal
			PassParameters->BlurRadius = Params.BlurRadius;
			PassParameters->KernelRadius = KernelRadius;
			for (int32 i = 0; i < GAUSSIAN_WEIGHTS_VECTOR_COUNT; ++i)
			{
				PassParameters->GaussianWeights[i] = GaussianWeightsPacked[i];
			}
			PassParameters->OutputVSM = GraphBuilder.CreateUAV(HorizontalOutput);

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("VSMBlur_Horizontal_Iter%d", Iteration),
				ComputeShader,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(TextureSize, 8));
		}

		//=====================================================================
		// Vertical Pass
		//=====================================================================
		{
			auto* PassParameters = GraphBuilder.AllocParameters<FFluidVSMBlurCS::FParameters>();
			PassParameters->InputVSM = HorizontalOutput;
			PassParameters->TextureSize = FVector2f(TextureSize.X, TextureSize.Y);
			PassParameters->BlurDirection = FVector2f(0.0f, 1.0f); // Vertical
			PassParameters->BlurRadius = Params.BlurRadius;
			PassParameters->KernelRadius = KernelRadius;
			for (int32 i = 0; i < GAUSSIAN_WEIGHTS_VECTOR_COUNT; ++i)
			{
				PassParameters->GaussianWeights[i] = GaussianWeightsPacked[i];
			}
			PassParameters->OutputVSM = GraphBuilder.CreateUAV(VerticalOutput);

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("VSMBlur_Vertical_Iter%d", Iteration),
				ComputeShader,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(TextureSize, 8));
		}

		// Use this iteration's output as next iteration's input
		CurrentInput = VerticalOutput;
	}

	OutBlurredVSMTexture = CurrentInput;
}
