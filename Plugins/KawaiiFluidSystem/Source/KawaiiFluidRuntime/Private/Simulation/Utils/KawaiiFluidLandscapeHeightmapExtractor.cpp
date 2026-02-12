// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "Simulation/Utils/KawaiiFluidLandscapeHeightmapExtractor.h"
#include "LandscapeComponent.h"
#include "EngineUtils.h"
#include "LandscapeProxy.h"
#include "Async/ParallelFor.h"

DEFINE_LOG_CATEGORY_STATIC(LogHeightmapExtractor, Log, All);

/**
 * @struct FHeightMinMax
 * @brief Thread-local tracking for parallel height sampling.
 * @param MinZ Minimum height found by the thread.
 * @param MaxZ Maximum height found by the thread.
 */
struct FHeightMinMax
{
	float MinZ = FLT_MAX;
	float MaxZ = -FLT_MAX;
};

/**
 * @brief Extract normalized heightmap data from a single landscape actor.
 * @param Landscape Source landscape.
 * @param OutHeightData Output array of normalized heights (0-1).
 * @param OutWidth Width of the generated heightmap.
 * @param OutHeight Height of the generated heightmap.
 * @param OutBounds Output world-space bounds of the extracted area.
 * @param Resolution Desired resolution (clamped to power of 2).
 * @return True if extraction was successful.
 */
bool FKawaiiFluidLandscapeHeightmapExtractor::ExtractHeightmap(
	ALandscapeProxy* Landscape,
	TArray<float>& OutHeightData,
	int32& OutWidth,
	int32& OutHeight,
	FBox& OutBounds,
	int32 Resolution)
{
	if (!Landscape)
	{
		UE_LOG(LogHeightmapExtractor, Warning, TEXT("ExtractHeightmap: Landscape is null"));
		return false;
	}

	Resolution = ClampToPowerOfTwo(Resolution);
	OutWidth = Resolution;
	OutHeight = Resolution;

	OutBounds = Landscape->GetComponentsBoundingBox(true);

	if (!OutBounds.IsValid)
	{
		UE_LOG(LogHeightmapExtractor, Warning, TEXT("ExtractHeightmap: Invalid landscape bounds"));
		return false;
	}

	const float Padding = 10.0f;
	OutBounds = OutBounds.ExpandBy(FVector(Padding, Padding, 0.0f));

	const FVector BoundsSize = OutBounds.GetSize();
	const float StepX = BoundsSize.X / static_cast<float>(Resolution - 1);
	const float StepY = BoundsSize.Y / static_cast<float>(Resolution - 1);

	OutHeightData.SetNumUninitialized(Resolution * Resolution);

	float MinZ = OutBounds.Max.Z;
	float MaxZ = OutBounds.Min.Z;

	const int32 NumThreads = FMath::Max(1, FPlatformMisc::NumberOfWorkerThreadsToSpawn() + 1);
	TArray<FHeightMinMax> ThreadMinMax;
	ThreadMinMax.SetNum(NumThreads);

	ParallelFor(Resolution, [&](int32 y)
	{
		const int32 ThreadIdx = FPlatformTLS::GetCurrentThreadId() % NumThreads;
		FHeightMinMax& LocalMinMax = ThreadMinMax[ThreadIdx];

		const float WorldY = OutBounds.Min.Y + y * StepY;

		for (int32 x = 0; x < Resolution; ++x)
		{
			const float WorldX = OutBounds.Min.X + x * StepX;
			const float Height = SampleLandscapeHeight(Landscape, WorldX, WorldY);

			LocalMinMax.MinZ = FMath::Min(LocalMinMax.MinZ, Height);
			LocalMinMax.MaxZ = FMath::Max(LocalMinMax.MaxZ, Height);

			OutHeightData[y * Resolution + x] = Height;
		}
	});

	for (const FHeightMinMax& Local : ThreadMinMax)
	{
		MinZ = FMath::Min(MinZ, Local.MinZ);
		MaxZ = FMath::Max(MaxZ, Local.MaxZ);
	}

	OutBounds.Min.Z = MinZ - Padding;
	OutBounds.Max.Z = MaxZ + Padding;

	const float PaddedMinZ = OutBounds.Min.Z;
	const float PaddedMaxZ = OutBounds.Max.Z;
	const float HeightRange = PaddedMaxZ - PaddedMinZ;
	if (HeightRange > SMALL_NUMBER)
	{
		const float InvHeightRange = 1.0f / HeightRange;
		for (float& Height : OutHeightData)
		{
			Height = (Height - PaddedMinZ) * InvHeightRange;
		}
	}
	else
	{
		for (float& Height : OutHeightData)
		{
			Height = 0.5f;
		}
	}

	UE_LOG(LogHeightmapExtractor, Log, TEXT("Extracted heightmap from %s: %dx%d, Bounds: (%.1f,%.1f,%.1f) - (%.1f,%.1f,%.1f)"),
		*Landscape->GetName(), OutWidth, OutHeight,
		OutBounds.Min.X, OutBounds.Min.Y, OutBounds.Min.Z,
		OutBounds.Max.X, OutBounds.Max.Y, OutBounds.Max.Z);

	return true;
}

