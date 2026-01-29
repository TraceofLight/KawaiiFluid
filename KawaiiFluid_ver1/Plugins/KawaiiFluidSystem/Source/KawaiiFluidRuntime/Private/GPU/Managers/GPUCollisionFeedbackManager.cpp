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

	// Initialize ready arrays for bone colliders
	ReadyFeedback.SetNum(MAX_COLLISION_FEEDBACK);
	ReadyFeedbackCount = 0;
	ReadyContactCounts.SetNumZeroed(MAX_COLLIDER_COUNT);

	// Initialize ready arrays for StaticMesh colliders (WorldCollision)
	ReadyStaticMeshFeedback.SetNum(MAX_STATICMESH_COLLISION_FEEDBACK);
	ReadyStaticMeshFeedbackCount = 0;

	// Initialize ready arrays for FluidInteraction StaticMesh colliders
	ReadyFluidInteractionSMFeedback.SetNum(MAX_FLUIDINTERACTION_SM_FEEDBACK);
	ReadyFluidInteractionSMFeedbackCount = 0;

	UE_LOG(LogGPUCollisionFeedback, Log, TEXT("GPUCollisionFeedbackManager initialized (BoneFeedback=%d, StaticMeshFeedback=%d, FluidInteractionSMFeedback=%d)"),
		MAX_COLLISION_FEEDBACK, MAX_STATICMESH_COLLISION_FEEDBACK, MAX_FLUIDINTERACTION_SM_FEEDBACK);
}

void FGPUCollisionFeedbackManager::Release()
{
	ReleaseReadbackObjects();

	// Release bone collider buffers
	CollisionFeedbackBuffer.SafeRelease();
	CollisionCounterBuffer.SafeRelease();
	ColliderContactCountBuffer.SafeRelease();

	// Release StaticMesh collider buffers (WorldCollision)
	StaticMeshFeedbackBuffer.SafeRelease();
	StaticMeshCounterBuffer.SafeRelease();

	// Release FluidInteraction StaticMesh collider buffers
	FluidInteractionSMFeedbackBuffer.SafeRelease();
	FluidInteractionSMCounterBuffer.SafeRelease();

	// Clear bone collider ready arrays
	ReadyFeedback.Empty();
	ReadyFeedbackCount = 0;
	ReadyContactCounts.Empty();

	// Clear StaticMesh collider ready arrays (WorldCollision)
	ReadyStaticMeshFeedback.Empty();
	ReadyStaticMeshFeedbackCount = 0;

	// Clear FluidInteraction StaticMesh collider ready arrays
	ReadyFluidInteractionSMFeedback.Empty();
	ReadyFluidInteractionSMFeedbackCount = 0;

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
		// Bone collider readbacks
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

		// StaticMesh collider readbacks (WorldCollision)
		if (StaticMeshFeedbackReadbacks[i] == nullptr)
		{
			StaticMeshFeedbackReadbacks[i] = new FRHIGPUBufferReadback(*FString::Printf(TEXT("StaticMeshFeedbackReadback_%d"), i));
		}

		if (StaticMeshCounterReadbacks[i] == nullptr)
		{
			StaticMeshCounterReadbacks[i] = new FRHIGPUBufferReadback(*FString::Printf(TEXT("StaticMeshCounterReadback_%d"), i));
		}

		// FluidInteraction StaticMesh readbacks
		if (FluidInteractionSMFeedbackReadbacks[i] == nullptr)
		{
			FluidInteractionSMFeedbackReadbacks[i] = new FRHIGPUBufferReadback(*FString::Printf(TEXT("FluidInteractionSMFeedbackReadback_%d"), i));
		}

		if (FluidInteractionSMCounterReadbacks[i] == nullptr)
		{
			FluidInteractionSMCounterReadbacks[i] = new FRHIGPUBufferReadback(*FString::Printf(TEXT("FluidInteractionSMCounterReadback_%d"), i));
		}
	}

	UE_LOG(LogGPUCollisionFeedback, Log, TEXT("Readback objects allocated (BoneFeedback=%d, StaticMeshFeedback=%d, FluidInteractionSMFeedback=%d, NumBuffers=%d, MaxColliders=%d)"),
		MAX_COLLISION_FEEDBACK, MAX_STATICMESH_COLLISION_FEEDBACK, MAX_FLUIDINTERACTION_SM_FEEDBACK, NUM_FEEDBACK_BUFFERS, MAX_COLLIDER_COUNT);
}

