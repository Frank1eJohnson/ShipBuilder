// Spaceship Builder - Gwennaël Arbona

using UnrealBuildTool;
using System.Collections.Generic;

public class ShipBuilderTarget : TargetRules
{
    public ShipBuilderTarget(TargetInfo Target) : base(Target)
    {
		DefaultBuildSettings = BuildSettingsVersion.V2;
        Type = TargetType.Game;
		bUsesOpen Source = true;
        ExtraModuleNames.Add("ShipBuilder");
    }
}
