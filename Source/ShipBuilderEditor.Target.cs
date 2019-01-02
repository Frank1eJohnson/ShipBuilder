// Spaceship Builder - Gwennaël Arbona

using UnrealBuildTool;
using System.Collections.Generic;

public class ShipBuilderEditorTarget : TargetRules
{
	public ShipBuilderEditorTarget(TargetInfo Target) : base(Target)
    {
		DefaultBuildSettings = BuildSettingsVersion.V2;
		Type = TargetType.Editor;
        ExtraModuleNames.Add("ShipBuilder");
    }
}
