// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Simulation/Collision/KawaiiFluidCollider.h"
#include "KawaiiFluidCapsuleCollider.generated.h"

/**
 * @brief Capsule-shaped fluid collider.
 * Defined by two endpoints and a radius.
 * @param HalfHeight Capsule half-height (from center to hemisphere center)
 * @param Radius Capsule radius
 * @param LocalOffset Local offset from owner location
 * @param LocalRotation Capsule orientation (default: Z-up)
 */
UCLASS(ClassGroup=(KawaiiFluid), meta=(BlueprintSpawnableComponent))
class KAWAIIFLUIDRUNTIME_API UKawaiiFluidCapsuleCollider : public UKawaiiFluidCollider
{
	GENERATED_BODY()

public:
	UKawaiiFluidCapsuleCollider();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Collider|Capsule")
	float HalfHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Collider|Capsule")
	float Radius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Collider|Capsule")
	FVector LocalOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Collider|Capsule")
	FRotator LocalRotation;

	virtual bool GetClosestPoint(const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal, float& OutDistance) const override;

	virtual bool IsPointInside(const FVector& Point) const override;

	virtual float GetSignedDistance(const FVector& Point, FVector& OutGradient) const override;

private:
	FVector GetCapsuleCenter() const;

	void GetCapsuleEndpoints(FVector& OutStart, FVector& OutEnd) const;

	FVector WorldToLocal(const FVector& WorldPoint) const;
};