/**
 * @brief Combine heightmap data from multiple landscape actors into a single unified map.
 * @param Landscapes Array of landscape actors.
 * @param OutHeightData Output array of normalized heights.
 * @param OutWidth Width of the unified map.
 * @param OutHeight Height of the unified map.
 * @param OutBounds Combined world-space bounds.
 * @param Resolution Target resolution for the combined map.
 * @return True if extraction and merging succeeded.
 */
bool FKawaiiFluidLandscapeHeightmapExtractor::ExtractCombinedHeightmap(
	const TArray<ALandscapeProxy*>& Landscapes,
	TArray<float>& OutHeightData,
	int32& OutWidth,
	int32& OutHeight,
	FBox& OutBounds,
	int32 Resolution)
{
	if (Landscapes.Num() == 0)
	{
		UE_LOG(LogHeightmapExtractor, Warning, TEXT("ExtractCombinedHeightmap: No landscapes provided"));
		return false;
	}

	if (Landscapes.Num() == 1)
	{
		return ExtractHeightmap(Landscapes[0], OutHeightData, OutWidth, OutHeight, OutBounds, Resolution);
	}

	Resolution = ClampToPowerOfTwo(Resolution);
	OutWidth = Resolution;
	OutHeight = Resolution;

	OutBounds = FBox(ForceInit);
	for (ALandscapeProxy* Landscape : Landscapes)
	{
		if (Landscape)
		{
			OutBounds += Landscape->GetComponentsBoundingBox(true);
		}
	}

	if (!OutBounds.IsValid)
	{
		UE_LOG(LogHeightmapExtractor, Warning, TEXT("ExtractCombinedHeightmap: Invalid combined bounds"));
		return false;
	}

	const float Padding = 10.0f;
	OutBounds = OutBounds.ExpandBy(FVector(Padding, Padding, 0.0f));

	const FVector BoundsSize = OutBounds.GetSize();
	const float StepX = BoundsSize.X / (float)(Resolution - 1);
	const float StepY = BoundsSize.Y / (float)(Resolution - 1);

	OutHeightData.SetNumUninitialized(Resolution * Resolution);

	float MinZ = OutBounds.Max.Z;
	float MaxZ = OutBounds.Min.Z;

	const int32 NumThreads = FPlatformMisc::NumberOfWorkerThreadsToSpawn() + 1;
	TArray<FHeightMinMax> ThreadMinMax;
	ThreadMinMax.SetNum(NumThreads);

	TArray<FBox> LandscapeBoundsCache;
	LandscapeBoundsCache.SetNum(Landscapes.Num());
	for (int32 i = 0; i < Landscapes.Num(); ++i)
	{
		if (Landscapes[i])
		{
			LandscapeBoundsCache[i] = Landscapes[i]->GetComponentsBoundingBox(true);
		}
	}

	ParallelFor(Resolution, [&](int32 y)
	{
		const int32 ThreadIdx = FPlatformTLS::GetCurrentThreadId() % NumThreads;
		FHeightMinMax& LocalMinMax = ThreadMinMax[ThreadIdx];

		const float WorldY = OutBounds.Min.Y + y * StepY;

		for (int32 x = 0; x < Resolution; ++x)
		{
			const float WorldX = OutBounds.Min.X + x * StepX;

			float Height = OutBounds.Min.Z;
			bool bFoundHeight = false;

			for (int32 i = 0; i < Landscapes.Num(); ++i)
			{
				ALandscapeProxy* Landscape = Landscapes[i];
				if (!Landscape) continue;

				const FBox& LandscapeBounds = LandscapeBoundsCache[i];
				if (WorldX >= LandscapeBounds.Min.X && WorldX <= LandscapeBounds.Max.X &&
					WorldY >= LandscapeBounds.Min.Y && WorldY <= LandscapeBounds.Max.Y)
				{
					Height = SampleLandscapeHeight(Landscape, WorldX, WorldY);
					bFoundHeight = true;
					break;
				}
			}

			if (bFoundHeight)
			{
				LocalMinMax.MinZ = FMath::Min(LocalMinMax.MinZ, Height);
				LocalMinMax.MaxZ = FMath::Max(LocalMinMax.MaxZ, Height);
			}

			OutHeightData[y * Resolution + x] = Height;
		}
	});

	for (const FHeightMinMax& Local : ThreadMinMax)
	{
		MinZ = FMath::Min(MinZ, Local.MinZ);
		MaxZ = FMath::Max(MaxZ, Local.MaxZ);
	}

	OutBounds.Min.Z = MinZ - Padding;
	OutBounds.Max.Z = MaxZ + Padding;

	const float PaddedMinZ = OutBounds.Min.Z;
	const float PaddedMaxZ = OutBounds.Max.Z;
	const float HeightRange = PaddedMaxZ - PaddedMinZ;
	if (HeightRange > SMALL_NUMBER)
	{
		const float InvHeightRange = 1.0f / HeightRange;
		for (float& Height : OutHeightData)
		{
			Height = FMath::Clamp((Height - PaddedMinZ) * InvHeightRange, 0.0f, 1.0f);
		}
	}
	else
	{
		for (float& Height : OutHeightData)
		{
			Height = 0.5f;
		}
	}

	UE_LOG(LogHeightmapExtractor, Log, TEXT("Extracted combined heightmap from %d landscapes: %dx%d"),
		Landscapes.Num(), OutWidth, OutHeight);

	return true;
}

