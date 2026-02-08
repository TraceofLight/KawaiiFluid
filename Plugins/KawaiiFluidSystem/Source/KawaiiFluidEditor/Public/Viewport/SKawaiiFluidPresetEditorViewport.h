// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "SCommonEditorViewportToolbarBase.h"

class FKawaiiFluidPreviewScene;
class FKawaiiFluidPresetAssetEditor;
class FKawaiiFluidPresetEditorViewportClient;

/**
 * @brief SKawaiiFluidPresetEditorViewport
 * 
 * Viewport widget for the fluid preset editor.
 * Displays the 3D preview of the fluid simulation and handles viewport-specific
 * interactions and stats overlays.
 * 
 * @param ViewportClient Shared pointer to the viewport client for camera/input logic
 * @param PreviewScene Reference to the preview world being displayed
 * @param AssetEditorPtr Weak pointer back to the parent asset editor
 */
class KAWAIIFLUIDEDITOR_API SKawaiiFluidPresetEditorViewport : public SEditorViewport,
                                                          public FGCObject,
                                                          public ICommonEditorViewportToolbarInfoProvider
{
public:
	SLATE_BEGIN_ARGS(SKawaiiFluidPresetEditorViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs,
	               TSharedPtr<FKawaiiFluidPreviewScene> InPreviewScene,
	               TSharedPtr<FKawaiiFluidPresetAssetEditor> InAssetEditor);

	virtual ~SKawaiiFluidPresetEditorViewport() override;

	//~ Begin FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("SFluidPresetEditorViewport"); }
	//~ End FGCObject Interface

	//~ Begin ICommonEditorViewportToolbarInfoProvider Interface
	virtual TSharedRef<SEditorViewport> GetViewportWidget() override;
	virtual TSharedPtr<FExtender> GetExtenders() const override;
	virtual void OnFloatingButtonClicked() override;
	//~ End ICommonEditorViewportToolbarInfoProvider Interface

	void RefreshViewport();

	void FocusOnParticles();

	void ResetCamera();

	TSharedPtr<FKawaiiFluidPresetEditorViewportClient> GetViewportClient() const { return ViewportClient; }

	TSharedPtr<FKawaiiFluidPreviewScene> GetPreviewScene() const { return PreviewScene; }

protected:
	//~ Begin SEditorViewport Interface
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual void PopulateViewportOverlays(TSharedRef<SOverlay> Overlay) override;
	virtual void BindCommands() override;
	//~ End SEditorViewport Interface

private:
	/** Viewport client */
	TSharedPtr<FKawaiiFluidPresetEditorViewportClient> ViewportClient;

	/** Preview scene reference */
	TSharedPtr<FKawaiiFluidPreviewScene> PreviewScene;

	/** Asset editor reference */
	TWeakPtr<FKawaiiFluidPresetAssetEditor> AssetEditorPtr;
};