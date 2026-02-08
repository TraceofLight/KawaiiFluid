// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetTypeCategories.h"

class IAssetTypeActions;

/**
 * @brief FKawaiiFluidEditorModule
 * 
 * Main editor module for the KawaiiFluid system.
 * Handles registration of asset tools, property customizations, and editor modes.
 * 
 * @param RegisteredAssetTypeActions List of registered asset actions for cleanup
 * @param FluidAssetCategory The custom asset category for fluid assets
 */
class FKawaiiFluidEditorModule : public IModuleInterface
{
public:
	//~ Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~ End IModuleInterface

	static FKawaiiFluidEditorModule& Get();

	EAssetTypeCategories::Type GetAssetCategory() const { return FluidAssetCategory; }

private:
	void RegisterAssetTypeActions();

	void UnregisterAssetTypeActions();

	void RegisterPropertyCustomizations();

	void UnregisterPropertyCustomizations();

	void HandleAssetPreSave(UPackage* InPackage, FObjectPreSaveContext InContext);

private:
	/** Registered asset type actions */
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;

	/** Custom asset type category for fluid assets */
	EAssetTypeCategories::Type FluidAssetCategory {};
};