void FGPUCollisionFeedbackManager::ReleaseReadbackObjects()
{
	for (int32 i = 0; i < NUM_FEEDBACK_BUFFERS; ++i)
	{
		// Release bone collider readbacks
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

		// Release StaticMesh collider readbacks (WorldCollision)
		if (StaticMeshFeedbackReadbacks[i] != nullptr)
		{
			delete StaticMeshFeedbackReadbacks[i];
			StaticMeshFeedbackReadbacks[i] = nullptr;
		}
		if (StaticMeshCounterReadbacks[i] != nullptr)
		{
			delete StaticMeshCounterReadbacks[i];
			StaticMeshCounterReadbacks[i] = nullptr;
		}

		// Release FluidInteraction StaticMesh readbacks
		if (FluidInteractionSMFeedbackReadbacks[i] != nullptr)
		{
			delete FluidInteractionSMFeedbackReadbacks[i];
			FluidInteractionSMFeedbackReadbacks[i] = nullptr;
		}
		if (FluidInteractionSMCounterReadbacks[i] != nullptr)
		{
			delete FluidInteractionSMCounterReadbacks[i];
			FluidInteractionSMCounterReadbacks[i] = nullptr;
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

	// Need at least 2 frames for triple buffering to work
	if (FeedbackFrameNumber < 2)
	{
		return;
	}

	// =====================================================
	// Process Bone Collider Feedback (BoneIndex >= 0)
	// Search for ready bone readback independently
	// =====================================================
	{
		int32 BoneReadIdx = -1;
		for (int32 i = 0; i < NUM_FEEDBACK_BUFFERS; ++i)
		{
			if (CounterReadbacks[i] && CounterReadbacks[i]->IsReady())
			{
				BoneReadIdx = i;
				break;
			}
		}

		if (BoneReadIdx >= 0)
		{
			// Read counter first
			uint32 FeedbackCount = 0;
			{
				const uint32* CounterData = (const uint32*)CounterReadbacks[BoneReadIdx]->Lock(sizeof(uint32));
				if (CounterData)
				{
					FeedbackCount = *CounterData;
				}
				CounterReadbacks[BoneReadIdx]->Unlock();
			}

			// Clamp to max
			FeedbackCount = FMath::Min(FeedbackCount, (uint32)MAX_COLLISION_FEEDBACK);

			// Read feedback data if any and if ready
			if (FeedbackCount > 0 && FeedbackReadbacks[BoneReadIdx]->IsReady())
			{
				FScopeLock Lock(&FeedbackLock);

				const uint32 CopySize = FeedbackCount * sizeof(FGPUCollisionFeedback);
				const FGPUCollisionFeedback* FeedbackData = (const FGPUCollisionFeedback*)FeedbackReadbacks[BoneReadIdx]->Lock(CopySize);

				if (FeedbackData)
				{
					ReadyFeedback.SetNum(FeedbackCount);
					FMemory::Memcpy(ReadyFeedback.GetData(), FeedbackData, CopySize);
					ReadyFeedbackCount = FeedbackCount;

					// Debug: BoneIndex 샘플 확인 (매 60프레임)
					static int32 BoneDebugCounter = 0;
					if (++BoneDebugCounter % 60 == 0 && FeedbackCount > 0)
					{
						FString BoneIdxSamples;
						int32 SampleCount = FMath::Min((uint32)5, FeedbackCount);
						for (int32 s = 0; s < SampleCount; ++s)
						{
							BoneIdxSamples += FString::Printf(TEXT("[%d:OwnerID=%d] "), FeedbackData[s].BoneIndex, FeedbackData[s].ColliderOwnerID);
						}
						UE_LOG(LogTemp, Warning, TEXT("[BoneBuffer] Count=%d, Samples=%s"), FeedbackCount, *BoneIdxSamples);
					}
				}

				FeedbackReadbacks[BoneReadIdx]->Unlock();

				UE_LOG(LogGPUCollisionFeedback, Verbose, TEXT("Read %d bone feedback entries from readback %d"), FeedbackCount, BoneReadIdx);
			}
			else if (FeedbackCount == 0)
			{
				FScopeLock Lock(&FeedbackLock);
				ReadyFeedbackCount = 0;
			}
		}
	}

	// =====================================================
	// Process StaticMesh Collider Feedback (BoneIndex < 0)
	// Search for ready StaticMesh readback INDEPENDENTLY
	// =====================================================
	{
		int32 SMReadIdx = -1;
		for (int32 i = 0; i < NUM_FEEDBACK_BUFFERS; ++i)
		{
			if (StaticMeshCounterReadbacks[i] && StaticMeshCounterReadbacks[i]->IsReady())
			{
				SMReadIdx = i;
				break;
			}
		}

		if (SMReadIdx >= 0)
		{
			// Read counter first
			uint32 StaticMeshFeedbackCount = 0;
			{
				const uint32* CounterData = (const uint32*)StaticMeshCounterReadbacks[SMReadIdx]->Lock(sizeof(uint32));
				if (CounterData)
				{
					StaticMeshFeedbackCount = *CounterData;
				}
				StaticMeshCounterReadbacks[SMReadIdx]->Unlock();
			}

			// Clamp to max
			StaticMeshFeedbackCount = FMath::Min(StaticMeshFeedbackCount, (uint32)MAX_STATICMESH_COLLISION_FEEDBACK);

			// Read feedback data if any and if ready
			if (StaticMeshFeedbackCount > 0 && StaticMeshFeedbackReadbacks[SMReadIdx] && StaticMeshFeedbackReadbacks[SMReadIdx]->IsReady())
			{
				FScopeLock Lock(&FeedbackLock);

				const uint32 CopySize = StaticMeshFeedbackCount * sizeof(FGPUCollisionFeedback);
				const FGPUCollisionFeedback* FeedbackData = (const FGPUCollisionFeedback*)StaticMeshFeedbackReadbacks[SMReadIdx]->Lock(CopySize);

				if (FeedbackData)
				{
					ReadyStaticMeshFeedback.SetNum(StaticMeshFeedbackCount);
					FMemory::Memcpy(ReadyStaticMeshFeedback.GetData(), FeedbackData, CopySize);
					ReadyStaticMeshFeedbackCount = StaticMeshFeedbackCount;

					// Debug: StaticMesh BoneIndex 샘플 확인 (매 60프레임)
					static int32 SMDebugCounter = 0;
					if (++SMDebugCounter % 60 == 0 && StaticMeshFeedbackCount > 0)
					{
						FString BoneIdxSamples;
						int32 SampleCount = FMath::Min((uint32)5, StaticMeshFeedbackCount);
						for (int32 s = 0; s < SampleCount; ++s)
						{
							BoneIdxSamples += FString::Printf(TEXT("[%d:OwnerID=%d] "), FeedbackData[s].BoneIndex, FeedbackData[s].ColliderOwnerID);
						}
						UE_LOG(LogTemp, Warning, TEXT("[SMBuffer] Count=%d, Samples=%s"), StaticMeshFeedbackCount, *BoneIdxSamples);
					}
				}

				StaticMeshFeedbackReadbacks[SMReadIdx]->Unlock();

				UE_LOG(LogGPUCollisionFeedback, Verbose, TEXT("Read %d StaticMesh feedback entries from readback %d"), StaticMeshFeedbackCount, SMReadIdx);
			}
			else if (StaticMeshFeedbackCount == 0)
			{
				FScopeLock Lock(&FeedbackLock);
				ReadyStaticMeshFeedbackCount = 0;
			}
		}
	}

	// =====================================================
	// Process FluidInteraction StaticMesh Feedback
	// (BoneIndex < 0, bHasFluidInteraction = 1)
	// Search for ready FluidInteractionSM readback INDEPENDENTLY
	// =====================================================
	{
		int32 FISMReadIdx = -1;
		for (int32 i = 0; i < NUM_FEEDBACK_BUFFERS; ++i)
		{
			if (FluidInteractionSMCounterReadbacks[i] && FluidInteractionSMCounterReadbacks[i]->IsReady())
			{
				FISMReadIdx = i;
				break;
			}
		}

		if (FISMReadIdx >= 0)
		{
			// Read counter first
			uint32 FluidInteractionSMFeedbackCount = 0;
			{
				const uint32* CounterData = (const uint32*)FluidInteractionSMCounterReadbacks[FISMReadIdx]->Lock(sizeof(uint32));
				if (CounterData)
				{
					FluidInteractionSMFeedbackCount = *CounterData;
				}
				FluidInteractionSMCounterReadbacks[FISMReadIdx]->Unlock();
			}

			// Clamp to max
			FluidInteractionSMFeedbackCount = FMath::Min(FluidInteractionSMFeedbackCount, (uint32)MAX_FLUIDINTERACTION_SM_FEEDBACK);

			// Read feedback data if any and if ready
			if (FluidInteractionSMFeedbackCount > 0 && FluidInteractionSMFeedbackReadbacks[FISMReadIdx] && FluidInteractionSMFeedbackReadbacks[FISMReadIdx]->IsReady())
			{
				FScopeLock Lock(&FeedbackLock);

				const uint32 CopySize = FluidInteractionSMFeedbackCount * sizeof(FGPUCollisionFeedback);
				const FGPUCollisionFeedback* FeedbackData = (const FGPUCollisionFeedback*)FluidInteractionSMFeedbackReadbacks[FISMReadIdx]->Lock(CopySize);

				if (FeedbackData)
				{
					ReadyFluidInteractionSMFeedback.SetNum(FluidInteractionSMFeedbackCount);
					FMemory::Memcpy(ReadyFluidInteractionSMFeedback.GetData(), FeedbackData, CopySize);
					ReadyFluidInteractionSMFeedbackCount = FluidInteractionSMFeedbackCount;

					// Debug: FluidInteractionSM BoneIndex sample check (every 60 frames)
					static int32 FISMDebugCounter = 0;
					if (++FISMDebugCounter % 60 == 0 && FluidInteractionSMFeedbackCount > 0)
					{
						FString BoneIdxSamples;
						int32 SampleCount = FMath::Min((uint32)5, FluidInteractionSMFeedbackCount);
						for (int32 s = 0; s < SampleCount; ++s)
						{
							BoneIdxSamples += FString::Printf(TEXT("[%d:OwnerID=%d] "), FeedbackData[s].BoneIndex, FeedbackData[s].ColliderOwnerID);
						}
						UE_LOG(LogTemp, Warning, TEXT("[FluidInteractionSMBuffer] Count=%d, Samples=%s"), FluidInteractionSMFeedbackCount, *BoneIdxSamples);
					}
				}

				FluidInteractionSMFeedbackReadbacks[FISMReadIdx]->Unlock();

				UE_LOG(LogGPUCollisionFeedback, Verbose, TEXT("Read %d FluidInteractionSM feedback entries from readback %d"), FluidInteractionSMFeedbackCount, FISMReadIdx);
			}
			else if (FluidInteractionSMFeedbackCount == 0)
			{
				FScopeLock Lock(&FeedbackLock);
				ReadyFluidInteractionSMFeedbackCount = 0;
			}
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

	// =====================================================
	// StaticMesh Collision Feedback Readback (BoneIndex < 0)
	// =====================================================
	if (bFeedbackEnabled && StaticMeshFeedbackBuffer.IsValid() && StaticMeshCounterBuffer.IsValid())
	{
		// Transition buffers for copy
		RHICmdList.Transition(FRHITransitionInfo(
			StaticMeshFeedbackBuffer->GetRHI(),
			ERHIAccess::UAVCompute,
			ERHIAccess::CopySrc));

		RHICmdList.Transition(FRHITransitionInfo(
			StaticMeshCounterBuffer->GetRHI(),
			ERHIAccess::UAVCompute,
			ERHIAccess::CopySrc));

		// EnqueueCopy - async copy to readback buffer (non-blocking!)
		StaticMeshFeedbackReadbacks[WriteIdx]->EnqueueCopy(
			RHICmdList,
			StaticMeshFeedbackBuffer->GetRHI(),
			MAX_STATICMESH_COLLISION_FEEDBACK * sizeof(FGPUCollisionFeedback)
		);

		StaticMeshCounterReadbacks[WriteIdx]->EnqueueCopy(
			RHICmdList,
			StaticMeshCounterBuffer->GetRHI(),
			sizeof(uint32)
		);

		// Transition back for next frame
		RHICmdList.Transition(FRHITransitionInfo(
			StaticMeshFeedbackBuffer->GetRHI(),
			ERHIAccess::CopySrc,
			ERHIAccess::UAVCompute));

		RHICmdList.Transition(FRHITransitionInfo(
			StaticMeshCounterBuffer->GetRHI(),
			ERHIAccess::CopySrc,
			ERHIAccess::UAVCompute));

		if (bLogThisFrame)
		{
			UE_LOG(LogGPUCollisionFeedback, Log, TEXT("EnqueueCopy StaticMesh feedback to readback %d"), WriteIdx);
		}
	}

	// =====================================================
	// FluidInteraction StaticMesh Feedback Readback
	// (BoneIndex < 0, bHasFluidInteraction = 1)
	// =====================================================
	if (bFeedbackEnabled && FluidInteractionSMFeedbackBuffer.IsValid() && FluidInteractionSMCounterBuffer.IsValid())
	{
		// Transition buffers for copy
		RHICmdList.Transition(FRHITransitionInfo(
			FluidInteractionSMFeedbackBuffer->GetRHI(),
			ERHIAccess::UAVCompute,
			ERHIAccess::CopySrc));

		RHICmdList.Transition(FRHITransitionInfo(
			FluidInteractionSMCounterBuffer->GetRHI(),
			ERHIAccess::UAVCompute,
			ERHIAccess::CopySrc));

		// EnqueueCopy - async copy to readback buffer (non-blocking!)
		FluidInteractionSMFeedbackReadbacks[WriteIdx]->EnqueueCopy(
			RHICmdList,
			FluidInteractionSMFeedbackBuffer->GetRHI(),
			MAX_FLUIDINTERACTION_SM_FEEDBACK * sizeof(FGPUCollisionFeedback)
		);

		FluidInteractionSMCounterReadbacks[WriteIdx]->EnqueueCopy(
			RHICmdList,
			FluidInteractionSMCounterBuffer->GetRHI(),
			sizeof(uint32)
		);

		// Transition back for next frame
		RHICmdList.Transition(FRHITransitionInfo(
			FluidInteractionSMFeedbackBuffer->GetRHI(),
			ERHIAccess::CopySrc,
			ERHIAccess::UAVCompute));

		RHICmdList.Transition(FRHITransitionInfo(
			FluidInteractionSMCounterBuffer->GetRHI(),
			ERHIAccess::CopySrc,
			ERHIAccess::UAVCompute));

		if (bLogThisFrame)
		{
			UE_LOG(LogGPUCollisionFeedback, Log, TEXT("EnqueueCopy FluidInteractionSM feedback to readback %d"), WriteIdx);
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

bool FGPUCollisionFeedbackManager::GetAllStaticMeshFeedback(TArray<FGPUCollisionFeedback>& OutFeedback, int32& OutCount)
{
	FScopeLock Lock(&FeedbackLock);

	OutCount = ReadyStaticMeshFeedbackCount;

	if (!bFeedbackEnabled || ReadyStaticMeshFeedbackCount == 0)
	{
		OutFeedback.Reset();
		return false;
	}

	OutFeedback.SetNum(ReadyStaticMeshFeedbackCount);
	FMemory::Memcpy(OutFeedback.GetData(), ReadyStaticMeshFeedback.GetData(), ReadyStaticMeshFeedbackCount * sizeof(FGPUCollisionFeedback));

	return true;
}

bool FGPUCollisionFeedbackManager::GetAllFluidInteractionSMFeedback(TArray<FGPUCollisionFeedback>& OutFeedback, int32& OutCount)
{
	FScopeLock Lock(&FeedbackLock);

	OutCount = ReadyFluidInteractionSMFeedbackCount;

	if (!bFeedbackEnabled || ReadyFluidInteractionSMFeedbackCount == 0)
	{
		OutFeedback.Reset();
		return false;
	}

	OutFeedback.SetNum(ReadyFluidInteractionSMFeedbackCount);
	FMemory::Memcpy(OutFeedback.GetData(), ReadyFluidInteractionSMFeedback.GetData(), ReadyFluidInteractionSMFeedbackCount * sizeof(FGPUCollisionFeedback));

	return true;
}
