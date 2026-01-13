// Copyright KawaiiFluid Team. All Rights Reserved.
// FGPUCollisionFeedbackManager - Collision feedback system with async GPU readback

#pragma once

#include "CoreMinimal.h"
#include "RenderGraphResources.h"
#include "RHIResources.h"
#include "GPU/GPUFluidParticle.h"

class FRHICommandListImmediate;
class FRHIGPUBufferReadback;
class FRDGBuilder;

/**
 * FGPUCollisionFeedbackManager
 *
 * Manages GPU collision feedback system for particle-collider interaction.
 * Uses triple-buffered async GPU readback (FRHIGPUBufferReadback) for
 * efficient GPU->CPU data transfer without pipeline stalls.
 *
 * Features:
 * - Collision feedback entries (position, velocity, force for each collision)
 * - Per-collider contact counts (simple collision detection)
 * - Async readback with no FlushRenderingCommands
 */
class KAWAIIFLUIDRUNTIME_API FGPUCollisionFeedbackManager
{
public:
	FGPUCollisionFeedbackManager();
	~FGPUCollisionFeedbackManager();

	//=========================================================================
	// Constants
	//=========================================================================

	static constexpr int32 MAX_COLLISION_FEEDBACK = 1024;
	static constexpr int32 NUM_FEEDBACK_BUFFERS = 3;
	static constexpr int32 MAX_COLLIDER_COUNT = 256;

	//=========================================================================
	// Lifecycle
	//=========================================================================

	/** Initialize the feedback manager */
	void Initialize();

	/** Release all resources */
	void Release();

	/** Check if ready */
	bool IsReady() const { return bIsInitialized; }

	//=========================================================================
	// Enable/Disable
	//=========================================================================

	/** Enable or disable collision feedback recording */
	void SetEnabled(bool bEnabled) { bFeedbackEnabled = bEnabled; }

	/** Check if collision feedback is enabled */
	bool IsEnabled() const { return bFeedbackEnabled; }

	//=========================================================================
	// Buffer Management (called from render thread)
	//=========================================================================

	/** Allocate readback objects (call from render thread) */
	void AllocateReadbackObjects(FRHICommandListImmediate& RHICmdList);

	/** Release readback objects */
	void ReleaseReadbackObjects();

	/** Get or create feedback buffer for RDG pass */
	TRefCountPtr<FRDGPooledBuffer>& GetFeedbackBuffer() { return CollisionFeedbackBuffer; }

	/** Get or create counter buffer for RDG pass */
	TRefCountPtr<FRDGPooledBuffer>& GetCounterBuffer() { return CollisionCounterBuffer; }

	/** Get or create contact count buffer for RDG pass */
	TRefCountPtr<FRDGPooledBuffer>& GetContactCountBuffer() { return ColliderContactCountBuffer; }

	//=========================================================================
	// Readback Processing (called from render thread after simulation)
	//=========================================================================

	/** Process collision feedback readback (non-blocking) */
	void ProcessFeedbackReadback(FRHICommandListImmediate& RHICmdList);

	/** Process contact count readback (non-blocking) */
	void ProcessContactCountReadback(FRHICommandListImmediate& RHICmdList);

	/** Enqueue copy for next frame's readback */
	void EnqueueReadbackCopy(FRHICommandListImmediate& RHICmdList);

	/** Increment frame counter (call after EnqueueCopy) */
	void IncrementFrameCounter();

	/** Get current write index for triple buffering */
	int32 GetCurrentWriteIndex() const { return CurrentWriteIndex; }

	//=========================================================================
	// Query API (thread-safe, callable from game thread)
	//=========================================================================

	/**
	 * Get collision feedback for a specific collider
	 * @param ColliderIndex - Index of the collider
	 * @param OutFeedback - Output array of feedback entries
	 * @param OutCount - Number of feedback entries
	 * @return true if feedback is available
	 */
	bool GetFeedbackForCollider(int32 ColliderIndex, TArray<FGPUCollisionFeedback>& OutFeedback, int32& OutCount);

	/**
	 * Get all collision feedback (unfiltered)
	 * @param OutFeedback - Output array of all feedback entries
	 * @param OutCount - Number of feedback entries
	 * @return true if feedback is available
	 */
	bool GetAllFeedback(TArray<FGPUCollisionFeedback>& OutFeedback, int32& OutCount);

	/** Get current feedback count */
	int32 GetFeedbackCount() const { return ReadyFeedbackCount; }

	/**
	 * Get contact count for a specific collider index
	 * @param ColliderIndex - Index of the collider
	 * @return Number of particles colliding with this collider
	 */
	int32 GetContactCount(int32 ColliderIndex) const;

	/**
	 * Get all collider contact counts
	 * @param OutCounts - Output array of contact counts per collider
	 */
	void GetAllContactCounts(TArray<int32>& OutCounts) const;

private:
	//=========================================================================
	// State
	//=========================================================================

	bool bIsInitialized = false;
	bool bFeedbackEnabled = false;

	//=========================================================================
	// GPU Buffers (managed via RDG extraction)
	//=========================================================================

	TRefCountPtr<FRDGPooledBuffer> CollisionFeedbackBuffer;
	TRefCountPtr<FRDGPooledBuffer> CollisionCounterBuffer;
	TRefCountPtr<FRDGPooledBuffer> ColliderContactCountBuffer;

	//=========================================================================
	// Async Readback Objects (triple buffered)
	//=========================================================================

	FRHIGPUBufferReadback* FeedbackReadbacks[NUM_FEEDBACK_BUFFERS] = {nullptr};
	FRHIGPUBufferReadback* CounterReadbacks[NUM_FEEDBACK_BUFFERS] = {nullptr};
	FRHIGPUBufferReadback* ContactCountReadbacks[NUM_FEEDBACK_BUFFERS] = {nullptr};

	//=========================================================================
	// Ready Data (thread-safe access via lock)
	//=========================================================================

	mutable FCriticalSection FeedbackLock;

	TArray<FGPUCollisionFeedback> ReadyFeedback;
	int32 ReadyFeedbackCount = 0;

	TArray<int32> ReadyContactCounts;

	//=========================================================================
	// Triple Buffer State
	//=========================================================================

	int32 CurrentWriteIndex = 0;
	int32 FeedbackFrameNumber = 0;
	int32 ContactCountFrameNumber = 0;
	std::atomic<int32> CompletedFeedbackFrame{-1};
};
