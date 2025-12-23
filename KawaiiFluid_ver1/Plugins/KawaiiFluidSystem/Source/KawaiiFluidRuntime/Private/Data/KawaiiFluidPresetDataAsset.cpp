// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Data/KawaiiFluidPresetDataAsset.h"

UKawaiiFluidPresetDataAsset::UKawaiiFluidPresetDataAsset()
{
	// Default values are set in header
}

#if WITH_EDITOR
void UKawaiiFluidPresetDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Validate parameters
	if (SmoothingRadius < 1.0f)
	{
		SmoothingRadius = 1.0f;
	}
}
#endif
