// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Rendering/FluidThicknessPass.h"
#include "Rendering/FluidThicknessShaders.h"
#include "Rendering/FluidRendererSubsystem.h"
#include "Core/FluidSimulator.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "SceneView.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "CommonRenderResources.h"

void RenderFluidThicknessPass(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	UFluidRendererSubsystem* Subsystem,
	FRDGTextureRef& OutThicknessTexture)
{
	RDG_EVENT_SCOPE(GraphBuilder, "FluidThicknessPass");

	const TArray<AFluidSimulator*>& Simulators = Subsystem->GetRegisteredSimulators();
	if (Simulators.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("KawaiiFluid: FluidThicknessPass - No registered simulators found."));
		return;
	}

	// Thickness Texture 생성 (가산 혼합을 위해 R16F 또는 R32F 사용)
	FRDGTextureDesc ThicknessDesc = FRDGTextureDesc::Create2D(
		View.UnscaledViewRect.Size(),
		PF_R16F,
		FClearValueBinding::Black,
		TexCreate_ShaderResource | TexCreate_RenderTargetable);

	OutThicknessTexture = GraphBuilder.CreateTexture(ThicknessDesc, TEXT("FluidThicknessTexture"));

	// 초기화 (Clear)
	AddClearRenderTargetPass(GraphBuilder, OutThicknessTexture, FLinearColor::Black);

	for (AFluidSimulator* Simulator : Simulators)
	{
		if (!Simulator || Simulator->GetParticleCount() == 0)
		{
			continue;
		}

		UInstancedStaticMeshComponent* MeshComp = Simulator->DebugMeshComponent;
		if (!MeshComp || !MeshComp->IsVisible())
		{
			continue;
		}

		int32 InstanceCount = MeshComp->GetInstanceCount();
		if (InstanceCount == 0)
		{
			continue;
		}

		TArray<FVector3f> ParticlePositions;
		ParticlePositions.Reserve(InstanceCount);

		UE_LOG(LogTemp, Log, TEXT("KawaiiFluid: Rendering FluidThicknessPass for %s. InstanceCount: %d"), 
			*Simulator->GetName(), InstanceCount);

		for (int32 i = 0; i < InstanceCount; ++i)
		{
			FTransform InstanceTransform;
			if (MeshComp->GetInstanceTransform(i, InstanceTransform, true))
			{
				ParticlePositions.Add(FVector3f(InstanceTransform.GetLocation()));
			}
		}

		if (ParticlePositions.Num() == 0)
		{
			continue;
		}

		const uint32 BufferSize = ParticlePositions.Num() * sizeof(FVector3f);
		FRDGBufferDesc BufferDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), ParticlePositions.Num());
		FRDGBufferRef ParticleBuffer = GraphBuilder.CreateBuffer(BufferDesc, TEXT("FluidThicknessParticlePositions"));
		GraphBuilder.QueueBufferUpload(ParticleBuffer, ParticlePositions.GetData(), BufferSize);

		FRDGBufferSRVRef ParticleBufferSRV = GraphBuilder.CreateSRV(ParticleBuffer);

		FMatrix ViewMatrix = View.ViewMatrices.GetViewMatrix();
		FMatrix ProjectionMatrix = View.ViewMatrices.GetProjectionMatrix();

		auto* PassParameters = GraphBuilder.AllocParameters<FFluidThicknessParameters>();
		PassParameters->ParticlePositions = ParticleBufferSRV;
		PassParameters->ParticleRadius = Subsystem->RenderingParameters.ParticleRenderRadius;
		PassParameters->ViewMatrix = FMatrix44f(ViewMatrix);
		PassParameters->ProjectionMatrix = FMatrix44f(ProjectionMatrix);
		PassParameters->ThicknessScale = Subsystem->RenderingParameters.ThicknessScale;
		PassParameters->RenderTargets[0] = FRenderTargetBinding(OutThicknessTexture, ERenderTargetLoadAction::ELoad);

		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
		TShaderMapRef<FFluidThicknessVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<FFluidThicknessPS> PixelShader(GlobalShaderMap);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("FluidThicknessDraw_%s", *Simulator->GetName()),
			PassParameters,
			ERDGPassFlags::Raster,
			[VertexShader, PixelShader, PassParameters, InstanceCount](FRHICommandList& RHICmdList)
			{
				FGraphicsPipelineStateInitializer GraphicsPSOInit;
				RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
				
				// Additive Blending 설정
				GraphicsPSOInit.BlendState = TStaticBlendState<CW_RED, BO_Add, BF_One, BF_One>::GetRHI();
				GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
				GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

				GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
				GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
				GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
				GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;

				SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

				SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), *PassParameters);
				SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), *PassParameters);

				RHICmdList.DrawPrimitive(0, 2, InstanceCount);
			});
	}
}
