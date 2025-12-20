// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class KawaiiFluid_ver1 : ModuleRules
{
	public KawaiiFluid_ver1(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"KawaiiFluid_ver1",
			"KawaiiFluid_ver1/Variant_Platforming",
			"KawaiiFluid_ver1/Variant_Platforming/Animation",
			"KawaiiFluid_ver1/Variant_Combat",
			"KawaiiFluid_ver1/Variant_Combat/AI",
			"KawaiiFluid_ver1/Variant_Combat/Animation",
			"KawaiiFluid_ver1/Variant_Combat/Gameplay",
			"KawaiiFluid_ver1/Variant_Combat/Interfaces",
			"KawaiiFluid_ver1/Variant_Combat/UI",
			"KawaiiFluid_ver1/Variant_SideScrolling",
			"KawaiiFluid_ver1/Variant_SideScrolling/AI",
			"KawaiiFluid_ver1/Variant_SideScrolling/Gameplay",
			"KawaiiFluid_ver1/Variant_SideScrolling/Interfaces",
			"KawaiiFluid_ver1/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
