// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class MultiverseClientLibrary : ModuleRules
{
	public MultiverseClientLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		// Add any include paths for the plugin
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory));

		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			string MultiverseClientSoPath = Path.Combine("$(PluginDir)", "Binaries", "ThirdParty", "MultiverseClientLibrary", "libmultiverse_client_lib.so");
			PublicAdditionalLibraries.Add(MultiverseClientSoPath);
			PublicDelayLoadDLLs.Add(MultiverseClientSoPath);
			RuntimeDependencies.Add(MultiverseClientSoPath);
		}
	}
}
