// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * GPU Bone Delta Attachment Structure (64 bytes)
 *
 * Per-particle attachment data for following WorldBoundaryParticles.
 * Uses BoundaryParticleIndex (original index before Z-Order sorting) for stable attachment.
 *
 * This structure mirrors the HLSL struct in FluidBoneDeltaAttachment.ush
 *
 * Used by:
 * - FluidApplyBoneTransform.usf: Read WorldBoundaryParticles[BoundaryParticleIndex].Position + LocalOffset
 * - FluidUpdateBoneDeltaAttachment.usf: Find nearest boundary and store OriginalIndex, update LocalOffset
 * - FluidAnisotropyCompute.usf: Use LocalNormal for surface-aligned anisotropy
 *
 * Detach condition: distance from PreviousPosition > DetachDistance OR LocalOffset too large
 */
struct FGPUBoneDeltaAttachment
{
	int32 BoundaryParticleIndex;  // 4 bytes  - Index into WorldBoundaryParticles buffer (-1 = not attached)
	float Padding1;               // 4 bytes  - Alignment padding (total: 8)

	FVector3f LocalNormal;        // 12 bytes - Surface normal in world space (for anisotropy)
	float Padding2;               // 4 bytes  - Alignment padding (total: 24)

	FVector3f PreviousPosition;   // 12 bytes - Previous frame position (for detach check)
	float Padding3;               // 4 bytes  - Alignment padding (total: 40)

	FVector3f LocalOffset;        // 12 bytes - Offset from boundary position (physics drift)
	float Padding4;               // 4 bytes  - Alignment padding (total: 56)

	// Add 8 bytes padding to reach 64 bytes (16-byte aligned)
	float Padding5;               // 4 bytes
	float Padding6;               // 4 bytes  (total: 64)

	FGPUBoneDeltaAttachment()
		: BoundaryParticleIndex(-1)
		, Padding1(0.0f)
		, LocalNormal(FVector3f::ZeroVector)
		, Padding2(0.0f)
		, PreviousPosition(FVector3f::ZeroVector)
		, Padding3(0.0f)
		, LocalOffset(FVector3f::ZeroVector)
		, Padding4(0.0f)
		, Padding5(0.0f)
		, Padding6(0.0f)
	{
	}

	/** Check if attached to a boundary particle */
	FORCEINLINE bool IsAttached() const
	{
		return BoundaryParticleIndex >= 0;
	}

	/** Clear attachment */
	FORCEINLINE void Clear()
	{
		BoundaryParticleIndex = -1;
		LocalNormal = FVector3f::ZeroVector;
		LocalOffset = FVector3f::ZeroVector;
	}
};

// Compile-time size validation
static_assert(sizeof(FGPUBoneDeltaAttachment) == 64, "FGPUBoneDeltaAttachment must be 64 bytes");
static_assert(alignof(FGPUBoneDeltaAttachment) <= 16, "FGPUBoneDeltaAttachment alignment must not exceed 16 bytes");

/**
 * Boundary Attachment Constants
 *
 * NOTE: DetachDistance is now calculated dynamically as SmoothingRadius * 3.0f
 * in GPUFluidSimulator_SimPasses.cpp (UpdateBoneDeltaAttachmentPass)
 */
namespace EBoundaryAttachment
{
	// DetachDistance is no longer used - now dynamic (SmoothingRadius * 3.0f)
	// constexpr float DetachDistance = 100000.0f;  // DEPRECATED
	constexpr int32 InvalidBoneIndex = -1;
}
