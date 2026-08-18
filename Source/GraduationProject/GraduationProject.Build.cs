using UnrealBuildTool;

public class GraduationProject : ModuleRules
{
	public GraduationProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"HeadMountedDisplay",
			"InputCore",
			"EnhancedInput"
		});
	}
}
