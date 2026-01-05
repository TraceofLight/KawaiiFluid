// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Rendering/FluidShadowReceiver.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "SceneView.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "ScreenPass.h"
#include "PostProcess/PostProcessing.h"
#include "PixelShaderUtils.h"

//=============================================================================
// Shader Permutation
//=============================================================================

class FDebugVisualizationDim : SHADER_PERMUTATION_BOOL("DEBUG_VISUALIZATION");

//=============================================================================
// Fluid Shadow Receiver Vertex Shader
//=============================================================================

class FFluidShadowReceiverVS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FFluidShadowReceiverVS);
	SHADER_USE_PARAMETER_STRUCT(FFluidShadowReceiverVS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

IMPLEMENT_GLOBAL_SHADER(FFluidShadowReceiverVS,
                        "/Plugin/KawaiiFluidSystem/Private/FluidShadowReceiver.usf",
                        "MainVS",
                        SF_Vertex);

//=============================================================================
// Fluid Shadow Receiver Pixel Shader
//=============================================================================

class FFluidShadowReceiverPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FFluidShadowReceiverPS);
	SHADER_USE_PARAMETER_STRUCT(FFluidShadowReceiverPS, FGlobalShader);

	using FPermutationDomain = TShaderPermutationDomain<FDebugVisualizationDim>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ShadowReceiverSceneColor)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ShadowReceiverSceneDepth)
		SHADER_PARAMETER_SAMPLER(SamplerState, ShadowReceiverColorSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, ShadowReceiverDepthSampler)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, FluidVSMTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, FluidVSMSampler)
		SHADER_PARAMETER(FMatrix44f, InvViewProjectionMatrix)
		SHADER_PARAMETER(FMatrix44f, LightViewProjectionMatrix)
		SHADER_PARAMETER(float, ShadowIntensity)
		SHADER_PARAMETER(float, ShadowBias)
		SHADER_PARAMETER(float, MinVariance)
		SHADER_PARAMETER(float, LightBleedReduction)
		SHADER_PARAMETER(FVector2f, ViewportSize)
		SHADER_PARAMETER(FVector2f, VSMTextureSize)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

IMPLEMENT_GLOBAL_SHADER(FFluidShadowReceiverPS,
                        "/Plugin/KawaiiFluidSystem/Private/FluidShadowReceiver.usf",
                        "MainPS",
                        SF_Pixel);

//=============================================================================
// Render Function Implementation
//=============================================================================

/**
 * @brief Render fluid shadow receiver pass.
 * @param GraphBuilder RDG builder for pass registration.
 * @param View Current scene view.
 * @param SceneColorTexture Scene color texture to apply shadows to.
 * @param SceneDepthTexture Scene depth texture for world position reconstruction.
 * @param FluidVSMTexture Fluid VSM texture.
 * @param LightViewProjectionMatrix Transform from world to light clip space.
 * @param Params Shadow receiver parameters.
 * @param Output Output render target for shadowed scene color.
 */
void RenderFluidShadowReceiver(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	FRDGTextureRef SceneColorTexture,
	FRDGTextureRef SceneDepthTexture,
	FRDGTextureRef FluidVSMTexture,
	const FMatrix44f& LightViewProjectionMatrix,
	const FFluidShadowReceiverParams& Params,
	FScreenPassRenderTarget& Output)
{
	RDG_EVENT_SCOPE(GraphBuilder, "FluidShadowReceiver");

	// Validate inputs
	if (!SceneColorTexture || !SceneDepthTexture || !FluidVSMTexture || !Output.IsValid())
	{
		return;
	}

	FIntPoint ViewportSize = SceneColorTexture->Desc.Extent;
	FIntPoint VSMSize = FluidVSMTexture->Desc.Extent;

	// Get shaders
	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

	FFluidShadowReceiverPS::FPermutationDomain PermutationVector;
	PermutationVector.Set<FDebugVisualizationDim>(Params.bDebugVisualization);

	TShaderMapRef<FFluidShadowReceiverVS> VertexShader(GlobalShaderMap);
	TShaderMapRef<FFluidShadowReceiverPS> PixelShader(GlobalShaderMap, PermutationVector);

	// Setup parameters
	auto* PassParameters = GraphBuilder.AllocParameters<FFluidShadowReceiverPS::FParameters>();
	PassParameters->ShadowReceiverSceneColor = SceneColorTexture;
	PassParameters->ShadowReceiverSceneDepth = SceneDepthTexture;
	PassParameters->ShadowReceiverColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->ShadowReceiverDepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->FluidVSMTexture = FluidVSMTexture;
	PassParameters->FluidVSMSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

	// Calculate inverse view-projection matrix
	FMatrix InvViewProjection = View.ViewMatrices.GetInvViewProjectionMatrix();
	PassParameters->InvViewProjectionMatrix = FMatrix44f(InvViewProjection);
	PassParameters->LightViewProjectionMatrix = LightViewProjectionMatrix;

	PassParameters->ShadowIntensity = Params.ShadowIntensity;
	PassParameters->ShadowBias = Params.ShadowBias;
	PassParameters->MinVariance = Params.MinVariance;
	PassParameters->LightBleedReduction = Params.LightBleedReduction;
	PassParameters->ViewportSize = FVector2f(ViewportSize.X, ViewportSize.Y);
	PassParameters->VSMTextureSize = FVector2f(VSMSize.X, VSMSize.Y);
	PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

	// Add render pass
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("FluidShadowReceiver%s", Params.bDebugVisualization ? TEXT("_Debug") : TEXT("")),
		PassParameters,
		ERDGPassFlags::Raster,
		[PassParameters, VertexShader, PixelShader, ViewportSize](FRHICommandList& RHICmdList)
		{
			RHICmdList.SetViewport(0, 0, 0.0f, ViewportSize.X, ViewportSize.Y, 1.0f);

			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

			GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;

			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

			SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PassParameters);

			// Draw fullscreen triangle
			RHICmdList.DrawPrimitive(0, 1, 3);
		});
}
