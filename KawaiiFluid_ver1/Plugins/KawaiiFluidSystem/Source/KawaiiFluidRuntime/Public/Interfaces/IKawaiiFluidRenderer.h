// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Rendering/KawaiiFluidRenderingMode.h"
#include "IKawaiiFluidRenderer.generated.h"

class IKawaiiFluidDataProvider;

/**
 * UInterface (Unreal Reflection System)
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UKawaiiFluidRenderer : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for fluid rendering implementations
 *
 * Different rendering modes implement this interface to render particles
 * in their own way.
 *
 * Responsibilities:
 * 1. Fetch simulation data (FFluidParticle) from DataProvider
 * 2. Convert simulation data to rendering data (FKawaiiRenderParticle)
 * 3. Perform actual rendering with converted data
 *
 * This design ensures simulation layer has no dependency on rendering layer.
 *
 * Implementations:
 * - UKawaiiFluidISMRenderer (Instanced Static Mesh)
 * - UKawaiiFluidSSFRRenderer (Screen Space Fluid Rendering)
 * - UKawaiiFluidNiagaraRenderer (Niagara Particles)
 *
 * Usage:
 * @code
 * // Used by Visual Component
 * for (IKawaiiFluidRenderer* Renderer : Renderers)
 * {
 *     if (Renderer->IsEnabled())
 *     {
 *         Renderer->UpdateRendering(DataProvider, DeltaTime);
 *     }
 * }
 * @endcode
 */
class IKawaiiFluidRenderer
{
	GENERATED_BODY()

public:
	/**
	 * Update rendering
	 *
	 * Implementation should:
	 * 1. Get simulation data via DataProvider->GetParticles()
	 * 2. Convert FFluidParticle -> FKawaiiRenderParticle (extract rendering data)
	 * 3. Perform rendering with converted data
	 *
	 * @param DataProvider Particle data provider (simulation data)
	 * @param DeltaTime Frame delta time
	 */
	virtual void UpdateRendering(const IKawaiiFluidDataProvider* DataProvider, float DeltaTime) = 0;

	/**
	 * Check if rendering is enabled
	 * @return true if rendering is active
	 */
	virtual bool IsEnabled() const = 0;

	/**
	 * Get rendering mode
	 * @return Rendering mode of this renderer
	 */
	virtual EKawaiiFluidRenderingMode GetRenderingMode() const = 0;

	/**
	 * Enable or disable rendering
	 * @param bInEnabled New enabled state
	 */
	virtual void SetEnabled(bool bInEnabled) = 0;
};

