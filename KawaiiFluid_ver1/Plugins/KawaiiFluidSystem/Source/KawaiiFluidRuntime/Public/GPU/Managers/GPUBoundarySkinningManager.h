// Copyright KawaiiFluid Team. All Rights Reserved.
// FGPUBoundarySkinningManager - GPU Boundary Skinning and Adhesion System

#pragma once

#include "CoreMinimal.h"
#include "RenderGraphResources.h"
#include "RHIResources.h"
#include "GPU/GPUFluidParticle.h"

class FRDGBuilder;

/**
 * FGPUBoundarySkinningManager
 *
 * Manages GPU-based boundary skinning for Flex-style adhesion.
 * Transforms bone-local boundary particles to world space using GPU compute,
 * enabling efficient interaction between fluid and skinned meshes.
 *
 * Features:
 * - Persistent local boundary particles (uploaded once per mesh)
 * - Per-frame bone transform updates
 * - GPU-based skinning transform
 * - Spatial hash for boundary adhesion
 */
class KAWAIIFLUIDRUNTIME_API FGPUBoundarySkinningManager
{
public:
	FGPUBoundarySkinningManager();
	~FGPUBoundarySkinningManager();

	//=========================================================================
	// Lifecycle
	//=========================================================================

	void Initialize();
	void Release();
	bool IsReady() const { return bIsInitialized; }

	//=========================================================================
	// Boundary Particles Upload (Legacy CPU path)
	//=========================================================================

	/**
	 * Upload boundary particles from CPU (legacy path)
	 * @param BoundaryParticles - World-space boundary particles
	 */
	void UploadBoundaryParticles(const FGPUBoundaryParticles& BoundaryParticles);

	/** Check if boundary particles are valid */
	bool HasBoundaryParticles() const { return bBoundaryParticlesValid; }

	/** Get boundary particle count */
	int32 GetBoundaryParticleCount() const { return CachedBoundaryParticles.Num(); }

	/** Get cached boundary particles (for RDG pass) */
	const TArray<FGPUBoundaryParticle>& GetCachedBoundaryParticles() const { return CachedBoundaryParticles; }

	//=========================================================================
	// GPU Boundary Skinning (Persistent Local + GPU Transform)
	//=========================================================================

	/**
	 * Upload local boundary particles for GPU skinning
	 * @param OwnerID - Unique ID for the mesh owner
	 * @param LocalParticles - Bone-local boundary particles
	 */
	void UploadLocalBoundaryParticles(int32 OwnerID, const TArray<FGPUBoundaryParticleLocal>& LocalParticles);

	/**
	 * Upload bone transforms for boundary skinning
	 * @param OwnerID - Unique ID for the mesh owner
	 * @param BoneTransforms - Current bone transforms
	 * @param ComponentTransform - Component world transform
	 */
	void UploadBoneTransformsForBoundary(int32 OwnerID, const TArray<FMatrix44f>& BoneTransforms, const FMatrix44f& ComponentTransform);

	/** Remove skinning data for an owner */
	void RemoveBoundarySkinningData(int32 OwnerID);

	/** Clear all boundary skinning data */
	void ClearAllBoundarySkinningData();

	/** Check if GPU boundary skinning is enabled */
	bool IsGPUBoundarySkinningEnabled() const { return TotalLocalBoundaryParticleCount > 0; }

	/** Get total local boundary particle count */
	int32 GetTotalLocalBoundaryParticleCount() const { return TotalLocalBoundaryParticleCount; }

	//=========================================================================
	// Boundary Adhesion Parameters
	//=========================================================================

	void SetBoundaryAdhesionParams(const FGPUBoundaryAdhesionParams& Params) { CachedBoundaryAdhesionParams = Params; }
	const FGPUBoundaryAdhesionParams& GetBoundaryAdhesionParams() const { return CachedBoundaryAdhesionParams; }
	bool IsBoundaryAdhesionEnabled() const;

	//=========================================================================
	// RDG Pass (called from simulator)
	//=========================================================================

	/**
	 * Add boundary skinning pass to transform local particles to world space
	 * @param GraphBuilder - RDG builder
	 */
	void AddBoundarySkinningPass(FRDGBuilder& GraphBuilder);

	/**
	 * Add boundary adhesion pass
	 * @param GraphBuilder - RDG builder
	 * @param ParticlesUAV - Fluid particles buffer
	 * @param CurrentParticleCount - Number of fluid particles
	 * @param Params - Simulation parameters
	 */
	void AddBoundaryAdhesionPass(
		FRDGBuilder& GraphBuilder,
		FRDGBufferUAVRef ParticlesUAV,
		int32 CurrentParticleCount,
		const FGPUFluidSimulationParams& Params);

	/** Get world boundary buffer for other passes */
	TRefCountPtr<FRDGPooledBuffer>& GetWorldBoundaryBuffer() { return PersistentWorldBoundaryBuffer; }

	/** Check if world boundary buffer is valid */
	bool HasWorldBoundaryBuffer() const { return PersistentWorldBoundaryBuffer.IsValid(); }

private:
	//=========================================================================
	// State
	//=========================================================================

	bool bIsInitialized = false;

	//=========================================================================
	// Boundary Particles (Legacy CPU path)
	//=========================================================================

	TArray<FGPUBoundaryParticle> CachedBoundaryParticles;
	bool bBoundaryParticlesValid = false;
	FGPUBoundaryAdhesionParams CachedBoundaryAdhesionParams;

	//=========================================================================
	// GPU Boundary Skinning Data
	//=========================================================================

	/** Skinning data per owner */
	struct FGPUBoundarySkinningData
	{
		int32 OwnerID = -1;
		TArray<FGPUBoundaryParticleLocal> LocalParticles;
		TArray<FMatrix44f> BoneTransforms;
		FMatrix44f ComponentTransform = FMatrix44f::Identity;
		bool bLocalParticlesUploaded = false;
	};

	TMap<int32, FGPUBoundarySkinningData> BoundarySkinningDataMap;
	int32 TotalLocalBoundaryParticleCount = 0;

	//=========================================================================
	// Persistent Buffers
	//=========================================================================

	TMap<int32, TRefCountPtr<FRDGPooledBuffer>> PersistentLocalBoundaryBuffers;
	TRefCountPtr<FRDGPooledBuffer> PersistentWorldBoundaryBuffer;
	int32 WorldBoundaryBufferCapacity = 0;

	//=========================================================================
	// Dirty Tracking
	//=========================================================================

	bool bBoundarySkinningDataDirty = false;

	//=========================================================================
	// Thread Safety
	//=========================================================================

	mutable FCriticalSection BoundarySkinningLock;
};
