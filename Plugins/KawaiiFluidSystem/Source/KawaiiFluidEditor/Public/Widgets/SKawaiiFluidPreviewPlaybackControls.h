// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FKawaiiFluidPresetAssetEditor;

/**
 * @brief SKawaiiFluidPreviewPlaybackControls
 * 
 * Playback control widget for fluid preview.
 * Contains Play, Pause, Stop, Reset buttons and simulation speed controls.
 * 
 * @param EditorPtr Weak pointer back to the parent asset editor for control commands
 */
class KAWAIIFLUIDEDITOR_API SKawaiiFluidPreviewPlaybackControls : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKawaiiFluidPreviewPlaybackControls) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FKawaiiFluidPresetAssetEditor> InEditor);

private:
	FReply OnPlayPauseClicked();
	FReply OnStopClicked();
	FReply OnResetClicked();

	bool IsPlaying() const;
	bool IsPaused() const;
	bool CanPlay() const;

	FText GetPlayPauseButtonText() const;
	FText GetPlayPauseTooltip() const;

	void OnSpeedChanged(float NewValue);
	float GetCurrentSpeed() const;
	TOptional<float> GetSpeedAsOptional() const;
	FText GetSpeedText() const;

private:
	TWeakPtr<FKawaiiFluidPresetAssetEditor> EditorPtr;
};
