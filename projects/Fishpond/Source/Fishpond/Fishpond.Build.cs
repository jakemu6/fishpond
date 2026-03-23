// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Fishpond : ModuleRules
{
	public Fishpond(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "StateTreeModule", "GameplayStateTreeModule" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

        // RealSense SDK
        string RealSenseSDKPath = "D:/RealSense SDK 2.0";
        string DLLPath = RealSenseSDKPath + "/bin/x64/realsense2.dll";

        PublicSystemIncludePaths.Add(RealSenseSDKPath + "/include");
        PublicAdditionalLibraries.Add(RealSenseSDKPath + "/lib/x64/realsense2.lib");
        RuntimeDependencies.Add("$(BinaryOutputDir)/realsense2.dll", DLLPath);

    }
}
