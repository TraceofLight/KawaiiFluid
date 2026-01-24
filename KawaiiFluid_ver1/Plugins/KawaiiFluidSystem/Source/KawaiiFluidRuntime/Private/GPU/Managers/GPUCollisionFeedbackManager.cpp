// Copyright 2026 Team_Bruteforce. All Rights Reserved.
// FGPUCollisionFeedbackManager - Collision feedback system with async GPU readback

#include "GPU/Managers/GPUCollisionFeedbackManager.h"
#include "RHIGPUReadback.h"
#include "RenderingThread.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGPUCollisionFeedback, Log, All);
DEFINE_LOG_CATEGORY(LogGPUCollisionFeedback);

//=============================================================================
// Constructor / Destructor
//=============================================================================

FGPUCollisionFeedbackManager::FGPUCollisionFeedbackManager()
	: bIsInitialized(false)
	, bFeedbackEnabled(false)
{
}

FGPUCollisionFeedbackManager::~FGPUCollisionFeedbackManager()
{
	Release();
}

//=============================================================================
// Lifecycle
//=============================================================================

void FGPUCollisionFeedbackManager::Initialize()
{
	bIsInitialized = true;

	// Initialize ready arrays
	ReadyFeedback.SetNum(MAX_COLLISION_FEEDBACK);
	ReadyFeedbackCount = 0;
	ReadyContactCounts.SetNumZeroed(MAX_COLLIDER_COUNT);

	UE_LOG(LogGPUCollisionFeedback, Log, TEXT("GPUCollisionFeedbackManager initialized"));
}

void FGPUCollisionFeedbackManager::Release()
{
	ReleaseReadbackObjects();

	CollisionFeedbackBuffer.SafeRelease();
	CollisionCounterBuffer.SafeRelease();
	ColliderContactCountBuffer.SafeRelease();

	ReadyFeedback.Empty();
	ReadyFeedbackCount = 0;
	ReadyContactCounts.Empty();

	CurrentWriteIndex = 0;
	FeedbackFrameNumber = 0;
	ContactCountFrameNumber = 0;
	CompletedFeedbackFrame.store(-1);

	bIsInitialized = false;

	UE_LOG(LogGPUCollisionFeedback, Log, TEXT("GPUCollisionFeedbackManager released"));
}

//=============================================================================
// Buffer Management
//=============================================================================

void FGPUCollisionFeedbackManager::AllocateReadbackObjects(FRHICommandListImmediate& RHICmdList)
{
	for (int32 i = 0; i < NUM_FEEDBACK_BUFFERS; ++i)
	{
		if (FeedbackReadbacks[i] == nullptr)
		{
			FeedbackReadbacks[i] = new FRHIGPUBufferReadback(*FString::Printf(TEXT("CollisionFeedbackReadback_%d"), i));
		}

		if (CounterReadbacks[i] == nullptr)
		{
			CounterReadbacks[i] = new FRHIGPUBufferReadback(*FString::Printf(TEXT("CollisionCounterReadback_%d"), i));
		}

		if (ContactCountReadbacks[i] == nullptr)
		{
			ContactCountReadbacks[i] = new FRHIGPUBufferReadback(*FString::Printf(TEXT("ContactCountReadback_%d"), i));
		}
	}

	UE_LOG(LogGPUCollisionFeedback, Log, TEXT("Readback objects allocated (MaxFeedback=%d, NumBuffers=%d, MaxColliders=%d)"),
		MAX_COLLISION_FEEDBACK, NUM_FEEDBACK_BUFFERS, MAX_COLLIDER_COUNT);
}

void FGPUCollisionFeedbackManager::ReleaseReadbackObjects()
{
	for (int32 i = 0; i < NUM_FEEDBACK_BUFFERS; ++i)
	{
		if (FeedbackReadbacks[i] != nullptr)
		{
			delete FeedbackReadbacks[i];
			FeedbackReadbacks[i] = nullptr;
		}
		if (CounterReadbacks[i] != nullptr)
		{
			delete CounterReadbacks[i];
			CounterReadbacks[i] = nullptr;
		}
		if (ContactCountReadbacks[i] != nullptr)
		{
			delete ContactCountReadbacks[i];
			ContactCountReadbacks[i] = nullptr;
		}
	}
}

//=============================================================================
// Readback Processing
//=============================================================================

