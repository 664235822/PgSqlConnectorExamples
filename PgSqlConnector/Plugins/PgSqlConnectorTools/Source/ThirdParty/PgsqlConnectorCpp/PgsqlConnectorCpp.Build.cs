// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
// publish name:HaoJunDeveloper
// intended year of published:2024 

using System.IO;
using UnrealBuildTool;

public class PgsqlConnectorCpp : ModuleRules
{
	public PgsqlConnectorCpp(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		PublicSystemLibraryPaths.Add(Path.Combine(ModuleDirectory, "lib"));
		RuntimeDependencies.Add("$(BinaryOutputDir)/libcrypto-3-x64.dll",
			Path.Combine(ModuleDirectory, "lib", "libcrypto-3-x64.dll"));
		RuntimeDependencies.Add("$(BinaryOutputDir)/libssl-3-x64.dll",
			Path.Combine(ModuleDirectory, "lib", "libssl-3-x64.dll"));
		RuntimeDependencies.Add("$(BinaryOutputDir)/libpq.dll",
			Path.Combine(ModuleDirectory, "lib", "libpq.dll"));
		RuntimeDependencies.Add("$(BinaryOutputDir)/zlib1.dll",
			Path.Combine(ModuleDirectory, "lib", "zlib1.dll"));
	}
}
