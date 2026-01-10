// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Rendering/Pipeline/KawaiiMetaballRayMarchPipeline.h"
#include "Rendering/KawaiiFluidMetaballRenderer.h"
#include "Rendering/KawaiiFluidRenderResource.h"
#include "GPU/FluidAnisotropyComputeShader.h"
#include "Core/KawaiiRenderParticle.h"
#include "Rendering/Shaders/FluidSpatialHashShaders.h"
#include "Rendering/Shaders/ExtractRenderPositionsShaders.h"

// Separated shading implementation
#include "Rendering/Shading/KawaiiRayMarchShadingImpl.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphEvent.h"
#include "RenderGraphUtils.h"
#include "RHICommandList.h"
#include "SceneView.h"
#include "ScenePrivate.h"
#include "SceneTextures.h"

void FKawaiiMetaballRayMarchPipeline::ProcessPendingBoundsReadback()
{
	// Process readback from previous frame (if available)
	if (bHasPendingBoundsReadback && PendingBoundsReadbackBuffer.IsValid())
	{
		// Map the pooled buffer and read bounds using RHI command list
		FRHIBuffer* BufferRHI = PendingBoundsReadbackBuffer->GetRHI();
		if (BufferRHI)
		{
			// Get the immediate command list for buffer operations
			FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

			// Lock buffer for reading
			void* MappedData = RHICmdList.LockBuffer(BufferRHI, 0, sizeof(FVector3f) * 2, RLM_ReadOnly);
			if (MappedData)
			{
				FVector3f* BoundsData = static_cast<FVector3f*>(MappedData);
				FVector3f ReadMin = BoundsData[0];
				FVector3f ReadMax = BoundsData[1];
				RHICmdList.UnlockBuffer(BufferRHI);

				// Validate bounds (check for NaN or extreme values)
				bool bValidBounds = !ReadMin.ContainsNaN() && !ReadMax.ContainsNaN() &&
					ReadMin.X < ReadMax.X && ReadMin.Y < ReadMax.Y && ReadMin.Z < ReadMax.Z;

				if (bValidBounds)
				{
					// Update cached bounds for this frame
					SDFVolumeManager.UpdateCachedBoundsFromReadback(ReadMin, ReadMax);

					// Debug log - every frame for debugging
					FVector3f Size = ReadMax - ReadMin;
					//UE_LOG(LogTemp, Log, TEXT("[Bounds Readback] Min(%.1f, %.1f, %.1f) Max(%.1f, %.1f, %.1f) Size(%.1f, %.1f, %.1f)"),ReadMin.X, ReadMin.Y, ReadMin.Z,ReadMax.X, ReadMax.Y, ReadMax.Z,Size.X, Size.Y, Size.Z);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("RayMarchPipeline: Invalid GPU bounds detected (Min: %.1f,%.1f,%.1f Max: %.1f,%.1f,%.1f)"),
						ReadMin.X, ReadMin.Y, ReadMin.Z,
						ReadMax.X, ReadMax.Y, ReadMax.Z);
				}
			}
		}
		bHasPendingBoundsReadback = false;
	}
}

