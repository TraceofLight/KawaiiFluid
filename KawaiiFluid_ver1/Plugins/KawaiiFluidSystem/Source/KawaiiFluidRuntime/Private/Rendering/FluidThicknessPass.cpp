// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Rendering/FluidThicknessPass.h"
#include "Rendering/FluidThicknessShaders.h"
#include "Rendering/KawaiiFluidRenderResource.h"
#include "Modules/KawaiiFluidRenderingModule.h"
#include "Rendering/KawaiiFluidMetaballRenderer.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "SceneView.h"
#include "ScenePrivate.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "CommonRenderResources.h"

//=============================================================================
// Batched Thickness Pass
//=============================================================================

void RenderFluidThicknessPass(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const TArray<UKawaiiFluidMetaballRenderer*>& Renderers,
	FRDGTextureRef SceneDepthTexture,
	FRDGTextureRef& OutThicknessTexture)
{
	RDG_EVENT_SCOPE(GraphBuilder, "FluidThicknessPass_Batched");

	if (Renderers.Num() == 0)
	{
		return;
	}

	// 뷰포트 크기로 텍스처 생성
	FIntPoint TextureSize = View.UnscaledViewRect.Size();

	// Thickness Texture 생성
	FRDGTextureDesc ThicknessDesc = FRDGTextureDesc::Create2D(
		TextureSize,
		PF_R16F,
		FClearValueBinding::Black,
		TexCreate_ShaderResource | TexCreate_RenderTargetable);

	OutThicknessTexture = GraphBuilder.CreateTexture(ThicknessDesc, TEXT("FluidThicknessTexture"));

	AddClearRenderTargetPass(GraphBuilder, OutThicknessTexture, FLinearColor::Black);

	// Get rendering parameters from first renderer's LocalParameters
	// (all renderers in batch have identical parameters - that's why they're batched)
	float ThicknessScale = Renderers[0]->GetLocalParameters().ThicknessScale;
	float ParticleRadius = Renderers[0]->GetLocalParameters().ParticleRenderRadius;

	// 중복 처리 방지를 위해 처리된 RenderResource 추적
	TSet<FKawaiiFluidRenderResource*> ProcessedResources;

	// Render each renderer's particles (batch-specific only)
	for (UKawaiiFluidMetaballRenderer* Renderer : Renderers)
	{
		if (!Renderer) continue;

		FKawaiiFluidRenderResource* RR = Renderer->GetFluidRenderResource();
		if (!RR || !RR->IsValid()) continue;

		// 이미 처리된 RenderResource는 스킵
		if (ProcessedResources.Contains(RR))
		{
			continue;
		}
		ProcessedResources.Add(RR);

		FRDGBufferSRVRef ParticleBufferSRV = nullptr;
		int32 ParticleCount = 0;

		// =====================================================
		// 통합 경로: RenderResource에서 일원화된 데이터 접근
		// GPU/CPU 모두 동일한 버퍼 사용
		// =====================================================
		ParticleCount = RR->GetUnifiedParticleCount();
		if (ParticleCount <= 0)
		{
			continue;
		}

		// Position 버퍼 가져오기 (GPU/CPU 공통 - ViewExtension에서 항상 생성됨)
		TRefCountPtr<FRDGPooledBuffer> PositionPooledBuffer = RR->GetPooledPositionBuffer();
		if (!PositionPooledBuffer.IsValid())
		{
			// ViewExtension에서 버퍼가 생성되지 않았으면 스킵
			continue;
		}

		FRDGBufferRef PositionBuffer = GraphBuilder.RegisterExternalBuffer(
			PositionPooledBuffer,
			TEXT("SSFRThicknessPositions"));
		ParticleBufferSRV = GraphBuilder.CreateSRV(PositionBuffer);

		// 유효한 파티클이 없으면 스킵
		if (!ParticleBufferSRV || ParticleCount == 0)
		{
			continue;
		}

		FMatrix ViewMatrix = View.ViewMatrices.GetViewMatrix();
		FMatrix ProjectionMatrix = View.ViewMatrices.GetProjectionNoAAMatrix();

		auto* PassParameters = GraphBuilder.AllocParameters<FFluidThicknessParameters>();
		PassParameters->ParticlePositions = ParticleBufferSRV;
		PassParameters->ParticleRadius = ParticleRadius;
		PassParameters->ViewMatrix = FMatrix44f(ViewMatrix);
		PassParameters->ProjectionMatrix = FMatrix44f(ProjectionMatrix);
		PassParameters->ThicknessScale = ThicknessScale; // Use from LocalParameters

		// Occlusion test를 위한 SceneDepth 파라미터
		PassParameters->SceneDepthTexture = SceneDepthTexture;
		PassParameters->SceneDepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

		// SceneDepth UV 변환을 위한 ViewRect와 텍스처 크기
		const FViewInfo& ViewInfo = static_cast<const FViewInfo&>(View);
		PassParameters->SceneViewRect = FVector2f(
			ViewInfo.ViewRect.Width(),
			ViewInfo.ViewRect.Height());
		PassParameters->SceneTextureSize = FVector2f(
			SceneDepthTexture->Desc.Extent.X,
			SceneDepthTexture->Desc.Extent.Y);

		PassParameters->RenderTargets[0] = FRenderTargetBinding(
			OutThicknessTexture, ERenderTargetLoadAction::ELoad);

		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
		TShaderMapRef<FFluidThicknessVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<FFluidThicknessPS> PixelShader(GlobalShaderMap);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("ThicknessDraw_Batched"),
			PassParameters,
			ERDGPassFlags::Raster,
			[VertexShader, PixelShader, PassParameters, ParticleCount](
			FRHICommandList& RHICmdList)
			{
				FGraphicsPipelineStateInitializer GraphicsPSOInit;
				GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.
					VertexDeclarationRHI;
				GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
				GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
				GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;

				GraphicsPSOInit.BlendState = TStaticBlendState<
					CW_RED, // R채널만 사용 (R16F)
					BO_Add, BF_One, BF_One, // Color: Add(Src, Dst)
					BO_Add, BF_Zero, BF_One // Alpha: Add(0, 1) -> Alpha는 안 건드림
				>::GetRHI();
				GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
				GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<
					false, CF_Always>::GetRHI();

				RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

				SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
				SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(),
				                    *PassParameters);
				SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(),
				                    *PassParameters);

				RHICmdList.DrawPrimitive(0, 2, ParticleCount);
			});
	}
}
