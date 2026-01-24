// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "EngineUtils.h"
#include "SceneView.h"

/**
 * @brief Parameters for fluid shadow light.
 * @param LightDirection World space light direction (normalized).
 * @param LightViewProjectionMatrix Combined view-projection matrix for shadow mapping.
 * @param ShadowBounds World space bounds for shadow projection.
 * @param bIsValid Whether the light parameters are valid.
 */
struct FFluidShadowLightParams
{
	/** Light direction in world space (normalized, pointing towards light) */
	FVector3f LightDirection = FVector3f(0, 0, 1);

	/** Combined view-projection matrix for shadow mapping */
	FMatrix44f LightViewProjectionMatrix = FMatrix44f::Identity;

	/** Shadow projection bounds in world space */
	FBox ShadowBounds;

	/** Whether these parameters are valid */
	bool bIsValid = false;
};

/**
 * Utility functions for fluid shadow rendering.
 */
namespace FluidShadowUtils
{
	/**
	 * Find the main directional light in the world.
	 *
	 * @param World The world to search.
	 * @return The first directional light found, or nullptr.
	 */
	inline ADirectionalLight* FindMainDirectionalLight(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<ADirectionalLight> It(World); It; ++It)
		{
			ADirectionalLight* Light = *It;
			if (Light && Light->GetLightComponent() && Light->GetLightComponent()->CastShadows)
			{
				return Light;
			}
		}

		return nullptr;
	}

	/**
	 * Calculate orthographic shadow projection matrix for directional light.
	 *
	 * @param LightDirection Light direction (pointing away from light source).
	 * @param Bounds World space bounds to encompass in shadow.
	 * @param OutViewMatrix Output view matrix.
	 * @param OutProjectionMatrix Output projection matrix.
	 */
	inline void CalculateDirectionalLightMatrices(
		const FVector& LightDirection,
		const FBox& Bounds,
		FMatrix& OutViewMatrix,
		FMatrix& OutProjectionMatrix)
	{
		// Calculate shadow frustum center
		FVector Center = Bounds.GetCenter();
		FVector Extent = Bounds.GetExtent();
		float Radius = Extent.Size();

		// Light position (far behind the scene in light direction)
		FVector LightPos = Center - LightDirection.GetSafeNormal() * Radius * 2.0f;

		// Create view matrix (look at center from light position)
		FVector UpVector = FMath::Abs(LightDirection.Z) < 0.99f
			? FVector::UpVector
			: FVector::ForwardVector;

		OutViewMatrix = FLookFromMatrix(LightPos, LightDirection.GetSafeNormal(), UpVector);

		// Create orthographic projection matrix
		// Ortho bounds based on scene extent
		float OrthoWidth = Radius * 2.0f;
		float OrthoHeight = Radius * 2.0f;
		float NearPlane = 0.1f;
		float FarPlane = Radius * 4.0f;

		OutProjectionMatrix = FReversedZOrthoMatrix(
			OrthoWidth,
			OrthoHeight,
			1.0f / (FarPlane - NearPlane),
			NearPlane
		);
	}

	/**
	 * Get shadow light parameters from a directional light actor.
	 *
	 * @param Light The directional light actor.
	 * @param FluidBounds World space bounds of the fluid.
	 * @param OutParams Output light parameters.
	 */
	inline void GetDirectionalLightParams(
		ADirectionalLight* Light,
		const FBox& FluidBounds,
		FFluidShadowLightParams& OutParams)
	{
		if (!Light || !Light->GetLightComponent())
		{
			OutParams.bIsValid = false;
			return;
		}

		UDirectionalLightComponent* LightComp = Cast<UDirectionalLightComponent>(Light->GetLightComponent());

		// Get light direction (component forward vector points towards light)
		FVector LightDir = -LightComp->GetForwardVector();
		OutParams.LightDirection = FVector3f(LightDir);

		// Expand bounds for shadow projection
		FBox ExpandedBounds = FluidBounds.ExpandBy(FluidBounds.GetExtent().Size() * 0.5f);
		OutParams.ShadowBounds = ExpandedBounds;

		// Calculate view and projection matrices
		FMatrix ViewMatrix, ProjectionMatrix;
		CalculateDirectionalLightMatrices(LightDir, ExpandedBounds, ViewMatrix, ProjectionMatrix);

		// Combine into view-projection matrix
		OutParams.LightViewProjectionMatrix = FMatrix44f(ViewMatrix * ProjectionMatrix);
		OutParams.bIsValid = true;
	}

	/**
	 * Get shadow light parameters from scene view (for main directional light).
	 * Uses the view's family scene to find directional light if available.
	 *
	 * @param View The scene view.
	 * @param FluidBounds World space bounds of the fluid.
	 * @param OutParams Output light parameters.
	 */
	inline void GetLightParamsFromView(
		const FSceneView& View,
		const FBox& FluidBounds,
		FFluidShadowLightParams& OutParams)
	{
		// Try to get directional light direction from the world
		FVector LightDir = FVector(0.5f, 0.5f, -0.707f).GetSafeNormal();

		// Try to find main directional light in the scene
		if (View.Family && View.Family->Scene)
		{
			UWorld* World = View.Family->Scene->GetWorld();
			if (ADirectionalLight* Light = FindMainDirectionalLight(World))
			{
				if (UDirectionalLightComponent* LightComp = Cast<UDirectionalLightComponent>(Light->GetLightComponent()))
				{
					LightDir = -LightComp->GetForwardVector();
				}
			}
		}

		OutParams.LightDirection = FVector3f(LightDir);

		// Expand bounds
		FBox ExpandedBounds = FluidBounds.ExpandBy(FluidBounds.GetExtent().Size() * 0.5f);
		OutParams.ShadowBounds = ExpandedBounds;

		// Calculate matrices
		FMatrix ViewMatrix, ProjectionMatrix;
		CalculateDirectionalLightMatrices(LightDir, ExpandedBounds, ViewMatrix, ProjectionMatrix);

		OutParams.LightViewProjectionMatrix = FMatrix44f(ViewMatrix * ProjectionMatrix);
		OutParams.bIsValid = true;
	}
}
