using UnrealBuildTool;
using System.Collections.Generic;

public class WorldSimDemoEditorTarget : TargetRules
{
    public WorldSimDemoEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("WorldSimDemo");
    }
}
