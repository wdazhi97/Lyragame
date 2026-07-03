// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class ExternalAIGrpc : ModuleRules
{
	public ExternalAIGrpc(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		const string Version = "1.72.0";
		string VersionRoot = Path.Combine(ModuleDirectory, Version);
		string LibraryRoot;

		PublicSystemIncludePaths.Add(Path.Combine(VersionRoot, "include"));

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Protobuf",
				"abseil",
				"cares",
				"Re2",
				"zlib",
				"OpenSSL"
			});

		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			LibraryRoot = Path.Combine(VersionRoot, "lib", "Mac", Target.Architecture.AppleName, "Release");
			PublicFrameworks.Add("CoreFoundation");
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			LibraryRoot = Path.Combine(VersionRoot, "lib", "Unix", Target.Architecture.LinuxName, "Release");
			PublicSystemLibraries.AddRange(new[] { "pthread", "dl" });
		}
		else
		{
			throw new BuildException("ExternalAIGrpc currently supports Mac and Linux only.");
		}

		string[] Libraries =
		{
			"grpc++",
			"grpc",
			"gpr",
			"address_sorting",
			"upb_json_lib",
			"upb_textformat_lib",
			"upb_wire_lib",
			"upb_message_lib",
			"upb_mini_descriptor_lib",
			"upb_mem_lib",
			"upb_base_lib",
			"utf8_range_lib"
		};

		foreach (string Library in Libraries)
		{
			string LibraryPath = Path.Combine(LibraryRoot, $"lib{Library}.a");
			if (!File.Exists(LibraryPath))
			{
				throw new BuildException(
					$"Missing UE-compatible gRPC library '{LibraryPath}'. " +
					$"Run the ExternalAIGrpc BuildForUE script for {Target.Platform} first.");
			}
			PublicAdditionalLibraries.Add(LibraryPath);
		}

		PublicDefinitions.Add("WITH_EXTERNAL_AI_GRPC=1");
	}
}
