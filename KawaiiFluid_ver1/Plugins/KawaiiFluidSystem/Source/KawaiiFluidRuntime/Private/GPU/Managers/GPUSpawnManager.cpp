// Copyright KawaiiFluid Team. All Rights Reserved.
// FGPUSpawnManager - Thread-safe particle spawn queue manager

#include "GPU/Managers/GPUSpawnManager.h"
#include "GPU/GPUFluidSimulatorShaders.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHIGPUReadback.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGPUSpawnManager, Log, All);
DEFINE_LOG_CATEGORY(LogGPUSpawnManager);

//=============================================================================
// Constructor / Destructor
//=============================================================================

FGPUSpawnManager::FGPUSpawnManager()
	: bIsInitialized(false)
	, MaxParticleCapacity(0)
{
}

FGPUSpawnManager::~FGPUSpawnManager()
{
	Release();
}

//=============================================================================
// Lifecycle
//=============================================================================

void FGPUSpawnManager::Initialize(int32 InMaxParticleCount)
{
	if (InMaxParticleCount <= 0)
	{
		UE_LOG(LogGPUSpawnManager, Warning, TEXT("Initialize called with invalid particle count: %d"), InMaxParticleCount);
		return;
	}

	MaxParticleCapacity = InMaxParticleCount;
	bIsInitialized = true;

	UE_LOG(LogGPUSpawnManager, Log, TEXT("GPUSpawnManager initialized with capacity: %d"), MaxParticleCapacity);
}

void FGPUSpawnManager::Release()
{
	FScopeLock Lock(&SpawnLock);

	PendingSpawnRequests.Empty();
	ActiveSpawnRequests.Empty();
	bHasPendingSpawnRequests.store(false);
	NextParticleID.store(0);
	bIsInitialized = false;
	MaxParticleCapacity = 0;

	UE_LOG(LogGPUSpawnManager, Log, TEXT("GPUSpawnManager released"));
}

//=============================================================================
// Thread-Safe Public API
//=============================================================================

void FGPUSpawnManager::AddSpawnRequest(const FVector3f& Position, const FVector3f& Velocity, float Mass)
{
	FScopeLock Lock(&SpawnLock);

	FGPUSpawnRequest Request;
	Request.Position = Position;
	Request.Velocity = Velocity;
	Request.Mass = Mass;
	Request.Radius = DefaultSpawnRadius;

	PendingSpawnRequests.Add(Request);
	bHasPendingSpawnRequests.store(true);

	UE_LOG(LogGPUSpawnManager, Verbose, TEXT("AddSpawnRequest: Pos=(%.2f, %.2f, %.2f), Vel=(%.2f, %.2f, %.2f)"),
		Position.X, Position.Y, Position.Z, Velocity.X, Velocity.Y, Velocity.Z);
}

void FGPUSpawnManager::AddSpawnRequests(const TArray<FGPUSpawnRequest>& Requests)
{
	if (Requests.Num() == 0)
	{
		return;
	}

	FScopeLock Lock(&SpawnLock);

	PendingSpawnRequests.Append(Requests);
	bHasPendingSpawnRequests.store(true);

	UE_LOG(LogGPUSpawnManager, Verbose, TEXT("AddSpawnRequests: Added %d requests (total pending: %d)"),
		Requests.Num(), PendingSpawnRequests.Num());
}

void FGPUSpawnManager::ClearSpawnRequests()
{
	FScopeLock Lock(&SpawnLock);
	PendingSpawnRequests.Empty();
	bHasPendingSpawnRequests.store(false);
}

int32 FGPUSpawnManager::GetPendingSpawnCount() const
{
	FScopeLock Lock(&SpawnLock);
	return PendingSpawnRequests.Num();
}

void FGPUSpawnManager::AddDespawnRequest(const FVector& Position, float Radius)
{
	FScopeLock Lock(&SpawnLock);

	PendingDespawnRequests.Add(FGPUDespawnRequest(static_cast<FVector3f>(Position), Radius));
}

void FGPUSpawnManager::SwapDespawnBuffers()
{
	FScopeLock Lock(&DespawnLock);
	
	if (PendingDespawnRequests.Num() > 0)
	{
		ActiveDespawnRequests.Append(PendingDespawnRequests);
		PendingDespawnRequests.Empty();
	}
}

