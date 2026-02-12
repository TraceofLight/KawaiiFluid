// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "Simulation/Collision/KawaiiFluidSkeletalMeshBVH.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "Algo/Sort.h"
#include "Engine/SkeletalMesh.h"
#include "Async/ParallelFor.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSkeletalMeshBVH, Log, All);
DEFINE_LOG_CATEGORY(LogSkeletalMeshBVH);

/**
 * @brief Default constructor for FKawaiiFluidSkeletalMeshBVH.
 */
FKawaiiFluidSkeletalMeshBVH::FKawaiiFluidSkeletalMeshBVH()
	: LODIndex(0)
	, VertexCount(0)
	, bIsInitialized(false)
{
}

FKawaiiFluidSkeletalMeshBVH::~FKawaiiFluidSkeletalMeshBVH()
{
	Clear();
}

/**
 * @brief Clears all BVH and mesh data.
 */
void FKawaiiFluidSkeletalMeshBVH::Clear()
{
	Nodes.Empty();
	SkinnedTriangles.Empty();
	TriangleIndicesSorted.Empty();
	IndexBuffer.Empty();
	SkelMeshComponent.Reset();
	bIsInitialized = false;
	VertexCount = 0;
}

/**
 * @brief Initializes the BVH from a skeletal mesh component.
 * @param InSkelMesh Target skeletal mesh component
 * @param InLODIndex LOD level to use (default 0)
 * @return True if initialization succeeded
 */
bool FKawaiiFluidSkeletalMeshBVH::Initialize(USkeletalMeshComponent* InSkelMesh, int32 InLODIndex)
{
	Clear();

	if (!InSkelMesh)
	{
		UE_LOG(LogSkeletalMeshBVH, Warning, TEXT("Initialize failed: SkelMesh is null"));
		return false;
	}

	USkeletalMesh* SkelMeshAsset = InSkelMesh->GetSkeletalMeshAsset();
	if (!SkelMeshAsset)
	{
		UE_LOG(LogSkeletalMeshBVH, Warning, TEXT("Initialize failed: SkeletalMeshAsset is null"));
		return false;
	}

	SkelMeshComponent = InSkelMesh;
	LODIndex = FMath::Clamp(InLODIndex, 0, SkelMeshAsset->GetLODNum() - 1);

	if (!ExtractTrianglesFromMesh())
	{
		UE_LOG(LogSkeletalMeshBVH, Warning, TEXT("Initialize failed: Could not extract triangles"));
		Clear();
		return false;
	}

	if (SkinnedTriangles.Num() == 0)
	{
		UE_LOG(LogSkeletalMeshBVH, Warning, TEXT("Initialize failed: No triangles extracted"));
		Clear();
		return false;
	}

	UpdateSkinnedPositions();

	TriangleIndicesSorted.SetNum(SkinnedTriangles.Num());
	for (int32 i = 0; i < SkinnedTriangles.Num(); ++i)
	{
		TriangleIndicesSorted[i] = i;
	}

	Nodes.Reserve(SkinnedTriangles.Num() * 2);
	BuildBVH(TriangleIndicesSorted, 0, TriangleIndicesSorted.Num());

	bIsInitialized = true;

	UE_LOG(LogSkeletalMeshBVH, Log, TEXT("BVH initialized: %d triangles, %d nodes"),
		SkinnedTriangles.Num(), Nodes.Num());

	return true;
}

/**
 * @brief Extracts triangle indices from the skeletal mesh render data.
 * @return True if successful
 */
bool FKawaiiFluidSkeletalMeshBVH::ExtractTrianglesFromMesh()
{
	USkeletalMeshComponent* SkelMesh = SkelMeshComponent.Get();
	if (!SkelMesh)
	{
		return false;
	}

	USkeletalMesh* MeshAsset = SkelMesh->GetSkeletalMeshAsset();
	if (!MeshAsset)
	{
		return false;
	}

	FSkeletalMeshRenderData* RenderData = MeshAsset->GetResourceForRendering();
	if (!RenderData || !RenderData->LODRenderData.IsValidIndex(LODIndex))
	{
		return false;
	}

	const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[LODIndex];
	VertexCount = LODData.GetNumVertices();

	const FRawStaticIndexBuffer16or32Interface* IndexBufferInterface = LODData.MultiSizeIndexContainer.GetIndexBuffer();
	if (!IndexBufferInterface)
	{
		return false;
	}

	const int32 NumIndices = IndexBufferInterface->Num();
	if (NumIndices < 3)
	{
		return false;
	}

	IndexBuffer.SetNum(NumIndices);
	for (int32 i = 0; i < NumIndices; ++i)
	{
		IndexBuffer[i] = IndexBufferInterface->Get(i);
	}

	const int32 NumTriangles = NumIndices / 3;
	SkinnedTriangles.SetNum(NumTriangles);

	for (int32 TriIdx = 0; TriIdx < NumTriangles; ++TriIdx)
	{
		FSkinnedTriangle& Tri = SkinnedTriangles[TriIdx];
		Tri.TriangleIndex = TriIdx;
		Tri.SectionIndex = 0;
	}

	return true;
}

