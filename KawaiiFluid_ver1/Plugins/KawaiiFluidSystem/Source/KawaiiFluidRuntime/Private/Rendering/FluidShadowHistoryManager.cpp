// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Rendering/FluidShadowHistoryManager.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "SceneView.h"

/**
 * @brief Default constructor. Initializes both history buffers to invalid state.
 */
FFluidShadowHistoryManager::FFluidShadowHistoryManager()
	: CurrentBufferIndex(0)
{
	HistoryBuffers[0].Reset();
	HistoryBuffers[1].Reset();
}

/**
 * @brief Destructor. Releases pooled render targets.
 */
FFluidShadowHistoryManager::~FFluidShadowHistoryManager()
{
	Reset();
}

/**
 * @brief Get the history buffer from the previous frame.
 * @return Reference to the previous frame's history buffer.
 */
const FFluidShadowHistoryBuffer& FFluidShadowHistoryManager::GetPreviousFrameBuffer() const
{
	return HistoryBuffers[GetReadBufferIndex()];
}

/**
 * @brief Check if we have valid history from previous frame.
 * @return True if previous frame history is valid and usable.
 */
bool FFluidShadowHistoryManager::HasValidHistory() const
{
	const FFluidShadowHistoryBuffer& ReadBuffer = HistoryBuffers[GetReadBufferIndex()];
	return ReadBuffer.bIsValid && ReadBuffer.DepthTexture.IsValid();
}

/**
 * @brief Store current frame's depth and matrices for next frame.
 * @param GraphBuilder RDG builder for texture extraction.
 * @param SmoothedDepthTexture Current frame's smoothed depth texture.
 * @param View Scene view containing camera matrices.
 */
void FFluidShadowHistoryManager::StoreCurrentFrame(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef SmoothedDepthTexture,
	const FSceneView& View)
{
	if (!SmoothedDepthTexture)
	{
		return;
	}

	FFluidShadowHistoryBuffer& WriteBuffer = HistoryBuffers[GetWriteBufferIndex()];

	// Store camera matrices
	WriteBuffer.ViewProjectionMatrix = FMatrix44f(View.ViewMatrices.GetViewProjectionMatrix());
	WriteBuffer.InvViewProjectionMatrix = FMatrix44f(View.ViewMatrices.GetInvViewProjectionMatrix());
	WriteBuffer.ViewportRect = View.UnscaledViewRect;

	// Extract depth texture to pooled render target for persistence across frames
	const FRDGTextureDesc& Desc = SmoothedDepthTexture->Desc;

	// Create or reuse pooled render target
	FPooledRenderTargetDesc RTDesc = FPooledRenderTargetDesc::Create2DDesc(
		Desc.Extent,
		Desc.Format,
		FClearValueBinding::None,
		TexCreate_None,
		TexCreate_ShaderResource | TexCreate_UAV,
		false
	);
	RTDesc.DebugName = TEXT("FluidShadowHistoryDepth");

	// Allocate or reuse the pooled render target
	if (!WriteBuffer.DepthTexture.IsValid() ||
		WriteBuffer.DepthTexture->GetDesc().Extent != Desc.Extent)
	{
		GRenderTargetPool.FindFreeElement(
			GraphBuilder.RHICmdList,
			RTDesc,
			WriteBuffer.DepthTexture,
			TEXT("FluidShadowHistoryDepth")
		);
	}

	if (WriteBuffer.DepthTexture.IsValid())
	{
		// Queue copy from RDG texture to pooled render target
		FRDGTextureRef TargetTexture = GraphBuilder.RegisterExternalTexture(
			WriteBuffer.DepthTexture,
			TEXT("FluidShadowHistoryDepthTarget")
		);

		AddCopyTexturePass(GraphBuilder, SmoothedDepthTexture, TargetTexture);

		WriteBuffer.bIsValid = true;
	}
}

/**
 * @brief Register the history depth texture for RDG use.
 * @param GraphBuilder RDG builder for texture registration.
 * @return RDG texture reference, or nullptr if no valid history.
 */
FRDGTextureRef FFluidShadowHistoryManager::RegisterHistoryDepthTexture(FRDGBuilder& GraphBuilder) const
{
	const FFluidShadowHistoryBuffer& ReadBuffer = HistoryBuffers[GetReadBufferIndex()];

	if (!ReadBuffer.bIsValid || !ReadBuffer.DepthTexture.IsValid())
	{
		return nullptr;
	}

	return GraphBuilder.RegisterExternalTexture(
		ReadBuffer.DepthTexture,
		TEXT("FluidShadowHistoryDepth")
	);
}

/**
 * @brief Swap buffers at the beginning of each frame.
 */
void FFluidShadowHistoryManager::BeginFrame()
{
	// Swap the buffer index
	CurrentBufferIndex = 1 - CurrentBufferIndex;

	// Reset the new write buffer (it will be filled this frame)
	HistoryBuffers[GetWriteBufferIndex()].bIsValid = false;
}

/**
 * @brief Reset all history data.
 */
void FFluidShadowHistoryManager::Reset()
{
	HistoryBuffers[0].Reset();
	HistoryBuffers[1].Reset();
	CurrentBufferIndex = 0;
}
