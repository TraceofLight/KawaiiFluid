// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Rendering/FluidDepthPass.h"
#include "Rendering/FluidDepthShaders.h"
#include "Rendering/FluidRendererSubsystem.h"
#include "Rendering/IKawaiiFluidRenderable.h"
#include "Rendering/KawaiiFluidRenderResource.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "SceneView.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "CommonRenderResources.h"

//=============================================================================
// Depth Pass Implementation
//=============================================================================

void RenderFluidDepthPass(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	UFluidRendererSubsystem* Subsystem,
	FRDGTextureRef& OutDepthTexture)
{
	RDG_EVENT_SCOPE(GraphBuilder, "FluidDepthPass");

	// Depth Texture 생성
	FRDGTextureDesc DepthDesc = FRDGTextureDesc::Create2D(
		View.UnscaledViewRect.Size(),
		PF_R32_FLOAT,
		FClearValueBinding::Black,
		TexCreate_ShaderResource | TexCreate_RenderTargetable);

	OutDepthTexture = GraphBuilder.CreateTexture(DepthDesc, TEXT("FluidDepthTexture"));


	//=============================================================================
	// SSFR 렌더링만 처리 (DebugMesh는 UE 기본 렌더링 사용)
	//=============================================================================
	
	TArray<IKawaiiFluidRenderable*> Renderables = Subsystem->GetAllRenderables();
	bool bFirstPass = true;

	for (IKawaiiFluidRenderable* Renderable : Renderables)
	{
		if (!Renderable)
		{
			continue;
		}

		// SSFR 모드만 처리 (DebugMesh 모드는 스킵)
		if (!Renderable->ShouldUseSSFR())
		{
			continue;
		}

		if (!Renderable->IsFluidRenderResourceValid())
		{
			continue;
		}

		FKawaiiFluidRenderResource* RR = Renderable->GetFluidRenderResource();
		const TArray<FKawaiiRenderParticle>& CachedParticles = RR->GetCachedParticles();

		if (CachedParticles.Num() == 0)
		{
			continue;
		}

		UE_LOG(LogTemp, Log, TEXT("✅ DepthPass (SSFR): %s with %d particles"),
			*Renderable->GetDebugName(), CachedParticles.Num());

		// Position만 추출
		TArray<FVector3f> ParticlePositions;
		ParticlePositions.Reserve(CachedParticles.Num());
		for (const FKawaiiRenderParticle& Particle : CachedParticles)
		{
			ParticlePositions.Add(Particle.Position);
		}

		// RDG 버퍼 생성 및 업로드
		const uint32 BufferSize = ParticlePositions.Num() * sizeof(FVector3f);
		FRDGBufferDesc BufferDesc = FRDGBufferDesc::CreateStructuredDesc(
			sizeof(FVector3f), ParticlePositions.Num());
		FRDGBufferRef ParticleBuffer = GraphBuilder.CreateBuffer(
			BufferDesc, TEXT("SSFRParticlePositions"));

		GraphBuilder.QueueBufferUpload(ParticleBuffer, ParticlePositions.GetData(), BufferSize);
		FRDGBufferSRVRef ParticleBufferSRV = GraphBuilder.CreateSRV(ParticleBuffer);

		// View matrices
		FMatrix ViewMatrix = View.ViewMatrices.GetViewMatrix();
		FMatrix ProjectionMatrix = View.ViewMatrices.GetProjectionMatrix();
		FMatrix ViewProjectionMatrix = View.ViewMatrices.GetViewProjectionMatrix();
		float ParticleRadius = Renderable->GetParticleRenderRadius();

		// Shader Parameters
		auto* PassParameters = GraphBuilder.AllocParameters<FFluidDepthParameters>();
		PassParameters->ParticlePositions = ParticleBufferSRV;
		PassParameters->ParticleRadius = ParticleRadius;
		PassParameters->ViewMatrix = FMatrix44f(ViewMatrix);
		PassParameters->ProjectionMatrix = FMatrix44f(ProjectionMatrix);
		PassParameters->ViewProjectionMatrix = FMatrix44f(ViewProjectionMatrix);
		PassParameters->RenderTargets[0] = FRenderTargetBinding(
			OutDepthTexture,
			bFirstPass ? ERenderTargetLoadAction::EClear : ERenderTargetLoadAction::ELoad
		);

		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
		TShaderMapRef<FFluidDepthVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<FFluidDepthPS> PixelShader(GlobalShaderMap);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("DepthDraw_SSFR_%s", *Renderable->GetDebugName()),
			PassParameters,
			ERDGPassFlags::Raster,
			[VertexShader, PixelShader, PassParameters, ParticleCount = ParticlePositions.Num()](FRHICommandList& RHICmdList)
			{
				FGraphicsPipelineStateInitializer GraphicsPSOInit;
				RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
				GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
				GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
				GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

				GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
				GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
				GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
				GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;

				SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
				SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), *PassParameters);
				SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PassParameters);

				RHICmdList.DrawPrimitive(0, 2, ParticleCount);
			});

		bFirstPass = false;
	}
}
