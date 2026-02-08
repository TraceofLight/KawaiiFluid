// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FKawaiiFluidPreviewScene;

/**
 * @brief SKawaiiFluidPreviewStatsOverlay
 * 
 * Stats overlay widget for the fluid preview viewport.
 * Displays real-time information like particle count, FPS, and simulation time.
 * 
 * @param PreviewScenePtr Weak pointer back to the preview scene for data polling
 * @param CachedFPS Average frames per second
 * @param FPSAccumulator Sum of frame times for averaging
 * @param FrameCount Number of frames since last FPS update
 * @param CachedParticleCount Number of particles in the simulation
 */
class KAWAIIFLUIDEDITOR_API SKawaiiFluidPreviewStatsOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKawaiiFluidPreviewStatsOverlay) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FKawaiiFluidPreviewScene> InPreviewScene);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	FText GetParticleCountText() const;
	FText GetSimulationTimeText() const;
	FText GetFPSText() const;

private:
	TWeakPtr<FKawaiiFluidPreviewScene> PreviewScenePtr;

	float CachedFPS{};
	float FPSAccumulator{};
	int32 FrameCount{};
	int32 CachedParticleCount{};
};