/**
 * @brief Build GPU-compatible collision parameters from extracted heightmap metadata.
 */
FGPUHeightmapCollisionParams FKawaiiFluidLandscapeHeightmapExtractor::BuildCollisionParams(
	const FBox& Bounds,
	int32 Width,
	int32 Height,
	float ParticleRadius,
	float Friction,
	float Restitution)
{
	FGPUHeightmapCollisionParams Params;

	Params.WorldMin = FVector3f(Bounds.Min);
	Params.WorldMax = FVector3f(Bounds.Max);
	Params.TextureWidth = Width;
	Params.TextureHeight = Height;
	Params.ParticleRadius = ParticleRadius;
	Params.Friction = Friction;
	Params.Restitution = Restitution;
	Params.NormalStrength = 1.0f;
	Params.CollisionOffset = 0.0f;
	Params.bEnabled = 1;

	Params.UpdateInverseValues();

	return Params;
}

/**
 * @brief Find and collect all landscape actors present in the specified world.
 */
void FKawaiiFluidLandscapeHeightmapExtractor::FindLandscapesInWorld(UWorld* World, TArray<ALandscapeProxy*>& OutLandscapes)
{
	OutLandscapes.Reset();

	if (!World)
	{
		return;
	}

	for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
	{
		OutLandscapes.Add(*It);
	}

	UE_LOG(LogHeightmapExtractor, Log, TEXT("Found %d landscapes in world"), OutLandscapes.Num());
}

/**
 * @brief Internal helper to sample the exact Z height of a landscape at a world XY position.
 */
float FKawaiiFluidLandscapeHeightmapExtractor::SampleLandscapeHeight(ALandscapeProxy* Landscape, float WorldX, float WorldY)
{
	if (!Landscape)
	{
		return 0.0f;
	}

	FVector Location(WorldX, WorldY, 0.0f);
	TOptional<float> Height = Landscape->GetHeightAtLocation(Location);

	if (Height.IsSet())
	{
		return Height.GetValue();
	}

	FVector Start(WorldX, WorldY, 100000.0f);
	FVector End(WorldX, WorldY, -100000.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;

	if (Landscape->GetWorld() &&
		Landscape->GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
	{
		return HitResult.ImpactPoint.Z;
	}

	FBox Bounds = Landscape->GetComponentsBoundingBox(true);
	return (Bounds.Min.Z + Bounds.Max.Z) * 0.5f;
}

/**
 * @brief Internal helper to clamp and round a value to the nearest power of two.
 */
int32 FKawaiiFluidLandscapeHeightmapExtractor::ClampToPowerOfTwo(int32 Value, int32 MinValue, int32 MaxValue)
{
	Value = FMath::Clamp(Value, MinValue, MaxValue);

	Value--;
	Value |= Value >> 1;
	Value |= Value >> 2;
	Value |= Value >> 4;
	Value |= Value >> 8;
	Value |= Value >> 16;
	Value++;

	return FMath::Clamp(Value, MinValue, MaxValue);
}