/**
 * @brief Updates vertex positions by applying skinning and recalculates the BVH bounds.
 */
void FKawaiiFluidSkeletalMeshBVH::UpdateSkinnedPositions()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SkeletalMeshBVH_UpdateSkinnedPositions);

	USkeletalMeshComponent* SkelMesh = SkelMeshComponent.Get();
	if (!SkelMesh || SkinnedTriangles.Num() == 0)
	{
		return;
	}

	USkeletalMesh* MeshAsset = SkelMesh->GetSkeletalMeshAsset();
	if (!MeshAsset)
	{
		return;
	}

	FSkeletalMeshRenderData* RenderData = MeshAsset->GetResourceForRendering();
	if (!RenderData || !RenderData->LODRenderData.IsValidIndex(LODIndex))
	{
		return;
	}

	const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[LODIndex];
	FSkinWeightVertexBuffer& SkinWeightBuffer = *const_cast<FSkinWeightVertexBuffer*>(&LODData.SkinWeightVertexBuffer);

	const FTransform ComponentTransform = SkelMesh->GetComponentTransform();
	const int32 NumTriangles = SkinnedTriangles.Num();

	ParallelFor(NumTriangles, [this, SkelMesh, &LODData, &SkinWeightBuffer, &ComponentTransform](int32 TriIdx)
	{
		FSkinnedTriangle& Tri = SkinnedTriangles[TriIdx];
		const int32 BaseIndex = TriIdx * 3;
		const uint32 Idx0 = IndexBuffer[BaseIndex + 0];
		const uint32 Idx1 = IndexBuffer[BaseIndex + 1];
		const uint32 Idx2 = IndexBuffer[BaseIndex + 2];

		FVector LocalV0 = FVector(USkinnedMeshComponent::GetSkinnedVertexPosition(SkelMesh, Idx0, LODData, SkinWeightBuffer));
		FVector LocalV1 = FVector(USkinnedMeshComponent::GetSkinnedVertexPosition(SkelMesh, Idx1, LODData, SkinWeightBuffer));
		FVector LocalV2 = FVector(USkinnedMeshComponent::GetSkinnedVertexPosition(SkelMesh, Idx2, LODData, SkinWeightBuffer));

		Tri.V0 = ComponentTransform.TransformPosition(LocalV0);
		Tri.V1 = ComponentTransform.TransformPosition(LocalV1);
		Tri.V2 = ComponentTransform.TransformPosition(LocalV2);
		Tri.ComputeDerivedData();
	});

	if (Nodes.Num() > 0)
	{
		for (int32 i = Nodes.Num() - 1; i >= 0; --i)
		{
			UpdateNodeBounds(i);
		}
	}
}

/**
 * @brief Updates the AABB bounds of a specific node by aggregating child or triangle bounds.
 * @param NodeIndex Index of the node to update
 */
void FKawaiiFluidSkeletalMeshBVH::UpdateNodeBounds(int32 NodeIndex)
{
	if (!Nodes.IsValidIndex(NodeIndex))
	{
		return;
	}

	FBVHNode& Node = Nodes[NodeIndex];

	if (Node.IsLeaf())
	{
		Node.Bounds = FBox(ForceInit);
		const int32 End = Node.TriangleStartIndex + Node.TriangleCount;
		for (int32 i = Node.TriangleStartIndex; i < End; ++i)
		{
			if (TriangleIndicesSorted.IsValidIndex(i))
			{
				const FSkinnedTriangle& Tri = SkinnedTriangles[TriangleIndicesSorted[i]];
				Node.Bounds += Tri.V0;
				Node.Bounds += Tri.V1;
				Node.Bounds += Tri.V2;
			}
		}
	}
	else
	{
		Node.Bounds = FBox(ForceInit);
		if (Nodes.IsValidIndex(Node.LeftChild))
		{
			Node.Bounds += Nodes[Node.LeftChild].Bounds;
		}
		if (Nodes.IsValidIndex(Node.RightChild))
		{
			Node.Bounds += Nodes[Node.RightChild].Bounds;
		}
	}
}

