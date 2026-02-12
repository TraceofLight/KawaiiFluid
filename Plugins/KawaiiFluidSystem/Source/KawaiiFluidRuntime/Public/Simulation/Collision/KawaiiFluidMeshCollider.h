// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Simulation/Collision/KawaiiFluidCollider.h"
#include "KawaiiFluidMeshCollider.generated.h"

/**
 * @brief Cached capsule collision data.
 * @param Start World space start point
 * @param End World space end point
 * @param Radius Capsule radius
 * @param BoneName Name of the associated bone
 * @param BoneTransform World transform of the bone
 * @param BoneIndex Index for GPU bone transform buffer
 */
struct FCachedCapsule
{
	FVector Start;
	FVector End;
	float Radius;
	FName BoneName;
	FTransform BoneTransform;
	int32 BoneIndex = -1;
};

/**
 * @brief Cached sphere collision data.
 * @param Center World space center
 * @param Radius Sphere radius
 * @param BoneName Name of the associated bone
 * @param BoneTransform World transform of the bone
 * @param BoneIndex Index for GPU bone transform buffer
 */
struct FCachedSphere
{
	FVector Center;
	float Radius;
	FName BoneName;
	FTransform BoneTransform;
	int32 BoneIndex = -1;
};

/**
 * @brief Cached box collision data.
 * @param Center World space center
 * @param Extent Half extents (X, Y, Z)
 * @param Rotation World rotation
 * @param BoneName Name of the associated bone
 * @param BoneTransform World transform of the bone
 * @param BoneIndex Index for GPU bone transform buffer
 */
struct FCachedBox
{
	FVector Center;
	FVector Extent;
	FQuat Rotation;
	FName BoneName;
	FTransform BoneTransform;
	int32 BoneIndex = -1;
};

/**
 * @brief Convex plane data.
 * @param Normal Outward-facing unit normal
 * @param Distance Signed distance from origin
 */
struct FCachedConvexPlane
{
	FVector Normal;
	float Distance;
};

/**
 * @brief Cached convex hull collision data.
 * @param Center Bounding sphere center
 * @param BoundingRadius Bounding sphere radius
 * @param Planes Array of planes defining the convex hull
 * @param BoneName Name of the associated bone
 * @param BoneTransform World transform of the bone
 * @param BoneIndex Index for GPU bone transform buffer
 */
struct FCachedConvex
{
	FVector Center;
	float BoundingRadius;
	TArray<FCachedConvexPlane> Planes;
	FName BoneName;
	FTransform BoneTransform;
	int32 BoneIndex = -1;
};

/**
 * @brief Mesh-based fluid collider.
 * Handles collision with characters or complex objects using simplified collision shapes.
 * @param TargetMeshComponent The mesh component to extract collision from
 * @param bAutoFindMesh Whether to automatically find a mesh on the owner
 * @param bUseSimplifiedCollision Whether to use simplified shapes (spheres, capsules, boxes)
 * @param CollisionMargin Safety margin added to extracted collision shapes
 */
UCLASS(ClassGroup=(KawaiiFluid), meta=(BlueprintSpawnableComponent))
class KAWAIIFLUIDRUNTIME_API UKawaiiFluidMeshCollider : public UKawaiiFluidCollider
{
	GENERATED_BODY()

public:
	UKawaiiFluidMeshCollider();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Collider|Mesh")
	TObjectPtr<UPrimitiveComponent> TargetMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Collider|Mesh")
	bool bAutoFindMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Collider|Mesh")
	bool bUseSimplifiedCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Collider|Mesh")
	float CollisionMargin;

	virtual bool GetClosestPoint(const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal, float& OutDistance) const override;

	virtual bool GetClosestPointWithBone(const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal, float& OutDistance, FName& OutBoneName, FTransform& OutBoneTransform) const override;

	virtual bool IsPointInside(const FVector& Point) const override;

	virtual void CacheCollisionShapes() override;

	virtual FBox GetCachedBounds() const override { return CachedBounds; }

	virtual bool IsCacheValid() const override { return bCacheValid; }

	void ExportToGPUPrimitives(
		TArray<struct FGPUCollisionSphere>& OutSpheres,
		TArray<struct FGPUCollisionCapsule>& OutCapsules,
		TArray<struct FGPUCollisionBox>& OutBoxes,
		TArray<struct FGPUCollisionConvex>& OutConvexes,
		TArray<struct FGPUConvexPlane>& OutPlanes,
		float Friction = 0.1f,
		float Restitution = 0.3f,
		int32 OwnerID = 0
	) const;

	void ExportToGPUPrimitivesWithBones(
		TArray<struct FGPUCollisionSphere>& OutSpheres,
		TArray<struct FGPUCollisionCapsule>& OutCapsules,
		TArray<struct FGPUCollisionBox>& OutBoxes,
		TArray<struct FGPUCollisionConvex>& OutConvexes,
		TArray<struct FGPUConvexPlane>& OutPlanes,
		TArray<struct FGPUBoneTransform>& OutBoneTransforms,
		TMap<FName, int32>& BoneNameToIndex,
		float Friction = 0.1f,
		float Restitution = 0.3f,
		int32 OwnerID = 0
	) const;

protected:
	virtual void BeginPlay() override;

private:
	void AutoFindMeshComponent();

	TArray<FCachedCapsule> CachedCapsules;
	TArray<FCachedSphere> CachedSpheres;
	TArray<FCachedBox> CachedBoxes;
	TArray<FCachedConvex> CachedConvexes;
	FBox CachedBounds;
	bool bCacheValid;

	void CacheStaticMeshCollision(UStaticMeshComponent* StaticMesh);

	void CacheSkeletalMeshCollision(USkeletalMeshComponent* SkelMesh);
};