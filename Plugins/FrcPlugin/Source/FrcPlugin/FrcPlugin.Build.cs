// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FrcPlugin : ModuleRules
{
	public FrcPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				"D:/Games/UE_4.25/Engine/Source/Runtime/Renderer/Private",
				"D:/Games/UE_4.25/Engine/Source/Runtime/Renderer/Private/CompositionLighting",
				"D:/Games/UE_4.25/Engine/Source/Runtime/Renderer/Private/PostProcess",
				"D:/Games/UE_4.25/Engine/Shaders/Shared"
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"MaterialShaderQualitySettings",
				"RHI",
				"Engine",
				"RenderCore",
				"Renderer",
				"RHI"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Projects",
				"RenderCore",
				"Renderer",
				"ImageWriteQueue",
				"RHI"
				// ... add private dependencies that you statically link with here ...	
			}
			);

		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.Add("TargetPlatform");
		}


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
