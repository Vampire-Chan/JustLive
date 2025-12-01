using UnrealBuildTool;
using System.IO;

public class Scripting : ModuleRules
{
    public Scripting(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
            );
                
        
        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
            );
            
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                // ... add other public dependencies that you statically link with here ...
            }
            );
            
        
    PrivateDependencyModuleNames.AddRange(
        new string[]
        {
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            // ... add private dependencies that you statically link with here ...    
        }
        );
    
    // Reference Sandbox module declarations without creating circular dependency
    PublicIncludePathModuleNames.AddRange(
        new string[]
        {
            // "Sandbox" 
        }
        );        
        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module should dynamically load at startup here ...
            }
            );
    }
}
