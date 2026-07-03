// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ExternalAIGrpcRuntime : ModuleRules
{
	public ExternalAIGrpcRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;
		bUseUnity = false;

		PrivateIncludePaths.Add("ExternalAIGrpcRuntime/Private/Generated");

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"LyraGame",
				"ExternalAIGrpc",
				"Protobuf"
			});
	}
}
