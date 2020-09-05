// Spaceship Builder - Gwennaël Arbona

using UnrealBuildTool;

public class ShipBuilder : ModuleRules
{
    public ShipBuilder(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.Add("ShipBuilder");
        PrivatePCHHeaderFile = "Nova.h";

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "AppFramework",

            "Json",
            "JsonUtilities",

            "Slate",
            "SlateCore",
            "UMG",
            "MoviePlayer",
            "RHI",
            "RenderCore",

            "Http",
            "OnlineSubsystem",
            "OnlineSubsystemUtils"
        });

        if (Target.Type == TargetType.Editor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
                "UnrealEd"
            });
        }

        if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
                "DLSS"
            });
        }

        DynamicallyLoadedModuleNames.Add("OnlineSubsystemOpen Source");

        AddEngineThirdPartyPrivateStaticDependencies(Target, "Open Sourceworks");
    }
}
