// Copyright KawaiiFluid Team. All Rights Reserved.

#include "Components/KawaiiFluidSimulationVolume.h"
#include "Components/KawaiiFluidSimulationVolumeComponent.h"

AKawaiiFluidSimulationVolume::AKawaiiFluidSimulationVolume()
{
	// Create the volume component as root
	VolumeComponent = CreateDefaultSubobject<UKawaiiFluidSimulationVolumeComponent>(TEXT("VolumeComponent"));
	RootComponent = VolumeComponent;

	// Don't need to tick the actor itself - the component handles ticking
	PrimaryActorTick.bCanEverTick = false;
}
