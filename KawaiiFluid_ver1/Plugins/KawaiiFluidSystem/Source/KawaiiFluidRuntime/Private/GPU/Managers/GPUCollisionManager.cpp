// Copyright KawaiiFluid Team. All Rights Reserved.
// FGPUCollisionManager Implementation

#include "GPU/Managers/GPUCollisionManager.h"
#include "GPU/GPUFluidSimulatorShaders.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHIGPUReadback.h"
#include "RenderUtils.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGPUCollisionManager, Log, All);
DEFINE_LOG_CATEGORY(LogGPUCollisionManager);

//=============================================================================
// Constructor / Destructor
//=============================================================================

FGPUCollisionManager::FGPUCollisionManager()
{
}

FGPUCollisionManager::~FGPUCollisionManager()
{
	Release();
}

//=============================================================================
// Lifecycle
//=============================================================================

void FGPUCollisionManager::Initialize()
{
	if (bIsInitialized)
	{
		return;
	}

	// Create feedback manager
	FeedbackManager = MakeUnique<FGPUCollisionFeedbackManager>();
	FeedbackManager->Initialize();

	bIsInitialized = true;
	UE_LOG(LogGPUCollisionManager, Log, TEXT("FGPUCollisionManager initialized"));
}

void FGPUCollisionManager::Release()
{
	if (!bIsInitialized)
	{
		return;
	}

	// Release feedback manager
	if (FeedbackManager.IsValid())
	{
		FeedbackManager->Release();
		FeedbackManager.Reset();
	}

	// Clear cached data
	CachedSpheres.Empty();
	CachedCapsules.Empty();
	CachedBoxes.Empty();
	CachedConvexHeaders.Empty();
	CachedConvexPlanes.Empty();
	CachedBoneTransforms.Empty();

	bCollisionPrimitivesValid = false;
	bBoneTransformsValid = false;
	bIsInitialized = false;

	UE_LOG(LogGPUCollisionManager, Log, TEXT("FGPUCollisionManager released"));
}

//=============================================================================
// Collision Primitives Upload
//=============================================================================

void FGPUCollisionManager::UploadCollisionPrimitives(const FGPUCollisionPrimitives& Primitives)
{
	if (!bIsInitialized)
	{
		return;
	}

	FScopeLock Lock(&CollisionLock);

	// Cache the primitive data (will be uploaded to GPU during simulation)
	CachedSpheres = Primitives.Spheres;
	CachedCapsules = Primitives.Capsules;
	CachedBoxes = Primitives.Boxes;
	CachedConvexHeaders = Primitives.Convexes;
	CachedConvexPlanes = Primitives.ConvexPlanes;
	CachedBoneTransforms = Primitives.BoneTransforms;

	// Check if we have any primitives
	if (Primitives.IsEmpty())
	{
		bCollisionPrimitivesValid = false;
		bBoneTransformsValid = false;
		return;
	}

	bCollisionPrimitivesValid = true;
	bBoneTransformsValid = CachedBoneTransforms.Num() > 0;

	UE_LOG(LogGPUCollisionManager, Verbose, TEXT("Cached collision primitives: Spheres=%d, Capsules=%d, Boxes=%d, Convexes=%d, Planes=%d, BoneTransforms=%d"),
		CachedSpheres.Num(), CachedCapsules.Num(), CachedBoxes.Num(), CachedConvexHeaders.Num(), CachedConvexPlanes.Num(), CachedBoneTransforms.Num());
}

//=============================================================================
// Bounds Collision Pass
//=============================================================================

