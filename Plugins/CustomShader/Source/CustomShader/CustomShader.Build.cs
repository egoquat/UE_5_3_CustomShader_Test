// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CustomShader : ModuleRules
{
	public CustomShader(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        	
        	if (Target.bBuildEditor == true)
        	{
        		PrivateDependencyModuleNames.Add("TargetPlatform");
        	}
        	PublicDependencyModuleNames.Add("Core");
        	PublicDependencyModuleNames.Add("Engine");
        	PublicDependencyModuleNames.Add("MaterialShaderQualitySettings");
        	
        	PrivateDependencyModuleNames.AddRange(new string[]
        	{
        		"CoreUObject",
        		"Renderer",
        		"RenderCore",
        		"RHI",
        		"Projects"
        	});
        	
        	if (Target.bBuildEditor == true)
        	{
        
        		PrivateDependencyModuleNames.AddRange(
        			new string[] {
        				"UnrealEd",
        				"MaterialUtilities",
        				"SlateCore",
        				"Slate"
        			}
        		);
        
        		CircularlyReferencedDependentModules.AddRange(
        			new string[] {
        				"UnrealEd",
        				"MaterialUtilities",
        			}
        		);
        	}
	}
}
