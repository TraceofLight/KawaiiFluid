// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Simulation/Collision/KawaiiFluidCollider.h"
#include "KawaiiFluidSphereCollider.generated.h"

/**
 * @brief Sphere-shaped fluid collider.
 * @param Radius Radius of the sphere in world units
 * @param LocalOffset Offset from the owner actor location
 */
UCLASS(ClassGroup=(KawaiiFluid), meta=(BlueprintSpawnableComponent))
class KAWAIIFLUIDRUNTIME_API UKawaiiFluidSphereCollider : public UKawaiiFluidCollider
{
	GENERATED_BODY()

public:
	UKawaiiFluidSphereCollider();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Collider|Sphere")
	float Radius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid Collider|Sphere")
	FVector LocalOffset;

	virtual bool GetClosestPoint(const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal, float& OutDistance) const override;

	virtual bool IsPointInside(const FVector& Point) const override;

	virtual float GetSignedDistance(const FVector& Point, FVector& OutGradient) const override;

private:
	FVector GetSphereCenter() const;
};