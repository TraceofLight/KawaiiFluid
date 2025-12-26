// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Rendering/KawaiiFluidRenderController.h"
#include "Rendering/KawaiiFluidISMRenderer.h"
#include "Rendering/KawaiiFluidSSFRRenderer.h"
#include "Core/FluidParticle.h"

UKawaiiFluidRenderController::UKawaiiFluidRenderController()
{
	// Create renderer instances as default subobjects (Instanced pattern)
	ISMRenderer = CreateDefaultSubobject<UKawaiiFluidISMRenderer>(TEXT("ISMRenderer"));
	SSFRRenderer = CreateDefaultSubobject<UKawaiiFluidSSFRRenderer>(TEXT("SSFRRenderer"));
}

void UKawaiiFluidRenderController::Initialize(UWorld* InWorld, AActor* InOwner, IKawaiiFluidDataProvider* InDataProvider)
{
	CachedWorld = InWorld;
	CachedOwner = InOwner;
	DataProviderPtr = InDataProvider;

	// Initialize renderers (already created in constructor)
	if (ISMRenderer)
	{
		ISMRenderer->Initialize(InWorld, InOwner);
	}

	if (SSFRRenderer)
	{
		SSFRRenderer->Initialize(InWorld, InOwner);
	}

	UE_LOG(LogTemp, Log, TEXT("RenderController: Initialized (ISM: %s, SSFR: %s)"),
		ISMRenderer && ISMRenderer->IsEnabled() ? TEXT("Enabled") : TEXT("Disabled"),
		SSFRRenderer && SSFRRenderer->IsEnabled() ? TEXT("Enabled") : TEXT("Disabled"));
}

void UKawaiiFluidRenderController::Cleanup()
{
	if (ISMRenderer)
	{
		ISMRenderer->Cleanup();
	}

	if (SSFRRenderer)
	{
		SSFRRenderer->Cleanup();
	}

	DataProviderPtr = nullptr;
	CachedWorld = nullptr;
	CachedOwner = nullptr;
}

void UKawaiiFluidRenderController::UpdateRenderers()
{
	if (!DataProviderPtr || !DataProviderPtr->IsDataValid())
	{
		return;
	}

	// Update all enabled renderers
	if (ISMRenderer && ISMRenderer->IsEnabled())
	{
		ISMRenderer->UpdateRendering(DataProviderPtr, 0.0f);
	}

	if (SSFRRenderer && SSFRRenderer->IsEnabled())
	{
		SSFRRenderer->UpdateRendering(DataProviderPtr, 0.0f);
	}
}

int32 UKawaiiFluidRenderController::GetParticleCount() const
{
	if (DataProviderPtr)
	{
		return DataProviderPtr->GetParticleCount();
	}
	return 0;
}