void FGPUCollisionFeedbackManager::ProcessFeedbackReadback(FRHICommandListImmediate& RHICmdList)
{
	if (!bFeedbackEnabled)
	{
		return;
	}

	// Ensure readback objects are allocated
	if (FeedbackReadbacks[0] == nullptr)
	{
		return;
	}

	// Search for any ready buffer
	int32 ReadIdx = -1;
	for (int32 i = 0; i < NUM_FEEDBACK_BUFFERS; ++i)
	{
		if (CounterReadbacks[i] && CounterReadbacks[i]->IsReady())
		{
			ReadIdx = i;
			break;
		}
	}

	// Only read if we have completed at least 2 frames and found a ready buffer
	if (FeedbackFrameNumber >= 2 && ReadIdx >= 0)
	{
		// Read counter first
		uint32 FeedbackCount = 0;
		{
			const uint32* CounterData = (const uint32*)CounterReadbacks[ReadIdx]->Lock(sizeof(uint32));
			if (CounterData)
			{
				FeedbackCount = *CounterData;
			}
			CounterReadbacks[ReadIdx]->Unlock();
		}

		// Clamp to max
		FeedbackCount = FMath::Min(FeedbackCount, (uint32)MAX_COLLISION_FEEDBACK);

		// Read feedback data if any and if ready
		if (FeedbackCount > 0 && FeedbackReadbacks[ReadIdx]->IsReady())
		{
			FScopeLock Lock(&FeedbackLock);

			const uint32 CopySize = FeedbackCount * sizeof(FGPUCollisionFeedback);
			const FGPUCollisionFeedback* FeedbackData = (const FGPUCollisionFeedback*)FeedbackReadbacks[ReadIdx]->Lock(CopySize);

			if (FeedbackData)
			{
				ReadyFeedback.SetNum(FeedbackCount);
				FMemory::Memcpy(ReadyFeedback.GetData(), FeedbackData, CopySize);
				ReadyFeedbackCount = FeedbackCount;
			}

			FeedbackReadbacks[ReadIdx]->Unlock();

			UE_LOG(LogGPUCollisionFeedback, Verbose, TEXT("Read %d feedback entries from readback %d"), FeedbackCount, ReadIdx);
		}
		else if (FeedbackCount == 0)
		{
			FScopeLock Lock(&FeedbackLock);
			ReadyFeedbackCount = 0;
		}
	}
}

void FGPUCollisionFeedbackManager::ProcessContactCountReadback(FRHICommandListImmediate& RHICmdList)
{
	// Debug logging (every 60 frames)
	static int32 DebugFrame = 0;
	const bool bLogThisFrame = (DebugFrame++ % 60 == 0);

	// Ensure readback objects are valid
	if (ContactCountReadbacks[0] == nullptr)
	{
		return;
	}

	// Search for any ready buffer
	int32 ReadIdx = -1;
	for (int32 i = 0; i < NUM_FEEDBACK_BUFFERS; ++i)
	{
		if (ContactCountReadbacks[i] && ContactCountReadbacks[i]->IsReady())
		{
			ReadIdx = i;
			break;
		}
	}

	// Only read if we have completed at least 2 frames and found a ready buffer
	if (ContactCountFrameNumber >= 2 && ReadIdx >= 0)
	{
		const uint32* CountData = (const uint32*)ContactCountReadbacks[ReadIdx]->Lock(MAX_COLLIDER_COUNT * sizeof(uint32));

		if (CountData)
		{
			FScopeLock Lock(&FeedbackLock);

			ReadyContactCounts.SetNumUninitialized(MAX_COLLIDER_COUNT);
			int32 TotalContactCount = 0;
			int32 NonZeroColliders = 0;

			for (int32 i = 0; i < MAX_COLLIDER_COUNT; ++i)
			{
				ReadyContactCounts[i] = static_cast<int32>(CountData[i]);
				if (CountData[i] > 0)
				{
					TotalContactCount += CountData[i];
					NonZeroColliders++;
				}
			}

			if (bLogThisFrame && NonZeroColliders > 0)
			{
				UE_LOG(LogGPUCollisionFeedback, Log, TEXT("Contact count: Total=%d, NonZeroColliders=%d"),
					TotalContactCount, NonZeroColliders);
			}
		}

		ContactCountReadbacks[ReadIdx]->Unlock();
	}
}

