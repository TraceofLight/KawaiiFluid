// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Collision/KawaiiFluidCollider.h"
#include "KawaiiFluidBoxCollider.generated.h"

/**
 * @brief Box-shaped fluid collider.
 * @param BoxExtent Half-size of the box in local space
 * @param LocalOffset Center offset of the box relative to actor location
 */
UCLASS(ClassGroup=(KawaiiFluid), meta=(BlueprintSpawnableComponent))
class KAWAIIFLUIDRUNTIME_API UKawaiiFluidBoxCollider : public UKawaiiFluidCollider
{
	GENERATED_BODY()

public:
	UKawaiiFluidBoxCollider();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Collider|Box")
	FVector BoxExtent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Collider|Box")
	FVector LocalOffset;

	virtual bool GetClosestPoint(const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal, float& OutDistance) const override;

	virtual bool IsPointInside(const FVector& Point) const override;

	virtual float GetSignedDistance(const FVector& Point, FVector& OutGradient) const override;

private:
	FVector WorldToLocal(const FVector& WorldPoint) const;

	FVector LocalToWorld(const FVector& LocalPoint) const;

	FVector GetBoxCenter() const;
};