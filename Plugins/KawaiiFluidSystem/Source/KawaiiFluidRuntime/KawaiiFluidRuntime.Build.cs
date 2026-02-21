// Copyright 2026 Team_Bruteforce. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class KawaiiFluidRuntime : ModuleRules
{
	public KawaiiFluidRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		string ModulePath = ModuleDirectory;

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModulePath, "Public"),
				Path.Combine(ModulePath, "Public/Core"),
				Path.Combine(ModulePath, "Public/Components"),
				Path.Combine(ModulePath, "Public/Rendering"),
				Path.Combine(ModulePath, "Public/Tests")
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModulePath, "Private"),
				Path.Combine(ModulePath, "Private/Core"),
				Path.Combine(ModulePath, "Private/Components"),
				Path.Combine(ModulePath, "Private/Rendering"),
				Path.Combine(ModulePath, "Private/Tests"),
				Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private"),
				Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Internal")
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"RenderCore",
				"Renderer",
				"RHI",
				"Niagara"  // Required for Niagara component usage
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"Projects", // Required for IPluginManager
				"RenderCore", // Required for AddShaderSourceDirectoryMapping
				"Renderer",
				"Landscape", // Landscape module for heightmap collision
				"MeshDescription", // Low-poly shadow sphere generation
				"StaticMeshDescription" // FStaticMeshAttributes
			}
		);

		// Editor-only dependencies (FEditorDelegates for PIE sync)
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
