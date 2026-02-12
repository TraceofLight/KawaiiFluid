// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "Modules/KawaiiFluidRenderingModule.h"
#include "Rendering/KawaiiFluidProxyRenderer.h"
#include "Rendering/KawaiiFluidRenderer.h"
#include "Core/KawaiiFluidParticle.h"

/**
 * @brief Default constructor creating default renderer subobjects.
 */
UKawaiiFluidRenderingModule::UKawaiiFluidRenderingModule()
{
	ISMRenderer = CreateDefaultSubobject<UKawaiiFluidProxyRenderer>(TEXT("KawaiiFluidISMRenderer"));
	MetaballRenderer = CreateDefaultSubobject<UKawaiiFluidRenderer>(TEXT("KawaiiFluidMetaballRenderer"));
}

/**
 * @brief Handles cleanup of stale pointers after duplication (e.g. for PIE).
 * @param bDuplicateForPIE Whether this is a PIE duplication.
 */
void UKawaiiFluidRenderingModule::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	CachedWorld = nullptr;
	CachedOwnerComponent = nullptr;
	DataProviderPtr = nullptr;

	if (ISMRenderer)
	{
		ISMRenderer->Cleanup();
	}
	if (MetaballRenderer)
	{
		MetaballRenderer->Cleanup();
	}

	UE_LOG(LogTemp, Log, TEXT("UKawaiiFluidRenderingModule::PostDuplicate - Cleared stale pointers (PIE=%d)"),
		bDuplicateForPIE ? 1 : 0);
}

/**
 * @brief Initialize the rendering module and its sub-renderers.
 * @param InWorld World context.
 * @param InOwnerComponent Parent component for renderer attachment.
 * @param InDataProvider Source of simulation particle data.
 * @param InPreset Preset data asset for rendering parameters.
 */
void UKawaiiFluidRenderingModule::Initialize(UWorld* InWorld, USceneComponent* InOwnerComponent, IKawaiiFluidDataProvider* InDataProvider, UKawaiiFluidPresetDataAsset* InPreset)
{
	CachedWorld = InWorld;
	CachedOwnerComponent = InOwnerComponent;
	DataProviderPtr = InDataProvider;

	if (!ISMRenderer)
	{
		ISMRenderer = NewObject<UKawaiiFluidProxyRenderer>(this, TEXT("ISMRenderer"));
	}

	if (!MetaballRenderer)
	{
		MetaballRenderer = NewObject<UKawaiiFluidRenderer>(this, TEXT("MetaballRenderer"));
	}

	if (ISMRenderer)
	{
		ISMRenderer->Initialize(InWorld, InOwnerComponent, InPreset);
	}

	if (MetaballRenderer)
	{
		MetaballRenderer->Initialize(InWorld, InOwnerComponent, InPreset);
	}

	UE_LOG(LogTemp, Log, TEXT("RenderingModule: Initialized (ISM: %s, Metaball: %s)"),
		ISMRenderer && ISMRenderer->IsEnabled() ? TEXT("Enabled") : TEXT("Disabled"),
		MetaballRenderer && MetaballRenderer->IsEnabled() ? TEXT("Enabled") : TEXT("Disabled"));
}

/**
 * @brief Release resources and cleanup sub-renderers.
 */
void UKawaiiFluidRenderingModule::Cleanup()
{
	if (ISMRenderer)
	{
		ISMRenderer->Cleanup();
	}

	if (MetaballRenderer)
	{
		MetaballRenderer->Cleanup();
	}

	DataProviderPtr = nullptr;
	CachedWorld = nullptr;
	CachedOwnerComponent = nullptr;
}

/**
 * @brief Fetch data from the provider and update all enabled renderers.
 */
void UKawaiiFluidRenderingModule::UpdateRenderers()
{
	if (!DataProviderPtr)
	{
		return;
	}

	if (MetaballRenderer && MetaballRenderer->IsEnabled())
	{
		MetaballRenderer->UpdateRendering(DataProviderPtr, 0.0f);
	}
	else if (ISMRenderer && ISMRenderer->IsEnabled())
	{
		ISMRenderer->UpdateRendering(DataProviderPtr, 0.0f);
	}
}

/**
 * @brief Get the current number of particles being rendered.
 * @return Particle count from the data provider.
 */
int32 UKawaiiFluidRenderingModule::GetParticleCount() const
{
	if (DataProviderPtr)
	{
		return DataProviderPtr->GetParticleCount();
	}
	return 0;
}