// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class LyraEditorTarget : TargetRules
{
	public LyraEditorTarget(TargetInfo Target) : base(Target)
	{
		DefaultBuildSettings = BuildSettingsVersion.V6;

		Type = TargetType.Editor;
		// Linux server cooking runs in UnrealEditor-Cmd and loads ServerOnly
		// project modules for the target platform. Build the server runtime
		// module into the editor target so the cook host can load it.
		ExtraModuleNames.AddRange(new string[] { "LyraGame", "LyraEditor", "ExternalAIGrpcRuntime" });

		if (!bBuildAllModules)
		{
			NativePointerMemberBehaviorOverride = PointerMemberBehavior.Disallow;
		}

		LyraGameTarget.ApplySharedLyraTargetSettings(this);

		// This is used for touch screen development along with the "Unreal Remote 2" app
		EnablePlugins.Add("RemoteSession");
	}
}