void FGPUCollisionManager::AddBoundsCollisionPass(
	FRDGBuilder& GraphBuilder,
	FRDGBufferUAVRef ParticlesUAV,
	int32 ParticleCount,
	const FGPUFluidSimulationParams& Params)
{
	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FBoundsCollisionCS> ComputeShader(ShaderMap);

	FBoundsCollisionCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FBoundsCollisionCS::FParameters>();
	PassParameters->Particles = ParticlesUAV;
	PassParameters->ParticleCount = ParticleCount;
	PassParameters->ParticleRadius = Params.ParticleRadius;

	// OBB parameters
	PassParameters->BoundsCenter = Params.BoundsCenter;
	PassParameters->BoundsExtent = Params.BoundsExtent;
	PassParameters->BoundsRotation = Params.BoundsRotation;
	PassParameters->bUseOBB = Params.bUseOBB;

	// Legacy AABB parameters
	PassParameters->BoundsMin = Params.BoundsMin;
	PassParameters->BoundsMax = Params.BoundsMax;

	// Collision response
	PassParameters->Restitution = Params.BoundsRestitution;
	PassParameters->Friction = Params.BoundsFriction;

	const uint32 NumGroups = FMath::DivideAndRoundUp(ParticleCount, FBoundsCollisionCS::ThreadGroupSize);

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("GPUFluid::BoundsCollision"),
		ComputeShader,
		PassParameters,
		FIntVector(NumGroups, 1, 1)
	);
}

//=============================================================================
// Distance Field Collision Pass
//=============================================================================

void FGPUCollisionManager::AddDistanceFieldCollisionPass(
	FRDGBuilder& GraphBuilder,
	FRDGBufferUAVRef ParticlesUAV,
	int32 ParticleCount,
	const FGPUFluidSimulationParams& Params)
{
	// Skip if Distance Field collision is not enabled
	if (!DFCollisionParams.bEnabled || !CachedGDFTexture.IsValid())
	{
		return;
	}

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FDistanceFieldCollisionCS> ComputeShader(ShaderMap);

	// Register external texture with RDG
	FRDGTextureRef GDFTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(CachedGDFTexture, TEXT("GDFTexture")));
	FRDGTextureSRVRef GDFSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(GDFTexture));

	FDistanceFieldCollisionCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FDistanceFieldCollisionCS::FParameters>();
	PassParameters->Particles = ParticlesUAV;
	PassParameters->ParticleCount = ParticleCount;
	PassParameters->ParticleRadius = DFCollisionParams.ParticleRadius;

	// Distance Field Volume Parameters
	PassParameters->GDFVolumeCenter = DFCollisionParams.VolumeCenter;
	PassParameters->GDFVolumeExtent = DFCollisionParams.VolumeExtent;
	PassParameters->GDFVoxelSize = FVector3f(DFCollisionParams.VoxelSize);
	PassParameters->GDFMaxDistance = DFCollisionParams.MaxDistance;

	// Collision Response Parameters
	PassParameters->DFCollisionRestitution = DFCollisionParams.Restitution;
	PassParameters->DFCollisionFriction = DFCollisionParams.Friction;
	PassParameters->DFCollisionThreshold = DFCollisionParams.CollisionThreshold;

	// Global Distance Field Texture
	PassParameters->GlobalDistanceFieldTexture = GDFSRV;
	PassParameters->GlobalDistanceFieldSampler = TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

	const uint32 NumGroups = FMath::DivideAndRoundUp(ParticleCount, FDistanceFieldCollisionCS::ThreadGroupSize);

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("GPUFluid::DistanceFieldCollision"),
		ComputeShader,
		PassParameters,
		FIntVector(NumGroups, 1, 1)
	);
}

//=============================================================================
// Primitive Collision Pass (Spheres, Capsules, Boxes, Convex)
//=============================================================================

