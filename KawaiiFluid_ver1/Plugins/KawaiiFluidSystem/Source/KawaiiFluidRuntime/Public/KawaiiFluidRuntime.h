// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FKawaiiFluidRuntimeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
