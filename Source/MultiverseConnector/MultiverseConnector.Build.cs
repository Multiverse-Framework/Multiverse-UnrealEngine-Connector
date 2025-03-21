// Copyright (c) 2022, Giang Hoang Nguyen - Institute for Artificial Intelligence, University Bremen

using System.IO;
using UnrealBuildTool;

public class MultiverseConnector : ModuleRules
{
  public MultiverseConnector(ReadOnlyTargetRules Target) : base(Target)
  {
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

    PublicDependencyModuleNames.AddRange(
      new string[]
      {
        "Core",
        "CoreUObject",
        "Engine",
        "Projects",
        "InputCore",
        "Json",
        "JsonUtilities",
        "AnimGraphRuntime",
        "MultiverseClientLibrary",
      }
      );

    if (Target.Platform == UnrealTargetPlatform.Win64)
    {
      PrivateDependencyModuleNames.AddRange(
        new string[]
        {
          "OculusXRInput"
        });
    }

    // Uncomment if you are using Slate UI
    // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

    // Uncomment if you are using online features
    // PrivateDependencyModuleNames.Add("OnlineSubsystem");

    // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

    bEnableExceptions = true;
  }
}
