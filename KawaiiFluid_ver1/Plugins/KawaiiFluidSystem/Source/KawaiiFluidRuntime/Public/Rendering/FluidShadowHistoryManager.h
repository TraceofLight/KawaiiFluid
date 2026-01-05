// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderGraphResources.h"
#include "RenderGraphBuilder.h"
#include "RendererInterface.h"

/**
 * @brief History buffer data for a single frame.
 * @param DepthTexture Pooled render target containing smoothed fluid depth from previous frame.
 * @param ViewProjectionMatrix View-Projection matrix from the frame when depth was captured.
 * @param InvViewProjectionMatrix Inverse View-Projection matrix for world position reconstruction.
 * @param ViewportRect Viewport rectangle at capture time.
 * @param bIsValid Whether this history buffer contains valid data.
 */
struct FFluidShadowHistoryBuffer
{
	/** Smoothed depth texture from previous frame (persists across frames) */
	TRefCountPtr<IPooledRenderTarget> DepthTexture;

	/** View-Projection matrix at capture time */
	FMatrix44f ViewProjectionMatrix;

	/** Inverse View-Projection matrix for world position reconstruction */
	FMatrix44f InvViewProjectionMatrix;

	/** Viewport rectangle at capture time */
	FIntRect ViewportRect;

	/** Whether this buffer contains valid data */
	bool bIsValid = false;

	void Reset()
	{
		DepthTexture = nullptr;
		ViewProjectionMatrix = FMatrix44f::Identity;
		InvViewProjectionMatrix = FMatrix44f::Identity;
		ViewportRect = FIntRect();
		bIsValid = false;
	}
};

/**
 * @brief Manages history buffers for fluid shadow projection.
 *
 * Uses double buffering to store previous frame's SSFR depth and camera matrices.
 * This data is used to project fluid depth into light space for VSM shadow generation.
 *
 * @param HistoryBuffers Double-buffered history data (ping-pong).
 * @param CurrentBufferIndex Index of the buffer being written to this frame.
 */
class KAWAIIFLUIDRUNTIME_API FFluidShadowHistoryManager
{
public:
	FFluidShadowHistoryManager();
	~FFluidShadowHistoryManager();

	/**
	 * Get the history buffer from the previous frame (for reading).
	 * @return Previous frame's history buffer, may be invalid on first frame.
	 */
	const FFluidShadowHistoryBuffer& GetPreviousFrameBuffer() const;

	/**
	 * Check if previous frame history is available and valid.
	 * @return True if we have valid history data to use for shadow projection.
	 */
	bool HasValidHistory() const;

	/**
	 * Store current frame's depth and matrices for use in next frame.
	 * Call this at the end of fluid rendering.
	 *
	 * @param GraphBuilder RDG builder for texture extraction.
	 * @param SmoothedDepthTexture Current frame's smoothed depth texture.
	 * @param View Scene view containing camera matrices.
	 */
	void StoreCurrentFrame(
		FRDGBuilder& GraphBuilder,
		FRDGTextureRef SmoothedDepthTexture,
		const FSceneView& View);

	/**
	 * Register external texture for RDG use.
	 * Call this at the beginning of shadow projection pass.
	 *
	 * @param GraphBuilder RDG builder for texture registration.
	 * @return RDG texture reference to previous frame's depth, or nullptr if no history.
	 */
	FRDGTextureRef RegisterHistoryDepthTexture(FRDGBuilder& GraphBuilder) const;

	/**
	 * Called at the start of each frame to swap buffers.
	 * The previous "current" buffer becomes the new "history" buffer.
	 */
	void BeginFrame();

	/**
	 * Reset all history data. Call on level change or when fluid system is disabled.
	 */
	void Reset();

private:
	/** Double-buffered history data */
	FFluidShadowHistoryBuffer HistoryBuffers[2];

	/** Index of the buffer being written to this frame (0 or 1) */
	int32 CurrentBufferIndex = 0;

	/** Get the buffer index for reading (previous frame) */
	int32 GetReadBufferIndex() const { return 1 - CurrentBufferIndex; }

	/** Get the buffer index for writing (current frame) */
	int32 GetWriteBufferIndex() const { return CurrentBufferIndex; }
};
