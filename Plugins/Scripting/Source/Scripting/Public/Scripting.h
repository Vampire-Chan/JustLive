// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Forward declarations
struct FBytecodeChunk;
class FScriptVM;

// Delegate for native API registration (startup scripts)
DECLARE_MULTICAST_DELEGATE_OneParam(FStartupScriptNativeAPIDelegate, FScriptVM*);

class FScriptingModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    
    /** Get the delegate for registering native APIs for startup scripts */
    FStartupScriptNativeAPIDelegate& GetStartupNativeAPIDelegate() { return OnRegisterStartupNativeAPI; }

private:
    /** Called after engine initialization to compile and load scripts */
    void OnPostEngineInit();
    
    /** Compile all scripts in Scripts/ root (including headers) */
    void CompileRootScripts();
    
    /** Load and execute scripts in Scripts/Startup/ folder */
    void LoadAndExecuteStartupScripts();
    
    /** Compile a script from source and save to Scripts/Compiled/ */
    TSharedPtr<FBytecodeChunk> CompileScript(const FString& SourceCode, const FString& ScriptName);
    
    /** Execute a startup script */
    void ExecuteStartupScript(TSharedPtr<FBytecodeChunk> Bytecode, const FString& ScriptName);
    
    /** Delegate for native API registration */
    FStartupScriptNativeAPIDelegate OnRegisterStartupNativeAPI;
};

