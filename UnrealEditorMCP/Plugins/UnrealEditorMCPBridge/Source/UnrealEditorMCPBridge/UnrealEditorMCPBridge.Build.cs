using UnrealBuildTool;

public class UnrealEditorMCPBridge : ModuleRules
{
	public UnrealEditorMCPBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Slate",
			"SlateCore",
			"Sockets",
			"Networking",
			"Json",
			"JsonUtilities",
			"Projects",
			"LevelEditor",
			"PythonScriptPlugin",
			"AssetRegistry",
			"Kismet",
			"BlueprintGraph",
			"UMG",
			"UMGEditor",
			"MaterialEditor"
		});
	}
}