void FGPUCollisionManager::AddPrimitiveCollisionPass(
	FRDGBuilder& GraphBuilder,
	FRDGBufferUAVRef ParticlesUAV,
	int32 ParticleCount,
	const FGPUFluidSimulationParams& Params)
{
	// Skip if no collision primitives
	if (!bCollisionPrimitivesValid || GetCollisionPrimitiveCount() == 0)
	{
		return;
	}

	FRDGBufferSRVRef SpheresSRV = nullptr;
	FRDGBufferSRVRef CapsulesSRV = nullptr;
	FRDGBufferSRVRef BoxesSRV = nullptr;
	FRDGBufferSRVRef ConvexesSRV = nullptr;
	FRDGBufferSRVRef ConvexPlanesSRV = nullptr;
	FRDGBufferSRVRef BoneTransformsSRV = nullptr;

	// Dummy data for empty buffers (shader requires all SRVs to be valid)
	static FGPUCollisionSphere DummySphere;
	static FGPUCollisionCapsule DummyCapsule;
	static FGPUCollisionBox DummyBox;
	static FGPUCollisionConvex DummyConvex;
	static FGPUConvexPlane DummyPlane;
	static FGPUBoneTransform DummyBone;

	// Create RDG buffers from cached data (or dummy for empty arrays)
	{
		const bool bHasData = CachedSpheres.Num() > 0;
		FRDGBufferRef SpheresBuffer = CreateStructuredBuffer(
			GraphBuilder,
			TEXT("GPUCollisionSpheres"),
			sizeof(FGPUCollisionSphere),
			bHasData ? CachedSpheres.Num() : 1,
			bHasData ? CachedSpheres.GetData() : &DummySphere,
			bHasData ? CachedSpheres.Num() * sizeof(FGPUCollisionSphere) : sizeof(FGPUCollisionSphere),
			ERDGInitialDataFlags::NoCopy
		);
		SpheresSRV = GraphBuilder.CreateSRV(SpheresBuffer);
	}

	{
		const bool bHasData = CachedCapsules.Num() > 0;
		FRDGBufferRef CapsulesBuffer = CreateStructuredBuffer(
			GraphBuilder,
			TEXT("GPUCollisionCapsules"),
			sizeof(FGPUCollisionCapsule),
			bHasData ? CachedCapsules.Num() : 1,
			bHasData ? CachedCapsules.GetData() : &DummyCapsule,
			bHasData ? CachedCapsules.Num() * sizeof(FGPUCollisionCapsule) : sizeof(FGPUCollisionCapsule),
			ERDGInitialDataFlags::NoCopy
		);
		CapsulesSRV = GraphBuilder.CreateSRV(CapsulesBuffer);
	}

	{
		const bool bHasData = CachedBoxes.Num() > 0;
		FRDGBufferRef BoxesBuffer = CreateStructuredBuffer(
			GraphBuilder,
			TEXT("GPUCollisionBoxes"),
			sizeof(FGPUCollisionBox),
			bHasData ? CachedBoxes.Num() : 1,
			bHasData ? CachedBoxes.GetData() : &DummyBox,
			bHasData ? CachedBoxes.Num() * sizeof(FGPUCollisionBox) : sizeof(FGPUCollisionBox),
			ERDGInitialDataFlags::NoCopy
		);
		BoxesSRV = GraphBuilder.CreateSRV(BoxesBuffer);
	}

	{
		const bool bHasData = CachedConvexHeaders.Num() > 0;
		FRDGBufferRef ConvexesBuffer = CreateStructuredBuffer(
			GraphBuilder,
			TEXT("GPUCollisionConvexes"),
			sizeof(FGPUCollisionConvex),
			bHasData ? CachedConvexHeaders.Num() : 1,
			bHasData ? CachedConvexHeaders.GetData() : &DummyConvex,
			bHasData ? CachedConvexHeaders.Num() * sizeof(FGPUCollisionConvex) : sizeof(FGPUCollisionConvex),
			ERDGInitialDataFlags::NoCopy
		);
		ConvexesSRV = GraphBuilder.CreateSRV(ConvexesBuffer);
	}

	{
		const bool bHasData = CachedConvexPlanes.Num() > 0;
		FRDGBufferRef ConvexPlanesBuffer = CreateStructuredBuffer(
			GraphBuilder,
			TEXT("GPUCollisionConvexPlanes"),
			sizeof(FGPUConvexPlane),
			bHasData ? CachedConvexPlanes.Num() : 1,
			bHasData ? CachedConvexPlanes.GetData() : &DummyPlane,
			bHasData ? CachedConvexPlanes.Num() * sizeof(FGPUConvexPlane) : sizeof(FGPUConvexPlane),
			ERDGInitialDataFlags::NoCopy
		);
		ConvexPlanesSRV = GraphBuilder.CreateSRV(ConvexPlanesBuffer);
	}

	{
		const bool bHasData = CachedBoneTransforms.Num() > 0;
		FRDGBufferRef BoneTransformsBuffer = CreateStructuredBuffer(
			GraphBuilder,
			TEXT("GPUCollisionBoneTransforms"),
			sizeof(FGPUBoneTransform),
			bHasData ? CachedBoneTransforms.Num() : 1,
			bHasData ? CachedBoneTransforms.GetData() : &DummyBone,
			bHasData ? CachedBoneTransforms.Num() * sizeof(FGPUBoneTransform) : sizeof(FGPUBoneTransform),
			ERDGInitialDataFlags::NoCopy
		);
		BoneTransformsSRV = GraphBuilder.CreateSRV(BoneTransformsBuffer);
	}

	// Create collision feedback buffers (for particle -> player interaction)
	FRDGBufferRef FeedbackBuffer = nullptr;
	FRDGBufferRef CounterBuffer = nullptr;
	const bool bFeedbackEnabled = FeedbackManager.IsValid() && FeedbackManager->IsEnabled();

	// Create feedback buffer (persistent across frames for extraction)
	if (bFeedbackEnabled)
	{
		// Create or reuse feedback buffer
		TRefCountPtr<FRDGPooledBuffer>& CollisionFeedbackBuffer = FeedbackManager->GetFeedbackBuffer();
		if (CollisionFeedbackBuffer.IsValid())
		{
			FeedbackBuffer = GraphBuilder.RegisterExternalBuffer(CollisionFeedbackBuffer, TEXT("GPUCollisionFeedback"));
		}
		else
		{
			FRDGBufferDesc FeedbackDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FGPUCollisionFeedback), FGPUCollisionFeedbackManager::MAX_COLLISION_FEEDBACK);
			FeedbackBuffer = GraphBuilder.CreateBuffer(FeedbackDesc, TEXT("GPUCollisionFeedback"));
		}

		// Create counter buffer (reset each frame)
		TRefCountPtr<FRDGPooledBuffer>& CollisionCounterBuffer = FeedbackManager->GetCounterBuffer();
		if (CollisionCounterBuffer.IsValid())
		{
			CounterBuffer = GraphBuilder.RegisterExternalBuffer(CollisionCounterBuffer, TEXT("GPUCollisionCounter"));
		}
		else
		{
			FRDGBufferDesc CounterDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), 1);
			CounterBuffer = GraphBuilder.CreateBuffer(CounterDesc, TEXT("GPUCollisionCounter"));
		}

		// Clear counter at start of frame
		AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(CounterBuffer), 0);
	}
	else
	{
		// Create dummy buffers when feedback is disabled
		FRDGBufferDesc DummyFeedbackDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FGPUCollisionFeedback), 1);
		FeedbackBuffer = GraphBuilder.CreateBuffer(DummyFeedbackDesc, TEXT("GPUCollisionFeedbackDummy"));

		FRDGBufferDesc DummyCounterDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), 1);
		CounterBuffer = GraphBuilder.CreateBuffer(DummyCounterDesc, TEXT("GPUCollisionCounterDummy"));
	}

	// Create collider contact count buffer
	FRDGBufferRef ContactCountBuffer = nullptr;
	if (FeedbackManager.IsValid())
	{
		TRefCountPtr<FRDGPooledBuffer>& ContactCountPooledBuffer = FeedbackManager->GetContactCountBuffer();
		if (ContactCountPooledBuffer.IsValid())
		{
			ContactCountBuffer = GraphBuilder.RegisterExternalBuffer(ContactCountPooledBuffer, TEXT("ColliderContactCounts"));
		}
		else
		{
			FRDGBufferDesc ContactCountDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), FGPUCollisionFeedbackManager::MAX_COLLIDER_COUNT);
			ContactCountBuffer = GraphBuilder.CreateBuffer(ContactCountDesc, TEXT("ColliderContactCounts"));
		}
	}
	else
	{
		FRDGBufferDesc ContactCountDesc = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), FGPUCollisionFeedbackManager::MAX_COLLIDER_COUNT);
		ContactCountBuffer = GraphBuilder.CreateBuffer(ContactCountDesc, TEXT("ColliderContactCounts"));
	}

	// Clear contact counts at start of frame
	AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(ContactCountBuffer), 0);

	// Dispatch primitive collision shader directly (with feedback buffers)
	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FPrimitiveCollisionCS> ComputeShader(GlobalShaderMap);

	FPrimitiveCollisionCS::FParameters* PassParameters =
		GraphBuilder.AllocParameters<FPrimitiveCollisionCS::FParameters>();

	PassParameters->Particles = ParticlesUAV;
	PassParameters->ParticleCount = ParticleCount;
	PassParameters->ParticleRadius = Params.ParticleRadius;
	PassParameters->CollisionThreshold = PrimitiveCollisionThreshold;

	PassParameters->CollisionSpheres = SpheresSRV;
	PassParameters->SphereCount = CachedSpheres.Num();

	PassParameters->CollisionCapsules = CapsulesSRV;
	PassParameters->CapsuleCount = CachedCapsules.Num();

	PassParameters->CollisionBoxes = BoxesSRV;
	PassParameters->BoxCount = CachedBoxes.Num();

	PassParameters->CollisionConvexes = ConvexesSRV;
	PassParameters->ConvexCount = CachedConvexHeaders.Num();

	PassParameters->ConvexPlanes = ConvexPlanesSRV;
	PassParameters->BoneTransforms = BoneTransformsSRV;
	PassParameters->BoneCount = CachedBoneTransforms.Num();

	// Collision feedback parameters
	PassParameters->CollisionFeedback = GraphBuilder.CreateUAV(FeedbackBuffer);
	PassParameters->CollisionCounter = GraphBuilder.CreateUAV(CounterBuffer);
	PassParameters->MaxCollisionFeedback = FGPUCollisionFeedbackManager::MAX_COLLISION_FEEDBACK;
	PassParameters->bEnableCollisionFeedback = bFeedbackEnabled ? 1 : 0;

	// Collider contact count parameters
	PassParameters->ColliderContactCounts = GraphBuilder.CreateUAV(ContactCountBuffer);
	PassParameters->MaxColliderCount = FGPUCollisionFeedbackManager::MAX_COLLIDER_COUNT;

	const int32 ThreadGroupSize = FPrimitiveCollisionCS::ThreadGroupSize;
	const int32 NumGroups = FMath::DivideAndRoundUp(ParticleCount, ThreadGroupSize);

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("GPUFluid::PrimitiveCollision(%d particles, %d primitives, feedback=%s)",
			ParticleCount, CachedSpheres.Num() + CachedCapsules.Num() + CachedBoxes.Num() + CachedConvexHeaders.Num(),
			bFeedbackEnabled ? TEXT("ON") : TEXT("OFF")),
		ComputeShader,
		PassParameters,
		FIntVector(NumGroups, 1, 1));

	// Extract feedback buffers for next frame (only if feedback is enabled)
	if (bFeedbackEnabled && FeedbackManager.IsValid())
	{
		GraphBuilder.QueueBufferExtraction(
			FeedbackBuffer,
			&FeedbackManager->GetFeedbackBuffer(),
			ERHIAccess::UAVCompute
		);

		GraphBuilder.QueueBufferExtraction(
			CounterBuffer,
			&FeedbackManager->GetCounterBuffer(),
			ERHIAccess::UAVCompute
		);
	}

	// Always extract collider contact count buffer (if manager valid)
	if (FeedbackManager.IsValid())
	{
		GraphBuilder.QueueBufferExtraction(
			ContactCountBuffer,
			&FeedbackManager->GetContactCountBuffer(),
			ERHIAccess::UAVCompute
		);
	}
}

