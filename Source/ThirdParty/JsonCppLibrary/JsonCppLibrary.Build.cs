// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class JsonCppLibrary : ModuleRules
{
	public JsonCppLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		// Add any include paths for the plugin
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory));

		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			string JsonCppSoPath = Path.Combine("$(PluginDir)", "Binaries", "ThirdParty", "JsonCppLibrary", "libjsoncpp.so");
			PublicAdditionalLibraries.Add(JsonCppSoPath);
			PublicDelayLoadDLLs.Add(JsonCppSoPath);
			RuntimeDependencies.Add(JsonCppSoPath);
		}
	}
}
