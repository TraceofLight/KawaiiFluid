// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "EditorUndoClient.h"

class UKawaiiFluidPresetDataAsset;
class SKawaiiFluidPresetEditorViewport;
class FKawaiiFluidPreviewScene;
class UFluidPreviewSettingsObject;

/**
 * @brief FKawaiiFluidPresetAssetEditor
 * 
 * Main asset editor toolkit for KawaiiFluidPresetDataAsset.
 * Provides a dedicated workspace with a 3D viewport, real-time simulation, 
 * property details, and preview settings.
 * 
 * @param EditingPreset Pointer to the preset asset being edited
 * @param PreviewScene Shared pointer to the 3D preview world
 * @param ViewportWidget Shared pointer to the viewport UI widget
 * @param DetailsView Property editor for the preset asset
 * @param PreviewSettingsView Property editor for the simulation settings
 * @param bIsPlaying Current state of the simulation playback
 * @param SimulationSpeed Multiplier for the simulation time step
 */
class KAWAIIFLUIDEDITOR_API FKawaiiFluidPresetAssetEditor : public FAssetEditorToolkit,
                                                            public FEditorUndoClient,
                                                            public FTickableEditorObject
{
public:
	FKawaiiFluidPresetAssetEditor();
	virtual ~FKawaiiFluidPresetAssetEditor() override;

	void InitFluidPresetEditor(
		const EToolkitMode::Type Mode,
		const TSharedPtr<IToolkitHost>& InitToolkitHost,
		UKawaiiFluidPresetDataAsset* InPreset);

	//~ Begin FAssetEditorToolkit Interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	//~ End FAssetEditorToolkit Interface

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	//~ End FEditorUndoClient Interface

	//~ Begin FTickableEditorObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return true; }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	//~ End FTickableEditorObject Interface

	//========================================
	// Playback Control
	//========================================

	void Play();

	void Pause();

	void Stop();

	void Reset();

	bool IsPlaying() const { return bIsPlaying; }

	void SetSimulationSpeed(float Speed);

	float GetSimulationSpeed() const { return SimulationSpeed; }

	//========================================
	// Accessors
	//========================================

	UKawaiiFluidPresetDataAsset* GetEditingPreset() const { return EditingPreset; }

	TSharedPtr<FKawaiiFluidPreviewScene> GetPreviewScene() const { return PreviewScene; }

	//========================================
	// Property Change Handling
	//========================================

	void OnPresetPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent);

	void OnPreviewSettingsPropertyChanged(const FPropertyChangedEvent& PropertyChangedEvent);

private:
	//========================================
	// Tab Spawners
	//========================================

	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_PreviewSettings(const FSpawnTabArgs& Args);

	//========================================
	// Toolbar
	//========================================

	void ExtendToolbar();

	//========================================
	// Internal Methods
	//========================================

	void RefreshPreview();
	void UpdateSimulation(float DeltaTime);
	void BindEditorDelegates();
	void UnbindEditorDelegates();

private:
	/** Preset being edited */
	UKawaiiFluidPresetDataAsset* EditingPreset;

	/** Preview scene */
	TSharedPtr<FKawaiiFluidPreviewScene> PreviewScene;

	/** Viewport widget */
	TSharedPtr<SKawaiiFluidPresetEditorViewport> ViewportWidget;

	/** Details view for preset properties */
	TSharedPtr<IDetailsView> DetailsView;

	/** Details view for preview settings */
	TSharedPtr<IDetailsView> PreviewSettingsView;

	/** Is simulation playing */
	bool bIsPlaying;

	/** Simulation speed multiplier */
	float SimulationSpeed;

	/** Tab IDs and App Identifier */
	static const FName ViewportTabId;
	static const FName DetailsTabId;
	static const FName PreviewSettingsTabId;
	static const FName AppIdentifier;
};