//=============================================================================
// Collision Feedback
//=============================================================================

void FGPUCollisionManager::SetCollisionFeedbackEnabled(bool bEnabled)
{
	if (FeedbackManager.IsValid())
	{
		FeedbackManager->SetEnabled(bEnabled);
	}
}

bool FGPUCollisionManager::IsCollisionFeedbackEnabled() const
{
	return FeedbackManager.IsValid() && FeedbackManager->IsEnabled();
}

void FGPUCollisionManager::AllocateCollisionFeedbackBuffers(FRHICommandListImmediate& RHICmdList)
{
	if (FeedbackManager.IsValid())
	{
		FeedbackManager->AllocateReadbackObjects(RHICmdList);
	}
}

void FGPUCollisionManager::ReleaseCollisionFeedbackBuffers()
{
	// Manager release is handled in Release()
}

void FGPUCollisionManager::ProcessCollisionFeedbackReadback(FRHICommandListImmediate& RHICmdList)
{
	if (FeedbackManager.IsValid())
	{
		FeedbackManager->ProcessFeedbackReadback(RHICmdList);
	}
}

void FGPUCollisionManager::ProcessColliderContactCountReadback(FRHICommandListImmediate& RHICmdList)
{
	if (FeedbackManager.IsValid())
	{
		FeedbackManager->ProcessContactCountReadback(RHICmdList);
	}
}