bool FKawaiiMetaballRayMarchPipeline::PrepareParticleBuffer(
	FRDGBuilder& GraphBuilder,
	const FFluidRenderingParameters& RenderParams,
	const TArray<UKawaiiFluidMetaballRenderer*>& Renderers)
{
	// Process readback from previous frame first
	ProcessPendingBoundsReadback();

	// =====================================================
	// 통합 경로: RenderResource에서 일원화된 데이터 접근
	// GPU/CPU 모두 동일한 버퍼 사용 (ViewExtension에서 추출됨)
	// =====================================================

	float AverageParticleRadius = 10.0f;
	float TotalRadius = 0.0f;
	int32 ValidCount = 0;
	int32 TotalParticleCount = 0;
	FRDGBufferSRVRef ParticleBufferSRV = nullptr;
	FRDGBufferSRVRef CachedGPUBoundsBufferSRV = nullptr;

	// 중복 처리 방지를 위해 처리된 RenderResource 추적
	TSet<FKawaiiFluidRenderResource*> ProcessedResources;

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

		// 통합 파티클 수 가져오기
		int32 ParticleCount = RR->GetUnifiedParticleCount();
		if (ParticleCount <= 0)
		{
			continue;
		}

		float ParticleRadius = RR->GetUnifiedParticleRadius();
		TotalRadius += ParticleRadius;
		ValidCount++;

		// =====================================================
		// 통합 경로: RenderResource에서 버퍼 가져오기
		// GPU/CPU 모두 ViewExtension에서 버퍼 생성됨
		// =====================================================
		TRefCountPtr<FRDGPooledBuffer> RenderParticlePooled = RR->GetPooledRenderParticleBuffer();
		TRefCountPtr<FRDGPooledBuffer> BoundsPooled = RR->GetPooledBoundsBuffer();
		TRefCountPtr<FRDGPooledBuffer> PositionPooled = RR->GetPooledPositionBuffer();

		// ViewExtension에서 버퍼가 생성되지 않았으면 스킵
		if (!RenderParticlePooled.IsValid() || !BoundsPooled.IsValid() || !PositionPooled.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[RayMarchPipeline] Buffers not ready for Renderer=%s"), *Renderer->GetName());
			continue;
		}

		bool bIsGPUMode = RR->HasGPUSimulator();
		UE_LOG(LogTemp, Verbose, TEXT("[RayMarchPipeline] Renderer=%s, Mode=%s, ParticleCount=%d"),
			*Renderer->GetName(),
			bIsGPUMode ? TEXT("GPU") : TEXT("CPU"),
			ParticleCount);

		// Register buffers (GPU/CPU 공통)
		FRDGBufferRef RenderParticleBuffer = GraphBuilder.RegisterExternalBuffer(
			RenderParticlePooled, TEXT("RenderParticles_FromRR"));
		FRDGBufferRef BoundsBuffer = GraphBuilder.RegisterExternalBuffer(
			BoundsPooled, TEXT("ParticleBounds_FromRR"));
		FRDGBufferRef PositionBuffer = GraphBuilder.RegisterExternalBuffer(
			PositionPooled, TEXT("PositionsSoA_FromRR"));

		ParticleBufferSRV = GraphBuilder.CreateSRV(RenderParticleBuffer);
		CachedGPUBoundsBufferSRV = GraphBuilder.CreateSRV(BoundsBuffer);
		CachedPipelineData.PositionBufferSRV = GraphBuilder.CreateSRV(PositionBuffer);
		CachedPipelineData.bUseSoABuffers = true;

		TotalParticleCount = ParticleCount;
		AverageParticleRadius = ParticleRadius;

		// Anisotropy (GPU 모드에서만 유효)
		if (RenderParams.AnisotropyParams.bEnabled && bIsGPUMode)
		{
			FRDGBufferSRVRef Axis1SRV, Axis2SRV, Axis3SRV;
			bool bHasAnisotropy = RR->GetAnisotropyBufferSRVs(GraphBuilder, Axis1SRV, Axis2SRV, Axis3SRV);

			if (bHasAnisotropy)
			{
				CachedPipelineData.AnisotropyData.bUseAnisotropy = true;
				CachedPipelineData.AnisotropyData.AnisotropyAxis1SRV = Axis1SRV;
				CachedPipelineData.AnisotropyData.AnisotropyAxis2SRV = Axis2SRV;
				CachedPipelineData.AnisotropyData.AnisotropyAxis3SRV = Axis3SRV;
				UE_LOG(LogTemp, Verbose, TEXT("  >>> ANISOTROPY: Enabled via RenderResource"));
			}
			else
			{
				CachedPipelineData.AnisotropyData.Reset();
			}
		}
		else
		{
			CachedPipelineData.AnisotropyData.Reset();
		}

		UE_LOG(LogTemp, Verbose, TEXT("  >>> Using unified buffers from RenderResource (%d particles)"), ParticleCount);
	}

	if (ValidCount > 0)
	{
		AverageParticleRadius = TotalRadius / ValidCount;
	}

	if (TotalParticleCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("FKawaiiMetaballRayMarchPipeline: No particles - skipping"));
		CachedPipelineData.Reset();
		return false;
	}

	// Build pipeline data
	CachedPipelineData.ParticleBufferSRV = ParticleBufferSRV;
	CachedPipelineData.ParticleCount = TotalParticleCount;
	CachedPipelineData.ParticleRadius = AverageParticleRadius;

	// Check if SDF Volume optimization is enabled
	// GPU mode now supports SDF Volume - ExtractRenderData validates particle positions
	const bool bUseSDFVolume = RenderParams.bUseSDFVolumeOptimization;

	if (bUseSDFVolume)
	{
		// Set volume resolution from parameters
		int32 Resolution = FMath::Clamp(RenderParams.SDFVolumeResolution, 32, 256);
		SDFVolumeManager.SetVolumeResolution(FIntVector(Resolution, Resolution, Resolution));

		UE_LOG(LogTemp, Verbose, TEXT("KawaiiFluid: Using SDF Volume optimization (%dx%dx%d)"),
			Resolution, Resolution, Resolution);

		// ========== HYBRID MODE: Build Spatial Hash for precise final evaluation ==========
		// We build it here so SDF Volume Bake can also use it for acceleration
		const bool bUseSpatialHash = RenderParams.bUseSpatialHash;
		if (bUseSpatialHash)
		{
			RDG_EVENT_SCOPE(GraphBuilder, "SpatialHashBuild_Hybrid");

			// Cell size = SearchRadius to ensure 3x3x3 search covers all neighbors
			// SearchRadius = ParticleRadius * 2.0 + Smoothness
			float SearchRadius = AverageParticleRadius * 2.0f + RenderParams.SDFSmoothness;
			float CellSize = SearchRadius;

			// Step 1: Extract float3 positions from FKawaiiRenderParticle buffer
			// (If already in SoA mode, we can use CachedPipelineData.PositionBufferSRV directly)
			FRDGBufferSRVRef PositionSRV = CachedPipelineData.PositionBufferSRV;

			if (!CachedPipelineData.bUseSoABuffers || !PositionSRV)
			{
				FRDGBufferRef PositionBuffer = GraphBuilder.CreateBuffer(
					FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), TotalParticleCount),
					TEXT("SpatialHash.ExtractedPositions"));
				FRDGBufferUAVRef PositionUAV = GraphBuilder.CreateUAV(PositionBuffer);
				PositionSRV = GraphBuilder.CreateSRV(PositionBuffer);

				FExtractRenderPositionsPassBuilder::AddExtractPositionsPass(
					GraphBuilder,
					ParticleBufferSRV,
					PositionUAV,
					TotalParticleCount);
			}

			// Step 2: Build Multi-pass Spatial Hash with extracted positions
			FSpatialHashMultipassResources HashResources;
			bool bHashSuccess = FSpatialHashBuilder::CreateAndBuildHashMultipass(
				GraphBuilder,
				PositionSRV,
				TotalParticleCount,
				CellSize,
				HashResources);

			if (bHashSuccess && HashResources.IsValid())
			{
				CachedPipelineData.SpatialHashData.bUseSpatialHash = true;
				CachedPipelineData.SpatialHashData.CellDataSRV = HashResources.CellDataSRV;
				CachedPipelineData.SpatialHashData.ParticleIndicesSRV = HashResources.ParticleIndicesSRV;
				CachedPipelineData.SpatialHashData.CellSize = CellSize;

				UE_LOG(LogTemp, Verbose, TEXT("KawaiiFluid: HYBRID MODE - SDF Volume + Spatial Hash (%d particles, CellSize: %.2f)"),
					TotalParticleCount, CellSize);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("KawaiiFluid: Spatial Hash build failed for Hybrid mode, using SDF Volume only"));
				CachedPipelineData.SpatialHashData.bUseSpatialHash = false;
			}
		}
		else
		{
			CachedPipelineData.SpatialHashData.bUseSpatialHash = false;
		}

		RDG_EVENT_SCOPE(GraphBuilder, "SDFVolumeBake");

		FRDGTextureSRVRef SDFVolumeSRV = nullptr;
		FVector3f VolumeMin, VolumeMax;
		bool bUseGPUBoundsForRayMarch = false;

		// Spatial Hash SRVs (if enabled)
		FRDGBufferSRVRef CellDataSRV = CachedPipelineData.SpatialHashData.bUseSpatialHash ? CachedPipelineData.SpatialHashData.CellDataSRV : nullptr;
		FRDGBufferSRVRef ParticleIndicesSRV = CachedPipelineData.SpatialHashData.bUseSpatialHash ? CachedPipelineData.SpatialHashData.ParticleIndicesSRV : nullptr;
		float SpatialHashCellSize = CachedPipelineData.SpatialHashData.bUseSpatialHash ? CachedPipelineData.SpatialHashData.CellSize : 0.0f;

		// 통합 경로: GPU/CPU 모두 BoundsBuffer 사용
		if (CachedGPUBoundsBufferSRV)
		{
			// Use bounds from GPU buffer directly (no readback latency!)
			// Both SDF Bake and Ray March will read bounds from the same GPU buffer
			SDFVolumeSRV = SDFVolumeManager.BakeSDFVolumeWithGPUBoundsDirect(
				GraphBuilder,
				ParticleBufferSRV,
				TotalParticleCount,
				AverageParticleRadius,
				RenderParams.SDFSmoothness,
				CachedGPUBoundsBufferSRV,
				CachedPipelineData.PositionBufferSRV,
				CellDataSRV,
				ParticleIndicesSRV,
				SpatialHashCellSize);

			// For debug visualization, we don't have exact bounds (they're in GPU buffer)
			// Use placeholder values - actual rendering uses GPU buffer
			VolumeMin = FVector3f(-500.0f, -500.0f, -500.0f);
			VolumeMax = FVector3f(500.0f, 500.0f, 500.0f);
			bUseGPUBoundsForRayMarch = true;

			UE_LOG(LogTemp, Verbose, TEXT("SDF Bake + Ray March using GPU bounds buffer (zero latency)"));
		}
		else
		{
			// BoundsBuffer가 없으면 스킵 (ViewExtension에서 생성되어야 함)
			UE_LOG(LogTemp, Warning, TEXT("KawaiiFluid: BoundsBuffer not ready, skipping SDF bake"));
			CachedPipelineData.Reset();
			return false;
		}

		// Store SDF volume data
		CachedPipelineData.SDFVolumeData.SDFVolumeTextureSRV = SDFVolumeSRV;
		CachedPipelineData.SDFVolumeData.VolumeMin = VolumeMin;
		CachedPipelineData.SDFVolumeData.VolumeMax = VolumeMax;
		CachedPipelineData.SDFVolumeData.VolumeResolution = SDFVolumeManager.GetVolumeResolution();
		CachedPipelineData.SDFVolumeData.bUseSDFVolume = true;
		CachedPipelineData.SDFVolumeData.bUseGPUBounds = bUseGPUBoundsForRayMarch;
		CachedPipelineData.SDFVolumeData.BoundsBufferSRV = CachedGPUBoundsBufferSRV;

		// Notify renderers of SDF volume bounds for debug visualization
		for (UKawaiiFluidMetaballRenderer* Renderer : Renderers)
		{
			if (Renderer && Renderer->GetLocalParameters().bDebugDrawSDFVolume)
			{
				Renderer->SetSDFVolumeBounds(FVector(VolumeMin), FVector(VolumeMax));
			}
		}
	}
	else
	{
		// No SDF Volume: Direct O(N) particle iteration (legacy mode)
		CachedPipelineData.SDFVolumeData.bUseSDFVolume = false;
		CachedPipelineData.SpatialHashData.bUseSpatialHash = false;
		UE_LOG(LogTemp, Verbose, TEXT("KawaiiFluid: Using direct particle iteration (legacy O(N))"));
	}

	return true;
}