/**
 * @brief Recursively builds the BVH tree using median splitting on the longest axis.
 * @param TriangleIndices Array of triangle indices to split
 * @param Start Start index in the array
 * @param End End index in the array
 * @return Index of the created node
 */
int32 FKawaiiFluidSkeletalMeshBVH::BuildBVH(TArray<int32>& TriangleIndices, int32 Start, int32 End)
{
	const int32 Count = End - Start;
	const int32 NodeIndex = Nodes.Num();
	Nodes.AddDefaulted();

	if (Count <= 0)
	{
		return NodeIndex;
	}

	FBox Bounds(ForceInit);
	for (int32 i = Start; i < End; ++i)
	{
		const FSkinnedTriangle& Tri = SkinnedTriangles[TriangleIndices[i]];
		Bounds += Tri.V0;
		Bounds += Tri.V1;
		Bounds += Tri.V2;
	}
	Nodes[NodeIndex].Bounds = Bounds;

	if (Count <= LeafTriangleThreshold)
	{
		Nodes[NodeIndex].LeftChild = INDEX_NONE;
		Nodes[NodeIndex].RightChild = INDEX_NONE;
		Nodes[NodeIndex].TriangleStartIndex = Start;
		Nodes[NodeIndex].TriangleCount = Count;
		return NodeIndex;
	}

	const FVector Extent = Bounds.GetExtent();
	int32 SplitAxis = 0;
	if (Extent.Y > Extent.X) SplitAxis = 1;
	if (Extent.Z > Extent[SplitAxis]) SplitAxis = 2;

	Algo::Sort(MakeArrayView(&TriangleIndices[Start], Count), [this, SplitAxis](int32 A, int32 B)
	{
		return SkinnedTriangles[A].Centroid[SplitAxis] < SkinnedTriangles[B].Centroid[SplitAxis];
	});

	const int32 Mid = Start + Count / 2;
	Nodes[NodeIndex].LeftChild = BuildBVH(TriangleIndices, Start, Mid);
	Nodes[NodeIndex].RightChild = BuildBVH(TriangleIndices, Mid, End);
	Nodes[NodeIndex].TriangleStartIndex = INDEX_NONE;
	Nodes[NodeIndex].TriangleCount = 0;

	return NodeIndex;
}

/**
 * @brief Queries the closest triangle to a given point within a search distance.
 * @param Point Query point in world space
 * @param MaxDistance Maximum distance to search
 * @param OutResult Output result structure
 * @return True if a triangle was found
 */
bool FKawaiiFluidSkeletalMeshBVH::QueryClosestTriangle(const FVector& Point, float MaxDistance, FTriangleQueryResult& OutResult) const
{
	OutResult = FTriangleQueryResult();

	if (!IsValid())
	{
		return false;
	}

	float BestDistSq = MaxDistance * MaxDistance;
	int32 BestTriangle = INDEX_NONE;

	QueryClosestRecursive(0, Point, BestDistSq, BestTriangle);

	if (BestTriangle != INDEX_NONE)
	{
		const FSkinnedTriangle& Tri = SkinnedTriangles[TriangleIndicesSorted[BestTriangle]];
		OutResult.ClosestPoint = ClosestPointOnTriangle(Point, Tri.V0, Tri.V1, Tri.V2);
		OutResult.Normal = Tri.Normal;
		OutResult.Distance = FMath::Sqrt(BestDistSq);
		OutResult.TriangleIndex = BestTriangle;
		OutResult.bValid = true;
		return true;
	}

	return false;
}

/**
 * @brief Recursive helper for QueryClosestTriangle.
 * @param NodeIndex Current node being visited
 * @param Point Query point
 * @param BestDistSq Reference to current best squared distance
 * @param BestTriangle Reference to current best triangle index
 */