bool FGPUCollisionManager::GetCollisionFeedbackForCollider(int32 ColliderIndex, TArray<FGPUCollisionFeedback>& OutFeedback, int32& OutCount)
{
	if (!FeedbackManager.IsValid())
	{
		OutFeedback.Reset();
		OutCount = 0;
		return false;
	}
	return FeedbackManager->GetFeedbackForCollider(ColliderIndex, OutFeedback, OutCount);
}

bool FGPUCollisionManager::GetAllCollisionFeedback(TArray<FGPUCollisionFeedback>& OutFeedback, int32& OutCount)
{
	if (!FeedbackManager.IsValid())
	{
		OutFeedback.Reset();
		OutCount = 0;
		return false;
	}
	return FeedbackManager->GetAllFeedback(OutFeedback, OutCount);
}

int32 FGPUCollisionManager::GetCollisionFeedbackCount() const
{
	return FeedbackManager.IsValid() ? FeedbackManager->GetFeedbackCount() : 0;
}

int32 FGPUCollisionManager::GetColliderContactCount(int32 ColliderIndex) const
{
	if (!FeedbackManager.IsValid())
	{
		return 0;
	}
	return FeedbackManager->GetContactCount(ColliderIndex);
}

void FGPUCollisionManager::GetAllColliderContactCounts(TArray<int32>& OutCounts) const
{
	if (!FeedbackManager.IsValid())
	{
		OutCounts.Empty();
		return;
	}
	FeedbackManager->GetAllContactCounts(OutCounts);
}

