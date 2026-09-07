using UnrealBuildTool;

public class SkylineRush : ModuleRules
{
    public SkylineRush(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore", "UMG", "HTTP", "RenderCore", "RHI", "Json", "JsonUtilities" });
    }
}
