// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IDetailCustomization.h"

class UKawaiiFluidVolumeComponent;
class AKawaiiFluidVolume;

/**
 * @brief FKawaiiFluidVolumeComponentDetails
 * 
 * Custom details panel for UKawaiiFluidVolumeComponent.
 * Adds interactive buttons for brush mode and particle management.
 * 
 * @param TargetComponent The simulation component being customized
 * @param TargetVolume The owning volume actor
 */
class FKawaiiFluidVolumeComponentDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	//~ Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	//~ End IDetailCustomization Interface

private:
	TWeakObjectPtr<UKawaiiFluidVolumeComponent> TargetComponent;

	TWeakObjectPtr<AKawaiiFluidVolume> TargetVolume;

	FReply OnStartBrushClicked();

	FReply OnStopBrushClicked();

	FReply OnClearParticlesClicked();

	EVisibility GetStartVisibility() const;

	EVisibility GetStopVisibility() const;

	bool IsBrushActive() const;
};
