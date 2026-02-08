// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"

/**
 * @brief Skinned Triangle Data.
 * Represents a single triangle from the skeletal mesh with skinned vertex positions.
 * @param V0 Skinned vertex 0 (world space)
 * @param V1 Skinned vertex 1 (world space)
 * @param V2 Skinned vertex 2 (world space)
 * @param Normal Triangle normal (computed from vertices)
 * @param Centroid Triangle center for BVH sorting
 * @param TriangleIndex Original triangle index in mesh
 * @param SectionIndex LOD section index
 */
struct FSkinnedTriangle
{
	FVector V0;
	FVector V1;
	FVector V2;
	FVector Normal;
	FVector Centroid;
	int32 TriangleIndex;
	int32 SectionIndex;

	FSkinnedTriangle()
		: V0(FVector::ZeroVector)
		, V1(FVector::ZeroVector)
		, V2(FVector::ZeroVector)
		, Normal(FVector::UpVector)
		, Centroid(FVector::ZeroVector)
		, TriangleIndex(INDEX_NONE)
		, SectionIndex(0)
	{
	}

	void ComputeDerivedData()
	{
		Centroid = (V0 + V1 + V2) / 3.0f;
		// UE5 skeletal mesh uses CW winding → Edge2 x Edge1 for outward normal
		FVector Edge1 = V1 - V0;
		FVector Edge2 = V2 - V0;
		Normal = FVector::CrossProduct(Edge2, Edge1).GetSafeNormal();
	}

	FBox GetBounds() const
	{
		FBox Bounds(ForceInit);
		Bounds += V0;
		Bounds += V1;
		Bounds += V2;
		return Bounds;
	}
};

/**
 * @brief BVH Node.
 * Binary tree node for spatial partitioning.
 * @param Bounds AABB bounding box
 * @param LeftChild Left child index (-1 = leaf)
 * @param RightChild Right child index (-1 = leaf)
 * @param TriangleStartIndex For leaf: start index in sorted triangle array
 * @param TriangleCount For leaf: number of triangles
 */
struct FBVHNode
{
	FBox Bounds;
	int32 LeftChild;
	int32 RightChild;
	int32 TriangleStartIndex;
	int32 TriangleCount;

	FBVHNode()
		: Bounds(ForceInit)
		, LeftChild(INDEX_NONE)
		, RightChild(INDEX_NONE)
		, TriangleStartIndex(INDEX_NONE)
		, TriangleCount(0)
	{
	}

	bool IsLeaf() const { return LeftChild == INDEX_NONE && RightChild == INDEX_NONE; }
};

/**
 * @brief Triangle Query Result.
 * Result of a closest point query.
 * @param ClosestPoint Closest point on triangle surface
 * @param Normal Triangle normal at closest point
 * @param Distance Distance from query point to closest point
 * @param TriangleIndex Index of the triangle
 * @param bValid Whether result is valid
 */
struct FTriangleQueryResult
{
	FVector ClosestPoint;
	FVector Normal;
	float Distance;
	int32 TriangleIndex;
	bool bValid;

	FTriangleQueryResult()
		: ClosestPoint(FVector::ZeroVector)
		, Normal(FVector::UpVector)
		, Distance(FLT_MAX)
		, TriangleIndex(INDEX_NONE)
		, bValid(false)
	{
	}
};

/**
 * @brief Skeletal Mesh BVH.
 * Bounding Volume Hierarchy for efficient triangle queries on skinned meshes.
 * @param SkelMeshComponent Weak pointer to target skeletal mesh component
 * @param Nodes Array of BVH nodes
 * @param SkinnedTriangles Array of skinned triangle data
 * @param TriangleIndicesSorted Triangle indices in BVH leaf order
 */
class KAWAIIFLUIDRUNTIME_API FKawaiiFluidSkeletalMeshBVH
{
public:
	FKawaiiFluidSkeletalMeshBVH();
	~FKawaiiFluidSkeletalMeshBVH();

	bool Initialize(USkeletalMeshComponent* InSkelMesh, int32 InLODIndex = 0);

	void UpdateSkinnedPositions();

	bool QueryClosestTriangle(const FVector& Point, float MaxDistance, FTriangleQueryResult& OutResult) const;

	void QuerySphere(const FVector& Center, float Radius, TArray<int32>& OutTriangleIndices) const;

	void QueryAABB(const FBox& AABB, TArray<int32>& OutTriangleIndices) const;

	bool IsValid() const { return bIsInitialized && Nodes.Num() > 0; }

	int32 GetTriangleCount() const { return SkinnedTriangles.Num(); }

	int32 GetNodeCount() const { return Nodes.Num(); }

	const TArray<FSkinnedTriangle>& GetTriangles() const { return SkinnedTriangles; }

	const FSkinnedTriangle& GetTriangle(int32 Index) const { return SkinnedTriangles[TriangleIndicesSorted[Index]]; }

	FBox GetRootBounds() const { return Nodes.Num() > 0 ? Nodes[0].Bounds : FBox(ForceInit); }

	USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkelMeshComponent.Get(); }

	void Clear();

	static FVector ClosestPointOnTriangle(const FVector& Point, const FVector& V0, const FVector& V1, const FVector& V2);

private:
	int32 BuildBVH(TArray<int32>& TriangleIndices, int32 Start, int32 End);

	void UpdateNodeBounds(int32 NodeIndex);

	void QuerySphereRecursive(int32 NodeIndex, const FVector& Center, float RadiusSq, TArray<int32>& OutTriangleIndices) const;

	void QueryAABBRecursive(int32 NodeIndex, const FBox& AABB, TArray<int32>& OutTriangleIndices) const;

	void QueryClosestRecursive(int32 NodeIndex, const FVector& Point, float& BestDistSq, int32& BestTriangle) const;

	bool ExtractTrianglesFromMesh();

	bool GetSkinnedVertexPosition(int32 VertexIndex, FVector& OutPosition) const;

private:
	TWeakObjectPtr<USkeletalMeshComponent> SkelMeshComponent;

	TArray<FBVHNode> Nodes;
	TArray<FSkinnedTriangle> SkinnedTriangles;
	TArray<int32> TriangleIndicesSorted;

	TArray<uint32> IndexBuffer;
	int32 LODIndex;
	int32 VertexCount;

	bool bIsInitialized;

	static constexpr int32 LeafTriangleThreshold = 4;
	static constexpr int32 MaxTreeDepth = 32;
};