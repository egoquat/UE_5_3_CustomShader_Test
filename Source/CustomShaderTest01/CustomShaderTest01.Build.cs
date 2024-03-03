// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CustomShaderTest01 : ModuleRules
{
	public CustomShaderTest01(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "CustomShader" });
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
