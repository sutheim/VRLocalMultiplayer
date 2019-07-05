// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class LocalMultiplayerInputTweaker : ModuleRules
{
	public LocalMultiplayerInputTweaker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        if (Target.Version.MinorVersion <= 19)
        {
            PublicIncludePaths.AddRange(
			    new string[] {
				    "Public"
			    });

		    PrivateIncludePaths.AddRange(
			    new string[] {
				    "Private"
			    });
        }

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"EngineSettings",
				"Slate",
				"InputCore"
            });
    }
}