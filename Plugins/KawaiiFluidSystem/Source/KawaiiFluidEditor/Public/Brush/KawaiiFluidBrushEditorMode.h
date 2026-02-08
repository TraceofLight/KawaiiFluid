// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EdMode.h"

class AKawaiiFluidVolume;
class UKawaiiFluidVolumeComponent;

/**
 * @brief FKawaiiFluidBrushEditorMode
 * 
 * Fluid particle brush editor mode.
 * Activated by detail panel button and operates on a specific FluidComponent target.
 * 
 * @param TargetVolume The volume actor being painted on
 * @param TargetVolumeComponent The simulation component of the target volume
 * @param BrushLocation Current 3D world position of the brush
 * @param BrushNormal Surface normal at the brush location
 * @param bValidLocation Whether the brush is currently over a valid target
 * @param bPainting Whether the user is currently holding the paint button
 */
class FKawaiiFluidBrushEditorMode : public FEdMode
{
public:
	static const FEditorModeID EM_FluidBrush;

	FKawaiiFluidBrushEditorMode();
	virtual ~FKawaiiFluidBrushEditorMode() override;

	//~ Begin FEdMode Interface
	virtual void Enter() override;
	virtual void Exit() override;
	virtual bool UsesToolkits() const override { return false; }

	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport,
	                      FKey Key, EInputEvent Event) override;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy,
	                         const FViewportClick& Click) override;
	virtual bool StartTracking(FEditorViewportClient* ViewportClient, FViewport* Viewport) override;
	virtual bool EndTracking(FEditorViewportClient* ViewportClient, FViewport* Viewport) override;
	virtual bool MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport,
	                       int32 x, int32 y) override;
	virtual bool CapturedMouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport,
	                               int32 InMouseX, int32 InMouseY) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport,
	                     const FSceneView* View, FCanvas* Canvas) override;
	virtual bool IsSelectionAllowed(AActor* InActor, bool bInSelected) const override { return false; }
	virtual bool ShouldDrawWidget() const override { return false; }
	virtual bool DisallowMouseDeltaTracking() const override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	//~ End FEdMode Interface

	void SetTargetVolume(AKawaiiFluidVolume* Volume);

	AKawaiiFluidVolume* GetTargetVolume() const { return TargetVolume.Get(); }

	bool IsTargetingVolume() const { return TargetVolume.IsValid(); }

private:
	TWeakObjectPtr<AKawaiiFluidVolume> TargetVolume;

	TWeakObjectPtr<UKawaiiFluidVolumeComponent> TargetVolumeComponent;

	FVector BrushLocation {};

	FVector BrushNormal { FVector::UpVector };

	bool bValidLocation = false;

	bool bPainting = false;

	double LastStrokeTime = 0.0;

	FDelegateHandle SelectionChangedHandle;

	void OnSelectionChanged(UObject* Object);

	TWeakObjectPtr<AActor> TargetOwnerActor;

	bool UpdateBrushLocation(FEditorViewportClient* ViewportClient, int32 MouseX, int32 MouseY);

	void ApplyBrush();

	void DrawBrushPreview(FPrimitiveDrawInterface* PDI);

	FLinearColor GetBrushColor() const;
};
