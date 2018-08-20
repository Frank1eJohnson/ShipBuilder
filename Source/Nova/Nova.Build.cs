// Nova project - Gwennaël Arbona

using UnrealBuildTool;

public class Nova : ModuleRules
{
    public Nova(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
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
