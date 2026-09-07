using UnrealBuildTool;

public class SkylineRushEditorTarget : TargetRules
{
    public SkylineRushEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("SkylineRush");
    }
}
