// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LobbyFlow : ModuleRules
{
	public LobbyFlow(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"ModelViewViewModel"
			}
		);
	}
}