void FKawaiiMetaballRayMarchPipeline::ExecutePostBasePass(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FFluidRenderingParameters& RenderParams,
	const TArray<UKawaiiFluidMetaballRenderer*>& Renderers,
	FRDGTextureRef SceneDepthTexture)
{
	if (Renderers.Num() == 0)
	{
		return;
	}

	// PostProcess mode uses PrepareForTonemap + ExecuteTonemap at Tonemap timing
	if (RenderParams.ShadingMode == EMetaballShadingMode::PostProcess)
	{
		// Nothing to do here - PostProcess mode preparation happens in PrepareForTonemap
		return;
	}

	RDG_EVENT_SCOPE(GraphBuilder, "MetaballPipeline_RayMarching_PostBasePass");

	// Prepare particle buffer for GBuffer/Translucent modes
	if (!PrepareParticleBuffer(GraphBuilder, RenderParams, Renderers))
	{
		return;
	}

	// ShadingMode-specific processing at PostBasePass timing
	// Delegate to separated shading implementation
	switch (RenderParams.ShadingMode)
	{
	case EMetaballShadingMode::GBuffer:
	case EMetaballShadingMode::Opaque:
		KawaiiRayMarchShading::RenderGBufferShading(
			GraphBuilder, View, RenderParams, CachedPipelineData, SceneDepthTexture);
		break;

	case EMetaballShadingMode::Translucent:
		KawaiiRayMarchShading::RenderTranslucentGBufferWrite(
			GraphBuilder, View, RenderParams, CachedPipelineData, SceneDepthTexture);
		break;

	default:
		break;
	}

	UE_LOG(LogTemp, Verbose, TEXT("KawaiiFluid: RayMarching PostBasePass completed (%d particles, ShadingMode=%d)"),
		CachedPipelineData.ParticleCount, static_cast<int32>(RenderParams.ShadingMode));
}

