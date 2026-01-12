// Copyright KawaiiFluid Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KawaiiFluidSimulationVolume.generated.h"

class UKawaiiFluidSimulationVolumeComponent;

/**
 * Kawaii Fluid Simulation Volume
 *
 * An actor that contains a UKawaiiFluidSimulationVolumeComponent.
 * Place this actor in the level to define a Z-Order simulation space.
 *
 * Usage:
 * 1. Place AKawaiiFluidSimulationVolume in the level
 * 2. Configure the Volume component's CellSize
 * 3. Assign this Actor to UKawaiiFluidComponent's TargetSimulationVolume property
 *
 * All fluid components referencing the same SimulationVolume will:
 * - Share the same Z-Order space bounds
 * - Be able to interact with each other (if same Preset)
 * - Be batched together for better performance
 */
UCLASS(Blueprintable, meta = (DisplayName = "Kawaii Fluid Simulation Volume"))
class KAWAIIFLUIDRUNTIME_API AKawaiiFluidSimulationVolume : public AActor
{
	GENERATED_BODY()

public:
	AKawaiiFluidSimulationVolume();

	/** Get the volume component */
	UFUNCTION(BlueprintPure, Category = "KawaiiFluid")
	UKawaiiFluidSimulationVolumeComponent* GetVolumeComponent() const { return VolumeComponent; }

protected:
	/** The fluid simulation volume component that defines the Z-Order space */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KawaiiFluid")
	TObjectPtr<UKawaiiFluidSimulationVolumeComponent> VolumeComponent;
};
