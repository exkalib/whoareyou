using UnrealBuildTool;
using System.Collections.Generic;

public class WorldSimDemoTarget : TargetRules
{
    public WorldSimDemoTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("WorldSimDemo");
    }
}