int32 FGPUCollisionManager::GetContactCountForOwner(int32 OwnerID) const
{
	// Debug logging (every 60 frames)
	static int32 OwnerCountDebugFrame = 0;
	const bool bLogThisFrame = (OwnerCountDebugFrame++ % 60 == 0);

	int32 TotalCount = 0;
	int32 ColliderIndex = 0;
	int32 MatchedColliders = 0;

	// Spheres: indices 0 to SphereCount-1
	for (int32 i = 0; i < CachedSpheres.Num(); ++i)
	{
		if (CachedSpheres[i].OwnerID == OwnerID)
		{
			TotalCount += GetColliderContactCount(ColliderIndex);
			MatchedColliders++;
		}
		ColliderIndex++;
	}

	// Capsules: indices SphereCount to SphereCount+CapsuleCount-1
	for (int32 i = 0; i < CachedCapsules.Num(); ++i)
	{
		if (CachedCapsules[i].OwnerID == OwnerID)
		{
			TotalCount += GetColliderContactCount(ColliderIndex);
			MatchedColliders++;
		}
		ColliderIndex++;
	}

	// Boxes: indices SphereCount+CapsuleCount to SphereCount+CapsuleCount+BoxCount-1
	for (int32 i = 0; i < CachedBoxes.Num(); ++i)
	{
		if (CachedBoxes[i].OwnerID == OwnerID)
		{
			TotalCount += GetColliderContactCount(ColliderIndex);
			MatchedColliders++;
		}
		ColliderIndex++;
	}

	// Convexes: remaining indices
	for (int32 i = 0; i < CachedConvexHeaders.Num(); ++i)
	{
		if (CachedConvexHeaders[i].OwnerID == OwnerID)
		{
			TotalCount += GetColliderContactCount(ColliderIndex);
			MatchedColliders++;
		}
		ColliderIndex++;
	}

	if (bLogThisFrame && MatchedColliders > 0)
	{
		UE_LOG(LogGPUCollisionManager, Log, TEXT("[ContactCountForOwner] OwnerID=%d, MatchedColliders=%d, TotalCount=%d"),
			OwnerID, MatchedColliders, TotalCount);
	}

	return TotalCount;
}