int32 FGPUSpawnManager::GetPendingDespawnCount() const
{
	FScopeLock Lock(&DespawnLock);
	return PendingDespawnRequests.Num();
}

//=============================================================================
// Render Thread API
//=============================================================================

void FGPUSpawnManager::SwapBuffers()
{
	FScopeLock Lock(&SpawnLock);

	// Move pending requests to active buffer
	ActiveSpawnRequests = MoveTemp(PendingSpawnRequests);
	PendingSpawnRequests.Empty();
	bHasPendingSpawnRequests.store(false);
}

void FGPUSpawnManager::AddSpawnParticlesPass(
	FRDGBuilder& GraphBuilder,
	FRDGBufferUAVRef ParticlesUAV,
	FRDGBufferUAVRef ParticleCounterUAV,
	int32 MaxParticleCount)
{
	if (ActiveSpawnRequests.Num() == 0)
	{
		return;
	}

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FSpawnParticlesCS> ComputeShader(ShaderMap);

	// Create spawn request buffer
	// IMPORTANT: Do NOT use NoCopy - SpawnRequests is temporary data that may be
	// invalidated before RDG pass executes. RDG must copy the data.
	FRDGBufferRef SpawnRequestBuffer = CreateStructuredBuffer(
		GraphBuilder,
		TEXT("GPUFluidSpawnRequests"),
		sizeof(FGPUSpawnRequest),
		ActiveSpawnRequests.Num(),
		ActiveSpawnRequests.GetData(),
		ActiveSpawnRequests.Num() * sizeof(FGPUSpawnRequest),
		ERDGInitialDataFlags::None
	);

	FSpawnParticlesCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FSpawnParticlesCS::FParameters>();
	PassParameters->SpawnRequests = GraphBuilder.CreateSRV(SpawnRequestBuffer);
	PassParameters->Particles = ParticlesUAV;
	PassParameters->ParticleCounter = ParticleCounterUAV;
	PassParameters->SpawnRequestCount = ActiveSpawnRequests.Num();
	PassParameters->MaxParticleCount = MaxParticleCount;
	PassParameters->NextParticleID = NextParticleID.load();
	PassParameters->DefaultRadius = DefaultSpawnRadius;
	PassParameters->DefaultMass = DefaultSpawnMass;

	const uint32 NumGroups = FMath::DivideAndRoundUp(ActiveSpawnRequests.Num(), FSpawnParticlesCS::ThreadGroupSize);

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("GPUFluid::SpawnParticles(%d)", ActiveSpawnRequests.Num()),
		ComputeShader,
		PassParameters,
		FIntVector(NumGroups, 1, 1)
	);

	UE_LOG(LogGPUSpawnManager, Verbose, TEXT("SpawnParticlesPass: Spawning %d particles (NextID: %d)"),
		ActiveSpawnRequests.Num(), NextParticleID.load());
}