void FGPUCollisionFeedbackManager::EnqueueReadbackCopy(FRHICommandListImmediate& RHICmdList)
{
	// This is called after the simulation pass to enqueue copies for next frame's readback
	if (!bIsInitialized)
	{
		return;
	}

	// Debug logging (every 60 frames)
	static int32 EnqueueDebugFrame = 0;
	const bool bLogThisFrame = (EnqueueDebugFrame++ % 60 == 0);

	// Ensure readback objects are allocated
	if (FeedbackReadbacks[0] == nullptr)
	{
		AllocateReadbackObjects(RHICmdList);
	}

	const int32 WriteIdx = CurrentWriteIndex;

	// =====================================================
	// Collision Feedback Readback
	// =====================================================
	if (bFeedbackEnabled && CollisionFeedbackBuffer.IsValid() && CollisionCounterBuffer.IsValid())
	{
		// Transition buffers for copy
		RHICmdList.Transition(FRHITransitionInfo(
			CollisionFeedbackBuffer->GetRHI(),
			ERHIAccess::UAVCompute,
			ERHIAccess::CopySrc));

		RHICmdList.Transition(FRHITransitionInfo(
			CollisionCounterBuffer->GetRHI(),
			ERHIAccess::UAVCompute,
			ERHIAccess::CopySrc));

		// EnqueueCopy - async copy to readback buffer (non-blocking!)
		FeedbackReadbacks[WriteIdx]->EnqueueCopy(
			RHICmdList,
			CollisionFeedbackBuffer->GetRHI(),
			MAX_COLLISION_FEEDBACK * sizeof(FGPUCollisionFeedback)
		);

		CounterReadbacks[WriteIdx]->EnqueueCopy(
			RHICmdList,
			CollisionCounterBuffer->GetRHI(),
			sizeof(uint32)
		);

		// Transition back for next frame
		RHICmdList.Transition(FRHITransitionInfo(
			CollisionFeedbackBuffer->GetRHI(),
			ERHIAccess::CopySrc,
			ERHIAccess::UAVCompute));

		RHICmdList.Transition(FRHITransitionInfo(
			CollisionCounterBuffer->GetRHI(),
			ERHIAccess::CopySrc,
			ERHIAccess::UAVCompute));

		if (bLogThisFrame)
		{
			UE_LOG(LogGPUCollisionFeedback, Log, TEXT("EnqueueCopy feedback to readback %d"), WriteIdx);
		}
	}

	// =====================================================
	// Contact Count Readback
	// =====================================================
	if (ColliderContactCountBuffer.IsValid())
	{
		// Transition for copy
		RHICmdList.Transition(FRHITransitionInfo(
			ColliderContactCountBuffer->GetRHI(),
			ERHIAccess::UAVCompute,
			ERHIAccess::CopySrc));

		// EnqueueCopy - async copy to readback buffer (non-blocking!)
		ContactCountReadbacks[WriteIdx]->EnqueueCopy(
			RHICmdList,
			ColliderContactCountBuffer->GetRHI(),
			MAX_COLLIDER_COUNT * sizeof(uint32)
		);

		// Transition back for next frame
		RHICmdList.Transition(FRHITransitionInfo(
			ColliderContactCountBuffer->GetRHI(),
			ERHIAccess::CopySrc,
			ERHIAccess::UAVCompute));

		if (bLogThisFrame)
		{
			UE_LOG(LogGPUCollisionFeedback, Log, TEXT("EnqueueCopy contact counts to readback %d"), WriteIdx);
		}
	}

	// Increment frame counter AFTER EnqueueCopy
	IncrementFrameCounter();
}

void FGPUCollisionFeedbackManager::IncrementFrameCounter()
{
	CurrentWriteIndex = (CurrentWriteIndex + 1) % NUM_FEEDBACK_BUFFERS;
	FeedbackFrameNumber++;
	ContactCountFrameNumber++;
}

//=============================================================================
// Query API
//=============================================================================

bool FGPUCollisionFeedbackManager::GetFeedbackForCollider(int32 ColliderIndex, TArray<FGPUCollisionFeedback>& OutFeedback, int32& OutCount)
{
	FScopeLock Lock(&FeedbackLock);

	OutFeedback.Reset();
	OutCount = 0;

	if (!bFeedbackEnabled || ReadyFeedbackCount == 0)
	{
		return false;
	}

	// Filter feedback for this collider
	for (int32 i = 0; i < ReadyFeedbackCount; ++i)
	{
		if (ReadyFeedback[i].ColliderIndex == ColliderIndex)
		{
			OutFeedback.Add(ReadyFeedback[i]);
		}
	}

	OutCount = OutFeedback.Num();
	return OutCount > 0;
}

bool FGPUCollisionFeedbackManager::GetAllFeedback(TArray<FGPUCollisionFeedback>& OutFeedback, int32& OutCount)
{
	FScopeLock Lock(&FeedbackLock);

	OutCount = ReadyFeedbackCount;

	if (!bFeedbackEnabled || ReadyFeedbackCount == 0)
	{
		OutFeedback.Reset();
		return false;
	}

	OutFeedback.SetNum(ReadyFeedbackCount);
	FMemory::Memcpy(OutFeedback.GetData(), ReadyFeedback.GetData(), ReadyFeedbackCount * sizeof(FGPUCollisionFeedback));

	return true;
}

int32 FGPUCollisionFeedbackManager::GetContactCount(int32 ColliderIndex) const
{
	FScopeLock Lock(&FeedbackLock);

	if (ColliderIndex < 0 || ColliderIndex >= ReadyContactCounts.Num())
	{
		return 0;
	}
	return ReadyContactCounts[ColliderIndex];
}

void FGPUCollisionFeedbackManager::GetAllContactCounts(TArray<int32>& OutCounts) const
{
	FScopeLock Lock(&FeedbackLock);
	OutCounts = ReadyContactCounts;
}