void FKawaiiMetaballRayMarchPipeline::PrepareRender(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FFluidRenderingParameters& RenderParams,
	const TArray<UKawaiiFluidMetaballRenderer*>& Renderers,
	FRDGTextureRef SceneDepthTexture)
{
	if (Renderers.Num() == 0)
	{
		return;
	}

	RDG_EVENT_SCOPE(GraphBuilder, "MetaballPipeline_RayMarching_PrepareForTonemap");

	// Prepare particle buffer and optional SDF volume for PostProcess shading
	if (!PrepareParticleBuffer(GraphBuilder, RenderParams, Renderers))
	{
		return;
	}

	UE_LOG(LogTemp, Verbose, TEXT("KawaiiFluid: RayMarching PrepareForTonemap completed (%d particles)"),
		CachedPipelineData.ParticleCount);
}

void FKawaiiMetaballRayMarchPipeline::ExecutePrePostProcess(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FFluidRenderingParameters& RenderParams,
	const TArray<UKawaiiFluidMetaballRenderer*>& Renderers,
	FRDGTextureRef SceneDepthTexture,
	FRDGTextureRef SceneColorTexture,
	FScreenPassRenderTarget Output,
	FRDGTextureRef GBufferATexture,
	FRDGTextureRef GBufferDTexture)
{
	if (Renderers.Num() == 0)
	{
		return;
	}

	// Only Translucent mode uses PrePostProcess timing
	if (RenderParams.ShadingMode != EMetaballShadingMode::Translucent)
	{
		return;
	}

	// Validate cached pipeline data
	if (!CachedPipelineData.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("FKawaiiMetaballRayMarchPipeline: Missing cached pipeline data for PrePostProcess"));
		return;
	}

	RDG_EVENT_SCOPE(GraphBuilder, "MetaballPipeline_RayMarching_PrePostProcess");

	// Delegate to separated shading implementation
	KawaiiRayMarchShading::RenderTranslucentTransparency(
		GraphBuilder, View, RenderParams,
		SceneDepthTexture, SceneColorTexture, Output,
		GBufferATexture, GBufferDTexture);

	UE_LOG(LogTemp, Verbose, TEXT("KawaiiFluid: RayMarching PrePostProcess executed"));
}