void FGPUSpawnManager::AddDespawnPass(FRDGBuilder& GraphBuilder, FRDGBufferRef& InOutParticleBuffer,
	int32& InOutParticleCount)
{
	if (ActiveDespawnRequests.Num() == 0)
	{
		return;
	}

	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FMarkDespawnCS> MarkDespawnCS(ShaderMap);

	FRDGBufferRef DespawnRequestsBuffer = CreateStructuredBuffer
	(
		GraphBuilder,
		TEXT("GPUFluidDespawnRequests"),
		sizeof(FGPUDespawnRequest),
		ActiveDespawnRequests.Num(),
		ActiveDespawnRequests.GetData(),
		ActiveDespawnRequests.Num() * sizeof(FGPUDespawnRequest),
		ERDGInitialDataFlags::None
		);

	FRDGBufferRef AliveMaskBuffer = CreateStructuredBuffer(
		GraphBuilder,
		TEXT("GPUFluidOutAliveMask"),
		sizeof(uint32),
		InOutParticleCount,
		nullptr,
		0,
		ERDGInitialDataFlags::None
	);

	// 제거할 파티클 마크
	FMarkDespawnCS::FParameters* MarkPassParameters = GraphBuilder.AllocParameters<FMarkDespawnCS::FParameters>();
	MarkPassParameters->DespawnRequests = GraphBuilder.CreateSRV(DespawnRequestsBuffer);
	MarkPassParameters->Particles = GraphBuilder.CreateSRV(InOutParticleBuffer);
	MarkPassParameters->OutAliveMask = GraphBuilder.CreateUAV(AliveMaskBuffer);
	MarkPassParameters->DespawnRequestCount = ActiveDespawnRequests.Num();
	MarkPassParameters->ParticleCount = InOutParticleCount;

	const uint32 MarkPassNumGroups = FMath::DivideAndRoundUp(InOutParticleCount, FMarkDespawnCS::ThreadGroupSize);
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("GPUFluid::Despawn_MarkDown(%d)", ActiveDespawnRequests.Num()),
		MarkDespawnCS,
		MarkPassParameters,
		FIntVector(MarkPassNumGroups, 1, 1)
	);

	FRDGBufferRef PrefixSumsBuffer = CreateStructuredBuffer(
	GraphBuilder,
	TEXT("PrefixSums"),
	sizeof(uint32),
	InOutParticleCount,
	nullptr,
	0,
	ERDGInitialDataFlags::None
	);

	FRDGBufferRef BlockSumsBuffer = CreateStructuredBuffer(
		GraphBuilder,
		TEXT("BlockSums"),
		sizeof(uint32),
		FMath::DivideAndRoundUp(InOutParticleCount, FPrefixSumBlockCS_RDG::ThreadGroupSize),
		nullptr,
		0,
		ERDGInitialDataFlags::None
	);

	// 블록 별 프리픽스 계산
	TShaderMapRef<FPrefixSumBlockCS_RDG> PrefixSumBlock(ShaderMap);
	FPrefixSumBlockCS_RDG::FParameters* PrefixSumBlockParameters = GraphBuilder.AllocParameters<FPrefixSumBlockCS_RDG::FParameters>();
	PrefixSumBlockParameters->MarkedFlags = GraphBuilder.CreateSRV(AliveMaskBuffer);
	PrefixSumBlockParameters->PrefixSums = GraphBuilder.CreateUAV(PrefixSumsBuffer);
	PrefixSumBlockParameters->BlockSums = GraphBuilder.CreateUAV(BlockSumsBuffer);
	PrefixSumBlockParameters->ElementCount = InOutParticleCount;

	const int32 PrefixSumBlockNumGroups = FMath::DivideAndRoundUp(InOutParticleCount, FPrefixSumBlockCS_RDG::ThreadGroupSize);
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("GPUFluid::PrefixSumBlock"),
		PrefixSumBlock,
		PrefixSumBlockParameters,
		FIntVector(PrefixSumBlockNumGroups, 1, 1)
	);

	// 블록 당 프리픽스 합 계산
	TShaderMapRef<FScanBlockSumsCS_RDG> ScanBlockSums(ShaderMap);
	FScanBlockSumsCS_RDG::FParameters* ScanBlockSumsParameters = GraphBuilder.AllocParameters<FScanBlockSumsCS_RDG::FParameters>();
	ScanBlockSumsParameters->BlockSums = GraphBuilder.CreateUAV(BlockSumsBuffer);
	ScanBlockSumsParameters->BlockCount = PrefixSumBlockNumGroups;

	const int32 ScanBlockSumsNumGroups = FMath::DivideAndRoundUp(PrefixSumBlockNumGroups, FScanBlockSumsCS_RDG::ThreadGroupSize);
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("GPUFluid::ScanBlockSums"),
		ScanBlockSums,
		ScanBlockSumsParameters,
		FIntVector(ScanBlockSumsNumGroups, 1, 1)
	);

	// 모든 프리픽스에 블록 합 가산
	TShaderMapRef<FAddBlockOffsetsCS_RDG> AddBlockOffsets(ShaderMap);
	FAddBlockOffsetsCS_RDG::FParameters* AddBlockOffsetsParameters = GraphBuilder.AllocParameters<FAddBlockOffsetsCS_RDG::FParameters>();
	AddBlockOffsetsParameters->PrefixSums = GraphBuilder.CreateUAV(PrefixSumsBuffer);
	AddBlockOffsetsParameters->BlockSums = GraphBuilder.CreateUAV(BlockSumsBuffer);
	AddBlockOffsetsParameters->ElementCount = InOutParticleCount;

	const int32 AddBlockOffsetsNumGroups = FMath::DivideAndRoundUp(InOutParticleCount, FPrefixSumBlockCS_RDG::ThreadGroupSize);
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("GPUFluid::AddBlockOffsets"),
		AddBlockOffsets,
		AddBlockOffsetsParameters,
		FIntVector(AddBlockOffsetsNumGroups, 1, 1)
	);


	FRDGBufferRef CompactedParticlesBuffer = CreateStructuredBuffer(
	GraphBuilder,
	TEXT("CompactedParticles"),
	sizeof(FGPUFluidParticle),
	InOutParticleCount,
	nullptr,
	0,
	ERDGInitialDataFlags::None
	);

	// 프리픽스를 통해 파티클 버퍼 최신화
	TShaderMapRef<FCompactParticlesCS_RDG> Compact(ShaderMap);
	FCompactParticlesCS_RDG::FParameters* CompactParameters = GraphBuilder.AllocParameters<FCompactParticlesCS_RDG::FParameters>();
	CompactParameters->Particles = GraphBuilder.CreateSRV(InOutParticleBuffer);
	CompactParameters->MarkedFlags = GraphBuilder.CreateSRV(AliveMaskBuffer);
	CompactParameters->PrefixSums = GraphBuilder.CreateSRV(PrefixSumsBuffer);
	CompactParameters->CompactedParticles = GraphBuilder.CreateUAV(CompactedParticlesBuffer);
	CompactParameters->ParticleCount = InOutParticleCount;

	const int32 CompactCSNumGroups = FMath::DivideAndRoundUp(InOutParticleCount, FPrefixSumBlockCS_RDG::ThreadGroupSize);
	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("GPUFluid::Compact"),
		Compact,
		CompactParameters,
		FIntVector(CompactCSNumGroups, 1, 1)
	);

	// 버퍼 업데이트
	InOutParticleBuffer = CompactedParticlesBuffer;

	FRDGBufferRef TotalCountBuffer = GraphBuilder.CreateBuffer(
		FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), 1),
		TEXT("Despawn.TotalCount")
	);
	
	// 파티클 카운트 비동기 리드백
	TShaderMapRef<FWriteTotalCountCS_RDG> WriteTotalCount(ShaderMap);
	FWriteTotalCountCS_RDG::FParameters* WriteTotalCountParameters = GraphBuilder.AllocParameters<FWriteTotalCountCS_RDG::FParameters>();
	WriteTotalCountParameters->PrefixSums = GraphBuilder.CreateSRV(PrefixSumsBuffer);
	WriteTotalCountParameters->MarkedFlags = GraphBuilder.CreateSRV(AliveMaskBuffer);
	WriteTotalCountParameters->OutTotalCount = GraphBuilder.CreateUAV(TotalCountBuffer);
	WriteTotalCountParameters->ParticleCount = InOutParticleCount;

	FComputeShaderUtils::AddPass(
		GraphBuilder,
		RDG_EVENT_NAME("GPUFluid::WriteTotalCount"),
		WriteTotalCount,
		WriteTotalCountParameters,
		FIntVector(1, 1, 1)
	);

	if (!ParticleCountReadback)
	{
		ParticleCountReadback = new FRHIGPUBufferReadback(TEXT("FluidParticleCountReadback"));
	}
	AddEnqueueCopyPass(GraphBuilder, ParticleCountReadback, TotalCountBuffer, 0);
	bDespawnPassExecuted = true;
}

int32 FGPUSpawnManager::ProcessAsyncReadback()
{
	if (ParticleCountReadback && ParticleCountReadback->IsReady())
	{
		uint32* BufferData = static_cast<uint32*>(ParticleCountReadback->Lock(sizeof(uint32)));
		int32 DeadCount = static_cast<int32>(BufferData[0]);

		ParticleCountReadback->Unlock();

		return DeadCount;
	}

	return -1;
}

void FGPUSpawnManager::OnSpawnComplete(int32 SpawnedCount)
{
	if (SpawnedCount > 0)
	{
		NextParticleID.fetch_add(SpawnedCount);
	}
}
