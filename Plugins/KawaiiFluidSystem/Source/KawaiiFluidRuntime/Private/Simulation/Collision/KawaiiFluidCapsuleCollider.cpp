// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "Simulation/Collision/KawaiiFluidCapsuleCollider.h"
#include "GameFramework/Actor.h"

/**
 * @brief Default constructor for UKawaiiFluidCapsuleCollider.
 */
UKawaiiFluidCapsuleCollider::UKawaiiFluidCapsuleCollider()
{
	HalfHeight = 50.0f;
	Radius = 25.0f;
	LocalOffset = FVector::ZeroVector;
	LocalRotation = FRotator::ZeroRotator;
}

/**
 * @brief Finds the closest point on the capsule surface.
 * @param Point Query point in world space
 * @param OutClosestPoint Closest point on the capsule surface
 * @param OutNormal Surface normal at the closest point
 * @param OutDistance Distance to the closest point
 * @return True if successful
 */
bool UKawaiiFluidCapsuleCollider::GetClosestPoint(const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal, float& OutDistance) const
{
	FVector Start, End;
	GetCapsuleEndpoints(Start, End);

	// Find closest point on line segment
	FVector ClosestOnLine = FMath::ClosestPointOnSegment(Point, Start, End);

	FVector ToPoint = Point - ClosestOnLine;
	float DistanceToLine = ToPoint.Size();

	if (DistanceToLine < KINDA_SMALL_NUMBER)
	{
		// Point is on the capsule axis
		OutNormal = FVector::UpVector;
		OutClosestPoint = ClosestOnLine + OutNormal * Radius;
		OutDistance = Radius;
		return true;
	}

	OutNormal = ToPoint / DistanceToLine;
	OutClosestPoint = ClosestOnLine + OutNormal * Radius;
	OutDistance = DistanceToLine - Radius;

	return true;
}

/**
 * @brief Checks if a point is inside the capsule.
 * @param Point Point to check in world space
 * @return True if the point is inside
 */
bool UKawaiiFluidCapsuleCollider::IsPointInside(const FVector& Point) const
{
	FVector Start, End;
	GetCapsuleEndpoints(Start, End);

	FVector ClosestOnLine = FMath::ClosestPointOnSegment(Point, Start, End);
	float DistanceSq = FVector::DistSquared(Point, ClosestOnLine);

	return DistanceSq <= Radius * Radius;
}

/**
 * @brief Calculates the signed distance to the capsule surface.
 * @param Point Query point in world space
 * @param OutGradient Surface normal at the closest point
 * @return Signed distance (negative if inside)
 */
float UKawaiiFluidCapsuleCollider::GetSignedDistance(const FVector& Point, FVector& OutGradient) const
{
	FVector Start, End;
	GetCapsuleEndpoints(Start, End);

	// Capsule SDF: distance to line segment minus radius
	FVector ClosestOnLine = FMath::ClosestPointOnSegment(Point, Start, End);

	FVector ToPoint = Point - ClosestOnLine;
	float DistanceToLine = ToPoint.Size();

	if (DistanceToLine < KINDA_SMALL_NUMBER)
	{
		// Point is on the capsule axis
		// Gradient points in arbitrary perpendicular direction
		FVector CapsuleDir = (End - Start).GetSafeNormal();
		OutGradient = FVector::CrossProduct(CapsuleDir, FVector::UpVector);
		if (OutGradient.SizeSquared() < KINDA_SMALL_NUMBER)
		{
			OutGradient = FVector::CrossProduct(CapsuleDir, FVector::RightVector);
		}
		OutGradient.Normalize();
		return -Radius;  // Deepest inside
	}

	// Gradient points from line to point (outward)
	OutGradient = ToPoint / DistanceToLine;

	// Signed distance: positive outside, negative inside
	return DistanceToLine - Radius;
}

/**
 * @brief Returns the world space center of the capsule.
 * @return World space position
 */
FVector UKawaiiFluidCapsuleCollider::GetCapsuleCenter() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return LocalOffset;
	}

	return Owner->GetActorLocation() + Owner->GetActorRotation().RotateVector(LocalOffset);
}

/**
 * @brief Calculates the world space endpoints of the capsule segment.
 * @param OutStart Output for the start point
 * @param OutEnd Output for the end point
 */
void UKawaiiFluidCapsuleCollider::GetCapsuleEndpoints(FVector& OutStart, FVector& OutEnd) const
{
	AActor* Owner = GetOwner();
	FVector Center = GetCapsuleCenter();

	// Default capsule direction is Z-up
	FVector LocalUp = FVector::UpVector;

	if (Owner)
	{
		// Apply local rotation then owner rotation
		FQuat CombinedQuat = Owner->GetActorQuat() * LocalRotation.Quaternion();
		LocalUp = CombinedQuat.RotateVector(FVector::UpVector);
	}
	else
	{
		LocalUp = LocalRotation.Quaternion().RotateVector(FVector::UpVector);
	}

	OutStart = Center - LocalUp * HalfHeight;
	OutEnd = Center + LocalUp * HalfHeight;
}

/**
 * @brief Transforms a world space point to the capsule's local space.
 * @param WorldPoint Point in world space
 * @return Point in local space
 */
FVector UKawaiiFluidCapsuleCollider::WorldToLocal(const FVector& WorldPoint) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return WorldPoint - LocalOffset;
	}

	FVector Center = GetCapsuleCenter();
	FVector RelativePoint = WorldPoint - Center;

	FQuat CombinedQuat = Owner->GetActorQuat() * LocalRotation.Quaternion();
	return CombinedQuat.UnrotateVector(RelativePoint);
}