void FKawaiiMetaballRayMarchPipeline::ExecuteRender(
	FRDGBuilder& GraphBuilder,
	const FSceneView& View,
	const FFluidRenderingParameters& RenderParams,
	const TArray<UKawaiiFluidMetaballRenderer*>& Renderers,
	FRDGTextureRef SceneDepthTexture,
	FRDGTextureRef SceneColorTexture,
	FScreenPassRenderTarget Output)
{
	if (Renderers.Num() == 0)
	{
		return;
	}

	// Only PostProcess mode uses Tonemap timing
	if (RenderParams.ShadingMode != EMetaballShadingMode::PostProcess)
	{
		return;
	}

	// Validate cached pipeline data
	if (!CachedPipelineData.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("FKawaiiMetaballRayMarchPipeline: Missing cached pipeline data for Tonemap"));
		return;
	}

	RDG_EVENT_SCOPE(GraphBuilder, "MetaballPipeline_RayMarching_Tonemap");

	// Reset intermediate textures from previous frame
	CachedIntermediateTextures.Reset();

	// Check if shadows are enabled (need depth output for VSM shadow projection)
	const bool bOutputDepth = RenderParams.bEnableShadowCasting && RenderParams.ShadowIntensity > 0.0f;
	FRDGTextureRef FluidDepthTexture = nullptr;

	// Delegate to separated shading implementation
	KawaiiRayMarchShading::RenderPostProcessShading(
		GraphBuilder, View, RenderParams, CachedPipelineData,
		SceneDepthTexture, SceneColorTexture, Output,
		bOutputDepth, &FluidDepthTexture);

	// Store depth texture for shadow history (reuse SmoothedDepthTexture field)
	if (bOutputDepth && FluidDepthTexture)
	{
		CachedIntermediateTextures.SmoothedDepthTexture = FluidDepthTexture;
		UE_LOG(LogTemp, Log, TEXT("KawaiiFluid: RayMarching depth output stored for shadow projection"));
	}

	UE_LOG(LogTemp, Verbose, TEXT("KawaiiFluid: RayMarching Tonemap executed"));
}
