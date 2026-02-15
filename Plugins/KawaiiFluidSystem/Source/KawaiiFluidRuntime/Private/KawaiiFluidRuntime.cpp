// Copyright 2026 Team_Bruteforce. All Rights Reserved.

#include "KawaiiFluidRuntime.h"
#include "Logging/KawaiiFluidLog.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FKawaiiFluidRuntimeModule"

DEFINE_LOG_CATEGORY(LogKawaiiFluid);

void FKawaiiFluidRuntimeModule::StartupModule()
{
	// Map plugin shader directory
	FString PluginShaderPath = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("KawaiiFluidSystem"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/KawaiiFluidSystem"), PluginShaderPath);

	KF_LOG_DEV(Log, TEXT("Runtime module started: Shader Directory: %s"), *PluginShaderPath);
}

void FKawaiiFluidRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FKawaiiFluidRuntimeModule, KawaiiFluidRuntime)
