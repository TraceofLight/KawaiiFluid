// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "Simulation/Collision/KawaiiFluidSphereCollider.h"
#include "GameFramework/Actor.h"

/**
 * @brief Default constructor for UKawaiiFluidSphereCollider.
 */
UKawaiiFluidSphereCollider::UKawaiiFluidSphereCollider()
{
	Radius = 50.0f;
	LocalOffset = FVector::ZeroVector;
}

/**
 * @brief Finds the closest point on the sphere surface.
 * @param Point Query point in world space
 * @param OutClosestPoint Closest point on the sphere surface
 * @param OutNormal Surface normal at the closest point
 * @param OutDistance Distance to the closest point
 * @return True if successful
 */
bool UKawaiiFluidSphereCollider::GetClosestPoint(const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal, float& OutDistance) const
{
	FVector Center = GetSphereCenter();
	FVector ToPoint = Point - Center;
	float DistanceToCenter = ToPoint.Size();

	if (DistanceToCenter < KINDA_SMALL_NUMBER)
	{
		OutNormal = FVector::UpVector;
		OutClosestPoint = Center + OutNormal * Radius;
		OutDistance = Radius;
		return true;
	}

	OutNormal = ToPoint / DistanceToCenter;
	OutClosestPoint = Center + OutNormal * Radius;
	OutDistance = DistanceToCenter - Radius;

	return true;
}

/**
 * @brief Checks if a point is inside the sphere.
 * @param Point Point to check in world space
 * @return True if the point is inside
 */
bool UKawaiiFluidSphereCollider::IsPointInside(const FVector& Point) const
{
	FVector Center = GetSphereCenter();
	float DistanceSq = FVector::DistSquared(Point, Center);

	return DistanceSq <= Radius * Radius;
}

/**
 * @brief Calculates the signed distance to the sphere surface.
 * @param Point Query point in world space
 * @param OutGradient Surface normal at the closest point
 * @return Signed distance (negative if inside)
 */
float UKawaiiFluidSphereCollider::GetSignedDistance(const FVector& Point, FVector& OutGradient) const
{
	FVector Center = GetSphereCenter();
	FVector ToPoint = Point - Center;
	float DistanceToCenter = ToPoint.Size();

	if (DistanceToCenter < KINDA_SMALL_NUMBER)
	{
		// At center: gradient points up, distance is -Radius (deepest inside)
		OutGradient = FVector::UpVector;
		return -Radius;
	}

	// Gradient always points outward from center
	OutGradient = ToPoint / DistanceToCenter;

	// Signed distance: positive outside, negative inside
	return DistanceToCenter - Radius;
}

/**
 * @brief Returns the world space center of the sphere.
 * @return World space position
 */
FVector UKawaiiFluidSphereCollider::GetSphereCenter() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return LocalOffset;
	}

	return Owner->GetActorLocation() + Owner->GetActorRotation().RotateVector(LocalOffset);
}