void FKawaiiFluidSkeletalMeshBVH::QueryClosestRecursive(int32 NodeIndex, const FVector& Point, float& BestDistSq, int32& BestTriangle) const
{
	if (!Nodes.IsValidIndex(NodeIndex))
	{
		return;
	}

	const FBVHNode& Node = Nodes[NodeIndex];
	const float NodeDistSq = Node.Bounds.ComputeSquaredDistanceToPoint(Point);
	if (NodeDistSq > BestDistSq)
	{
		return;
	}

	if (Node.IsLeaf())
	{
		const int32 End = Node.TriangleStartIndex + Node.TriangleCount;
		for (int32 i = Node.TriangleStartIndex; i < End; ++i)
		{
			if (!TriangleIndicesSorted.IsValidIndex(i)) continue;
			const FSkinnedTriangle& Tri = SkinnedTriangles[TriangleIndicesSorted[i]];
			const FVector ClosestPt = ClosestPointOnTriangle(Point, Tri.V0, Tri.V1, Tri.V2);
			const float DistSq = FVector::DistSquared(Point, ClosestPt);

			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestTriangle = i;
			}
		}
	}
	else
	{
		const float LeftDistSq = Nodes.IsValidIndex(Node.LeftChild) ? Nodes[Node.LeftChild].Bounds.ComputeSquaredDistanceToPoint(Point) : FLT_MAX;
		const float RightDistSq = Nodes.IsValidIndex(Node.RightChild) ? Nodes[Node.RightChild].Bounds.ComputeSquaredDistanceToPoint(Point) : FLT_MAX;

		if (LeftDistSq < RightDistSq)
		{
			QueryClosestRecursive(Node.LeftChild, Point, BestDistSq, BestTriangle);
			QueryClosestRecursive(Node.RightChild, Point, BestDistSq, BestTriangle);
		}
		else
		{
			QueryClosestRecursive(Node.RightChild, Point, BestDistSq, BestTriangle);
			QueryClosestRecursive(Node.LeftChild, Point, BestDistSq, BestTriangle);
		}
	}
}

/**
 * @brief Queries all triangles that might intersect a given sphere.
 * @param Center Sphere center
 * @param Radius Sphere radius
 * @param OutTriangleIndices Output array for overlapping triangle indices
 */
void FKawaiiFluidSkeletalMeshBVH::QuerySphere(const FVector& Center, float Radius, TArray<int32>& OutTriangleIndices) const
{
	OutTriangleIndices.Reset();
	if (!IsValid()) return;
	const float RadiusSq = Radius * Radius;
	QuerySphereRecursive(0, Center, RadiusSq, OutTriangleIndices);
}

/**
 * @brief Recursive helper for QuerySphere.
 */
void FKawaiiFluidSkeletalMeshBVH::QuerySphereRecursive(int32 NodeIndex, const FVector& Center, float RadiusSq, TArray<int32>& OutTriangleIndices) const
{
	if (!Nodes.IsValidIndex(NodeIndex)) return;
	const FBVHNode& Node = Nodes[NodeIndex];
	if (Node.Bounds.ComputeSquaredDistanceToPoint(Center) > RadiusSq) return;

	if (Node.IsLeaf())
	{
		const int32 End = Node.TriangleStartIndex + Node.TriangleCount;
		for (int32 i = Node.TriangleStartIndex; i < End; ++i)
		{
			if (TriangleIndicesSorted.IsValidIndex(i)) OutTriangleIndices.Add(TriangleIndicesSorted[i]);
		}
	}
	else
	{
		QuerySphereRecursive(Node.LeftChild, Center, RadiusSq, OutTriangleIndices);
		QuerySphereRecursive(Node.RightChild, Center, RadiusSq, OutTriangleIndices);
	}
}

/**
 * @brief Queries all triangles that might intersect a given AABB.
 * @param AABB Query bounding box
 * @param OutTriangleIndices Output array for overlapping triangle indices
 */
void FKawaiiFluidSkeletalMeshBVH::QueryAABB(const FBox& AABB, TArray<int32>& OutTriangleIndices) const
{
	OutTriangleIndices.Reset();
	if (!IsValid()) return;
	QueryAABBRecursive(0, AABB, OutTriangleIndices);
}

/**
 * @brief Recursive helper for QueryAABB.
 */
void FKawaiiFluidSkeletalMeshBVH::QueryAABBRecursive(int32 NodeIndex, const FBox& AABB, TArray<int32>& OutTriangleIndices) const
{
	if (!Nodes.IsValidIndex(NodeIndex)) return;
	const FBVHNode& Node = Nodes[NodeIndex];
	if (!Node.Bounds.Intersect(AABB)) return;

	if (Node.IsLeaf())
	{
		const int32 End = Node.TriangleStartIndex + Node.TriangleCount;
		for (int32 i = Node.TriangleStartIndex; i < End; ++i)
		{
			if (TriangleIndicesSorted.IsValidIndex(i)) OutTriangleIndices.Add(TriangleIndicesSorted[i]);
		}
	}
	else
	{
		QueryAABBRecursive(Node.LeftChild, AABB, OutTriangleIndices);
		QueryAABBRecursive(Node.RightChild, AABB, OutTriangleIndices);
	}
}

