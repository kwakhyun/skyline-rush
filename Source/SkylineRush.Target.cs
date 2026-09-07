using UnrealBuildTool;

public class SkylineRushTarget : TargetRules
{
    public SkylineRushTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("SkylineRush");
    }
}
