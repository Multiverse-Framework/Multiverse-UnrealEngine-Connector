// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;
// ReSharper disable All

public class MultiverseClientLibrary : ModuleRules
{
	public MultiverseClientLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Add any include paths for the plugin
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory));

		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			string MultiverseClientSoPath = Path.Combine("$(PluginDir)", "Binaries", "Linux", "libmultiverse_client.so");
			PublicAdditionalLibraries.Add(MultiverseClientSoPath);
			PublicDelayLoadDLLs.Add(MultiverseClientSoPath);
			RuntimeDependencies.Add(MultiverseClientSoPath);
		}
		else if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string MultiverseClientLibPath = Path.Combine("$(PluginDir)", "Binaries", "Win64", "multiverse_client.lib");
			PublicAdditionalLibraries.Add(MultiverseClientLibPath);
			
			string ZmqLibPath = Path.Combine("$(PluginDir)", "Binaries", "Win64", "libzmq-mt-4_3_5.lib");
			PublicAdditionalLibraries.Add(ZmqLibPath);
		}
		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
			string MultiverseClientLibPath = Path.Combine("$(PluginDir)", "Binaries", "Android", "libmultiverse_client.so");
			PublicAdditionalLibraries.Add(MultiverseClientLibPath);
			// string CxxLibPath = Path.Combine("$(PluginDir)", "Binaries", "Android", "libstdc++.so.6");
			// PublicAdditionalLibraries.Add(CxxLibPath);
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "MultiverseClient_APL.xml"));
		}
	}
}
