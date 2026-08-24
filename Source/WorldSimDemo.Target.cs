using UnrealBuildTool;
using System.Collections.Generic;

public class WorldSimDemoTarget : TargetRules
{
    public WorldSimDemoTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.Add("WorldSimDemo");
    }
}