/**
 * @brief Computes the closest point on a triangle surface to a query point.
 * @param Point Query point
 * @param V0 Triangle vertex 0
 * @param V1 Triangle vertex 1
 * @param V2 Triangle vertex 2
 * @return Closest point on the triangle
 */
FVector FKawaiiFluidSkeletalMeshBVH::ClosestPointOnTriangle(const FVector& Point, const FVector& V0, const FVector& V1, const FVector& V2)
{
	const FVector Edge0 = V1 - V0, Edge1 = V2 - V0, V0ToPoint = V0 - Point;
	const float A = FVector::DotProduct(Edge0, Edge0), B = FVector::DotProduct(Edge0, Edge1), C = FVector::DotProduct(Edge1, Edge1), D = FVector::DotProduct(Edge0, V0ToPoint), E = FVector::DotProduct(Edge1, V0ToPoint);
	const float Det = A * C - B * B;
	float S = B * E - C * D, T = B * D - A * E;

	if (S + T <= Det)
	{
		if (S < 0.0f)
		{
			if (T < 0.0f) { if (D < 0.0f) { S = FMath::Clamp(-D / A, 0.0f, 1.0f); T = 0.0f; } else { S = 0.0f; T = FMath::Clamp(-E / C, 0.0f, 1.0f); } }
			else { S = 0.0f; T = FMath::Clamp(-E / C, 0.0f, 1.0f); }
		}
		else if (T < 0.0f) { S = FMath::Clamp(-D / A, 0.0f, 1.0f); T = 0.0f; }
		else { const float InvDet = 1.0f / Det; S *= InvDet; T *= InvDet; }
	}
	else
	{
		if (S < 0.0f) { const float Tmp0 = B + D, Tmp1 = C + E; if (Tmp1 > Tmp0) { const float Numer = Tmp1 - Tmp0, Denom = A - 2.0f * B + C; S = FMath::Clamp(Numer / Denom, 0.0f, 1.0f); T = 1.0f - S; } else { S = 0.0f; T = FMath::Clamp(-E / C, 0.0f, 1.0f); } }
		else if (T < 0.0f) { const float Tmp0 = B + E, Tmp1 = A + D; if (Tmp1 > Tmp0) { const float Numer = Tmp1 - Tmp0, Denom = A - 2.0f * B + C; T = FMath::Clamp(Numer / Denom, 0.0f, 1.0f); S = 1.0f - T; } else { T = 0.0f; S = FMath::Clamp(-D / A, 0.0f, 1.0f); } }
		else { const float Numer = (C + E) - (B + D); if (Numer <= 0.0f) S = 0.0f; else { const float Denom = A - 2.0f * B + C; S = FMath::Clamp(Numer / Denom, 0.0f, 1.0f); } T = 1.0f - S; }
	}
	return V0 + S * Edge0 + T * Edge1;
}

/**
 * @brief Retrieves the skinned world space position of a specific vertex.
 * @param VertexIndex Index of the vertex
 * @param OutPosition Output position
 * @return True if successful
 */
bool FKawaiiFluidSkeletalMeshBVH::GetSkinnedVertexPosition(int32 VertexIndex, FVector& OutPosition) const
{
	USkeletalMeshComponent* SkelMesh = SkelMeshComponent.Get();
	if (!SkelMesh || VertexIndex < 0 || VertexIndex >= VertexCount) return false;
	USkeletalMesh* MeshAsset = SkelMesh->GetSkeletalMeshAsset();
	if (!MeshAsset) return false;
	FSkeletalMeshRenderData* RenderData = MeshAsset->GetResourceForRendering();
	if (!RenderData || !RenderData->LODRenderData.IsValidIndex(LODIndex)) return false;
	const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[LODIndex];
	FSkinWeightVertexBuffer& SkinWeightBuffer = *const_cast<FSkinWeightVertexBuffer*>(&LODData.SkinWeightVertexBuffer);
	FVector LocalPos = FVector(USkinnedMeshComponent::GetSkinnedVertexPosition(SkelMesh, VertexIndex, LODData, SkinWeightBuffer));
	OutPosition = SkelMesh->GetComponentTransform().TransformPosition(LocalPos);
	return true;
}