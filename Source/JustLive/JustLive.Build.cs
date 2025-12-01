// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class JustLive : ModuleRules
{
	public JustLive(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "XmlParser", "PakFile", "ImageWrapper", "MetasoundEngine", "AIModule", "NavigationSystem", "Json", "JsonUtilities", "Niagara", "PhysicsCore", "Scripting" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}
