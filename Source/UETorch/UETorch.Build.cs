/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

using System.IO;
using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
	public class UETorch : ModuleRules
	{
		public UETorch(TargetInfo Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
					"UETorch/Public"
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"UETorch/Private"
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"InputCore",
					"RenderCore",
					"SlateCore",
					"Slate",
					// "ScriptGeneratorPlugin",
					"ScriptPlugin",
					// ... add other public dependencies that you statically link with here ...
				}
				);

			if (UEBuildConfiguration.bBuildEditor == true)
			{
				PublicDependencyModuleNames.AddRange(
					new string[]
					{
						"UnrealEd",
					}
				);
			}

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);

			// Get the engine path. Ends with "Engine/"
			string engine_path = Path.GetFullPath(BuildConfiguration.RelativeEnginePath);

			// now you can include the module's private paths!
			string script_plugin_src_path = Path.Combine(engine_path, "Plugins", "ScriptPlugin", "Source");
			PublicIncludePaths.Add(Path.Combine(script_plugin_src_path, "ScriptPlugin", "Private"));
			PublicIncludePaths.Add(Path.Combine(script_plugin_src_path, "ScriptPlugin", "Public"));
			PublicIncludePaths.Add(Path.Combine(script_plugin_src_path, "ScriptPlugin", "Classes"));
			PublicIncludePaths.Add(Path.Combine(script_plugin_src_path, "Lua", "install", "include"));

			// string scripts_path = Path.GetFullPath( "Scripts" );
			// Definitions.Add("UETORCH_SCRIPTS_DIR=" + scripts_path);
		}
	}
}
