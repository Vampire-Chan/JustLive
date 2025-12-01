// Copyright Vampire Game Project. All Rights Reserved.
// Native API bridge implementation

#include "ScriptNativeAPI.h"
#include "ScriptLogger.h"
#include "ScriptLatentManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "ScriptManager.h"

// Include module registration headers
#include "MathNative.h"
#include "CollectionNative.h"
#include "StringNative.h"
#include "AudioNative.h"
#include "UINative.h"
#include "LightNative.h"
#include "DecalNative.h"

void FScriptNativeAPI::RegisterNativeFunctions(FScriptVM* VM)
{
    if (!VM)
    {
        SCRIPT_LOG_ERROR(TEXT("Cannot register native functions - VM is null"));
        return;
    }

    SCRIPT_LOG(TEXT("[NATIVE API] Registering utility functions..."));
    // Utility functions
    VM->RegisterNativeFunction(TEXT("Log"), NativeLog);
    VM->RegisterNativeFunction(TEXT("Print"), NativePrint);
    VM->RegisterNativeFunction(TEXT("Sleep"), NativeSleep);

    // Script Management functions
    VM->RegisterNativeFunction(TEXT("LoadScript"), NativeLoadScript);
    VM->RegisterNativeFunction(TEXT("RunScript"), NativeRunScript);
    VM->RegisterNativeFunction(TEXT("DoesScriptExist"), NativeDoesScriptExist);
    VM->RegisterNativeFunction(TEXT("IsScriptRunning"), NativeIsScriptRunning);
    VM->RegisterNativeFunction(TEXT("CanRunScript"), NativeCanRunScript);
    VM->RegisterNativeFunction(TEXT("IsMissionScript"), NativeIsMissionScript);

    // Register module functions
    FMathNativeReg::RegisterFunctions(VM);
    FScriptCollectionManager::RegisterFunctions(VM);
    FStringNativeReg::RegisterFunctions(VM);
    
    // Register Game System modules
    FAudioNativeReg::RegisterFunctions(VM);
    FUINativeReg::RegisterFunctions(VM);
    FLightNativeReg::RegisterFunctions(VM);
    FDecalNativeReg::RegisterFunctions(VM);
    
    SCRIPT_LOG(TEXT("[NATIVE API] Registered all native API functions successfully"));
}

//=============================================================================
// Utility Functions 
//=============================================================================

FScriptValue FScriptNativeAPI::NativeLog(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() > 0)
    {
        SCRIPT_LOG(FString::Printf(TEXT("[SCRIPT] %s"), *Args[0].ToString()));
    }
    return FScriptValue::Nil();
}

FScriptValue FScriptNativeAPI::NativePrint(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() > 0)
    {
        SCRIPT_LOG(FString::Printf(TEXT("[SCRIPT] %s"), *Args[0].ToString()));
    }
    return FScriptValue::Nil();
}

FScriptValue FScriptNativeAPI::NativeSleep(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber())
    {
        SCRIPT_LOG_ERROR(TEXT("[SCRIPT API] sleep requires 1 number argument (seconds)"));
        return FScriptValue::Nil();
    }
    
    float SleepDuration = (float)Args[0].AsNumber();
    if (SleepDuration <= 0.0f) return FScriptValue::Nil();

    UWorld* World = nullptr;
    if (GEngine)
    {
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
            {
                World = Context.World();
                break;
            }
        }
    }

    if (!World)
    {
        SCRIPT_LOG_ERROR(TEXT("[SCRIPT API] Cannot sleep: No Game World found"));
        return FScriptValue::Nil();
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance) return FScriptValue::Nil();

    UScriptLatentManager* LatentManager = GameInstance->GetSubsystem<UScriptLatentManager>();
    if (LatentManager)
    {
        LatentManager->RequestSleep(VM->AsShared(), SleepDuration);
    }
    else
    {
        SCRIPT_LOG_ERROR(TEXT("[SCRIPT API] ScriptLatentManager subsystem not found"));
    }
    
    return FScriptValue::Nil();
}

//=============================================================================
// Script Management Functions
//=============================================================================

FScriptValue FScriptNativeAPI::NativeLoadScript(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    
    FString Path = Args[0].AsString();
    UScriptManager* SM = GetScriptManager();
    if (SM)
    {
        FString Result = SM->LoadScript(Path);
        return FScriptValue::Bool(!Result.IsEmpty());
    }
    return FScriptValue::Bool(false);
}

FScriptValue FScriptNativeAPI::NativeRunScript(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    
    FString Name = Args[0].AsString();
    UScriptManager* SM = GetScriptManager();
    if (SM)
    {
        // If not loaded, attempt to load first
        if (!SM->IsScriptLoaded(Name))
        {
            SM->LoadScript(Name);
        }
        return FScriptValue::Bool(SM->ExecuteScript(Name));
    }
    return FScriptValue::Bool(false);
}

FScriptValue FScriptNativeAPI::NativeDoesScriptExist(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    FString Name = Args[0].AsString();
    
    // Check if file exists in scripts folder
    FString ScriptPath = FPaths::ProjectDir() / TEXT("Scripts") / Name + TEXT(".sc");
    FString CompiledPath = FPaths::ProjectDir() / TEXT("Scripts/Compiled") / Name + TEXT(".scc");
    
    return FScriptValue::Bool(FPaths::FileExists(ScriptPath) || FPaths::FileExists(CompiledPath));
}

FScriptValue FScriptNativeAPI::NativeIsScriptRunning(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    FString Name = Args[0].AsString();
    
    UScriptManager* SM = GetScriptManager();
    if (SM)
    {
        return FScriptValue::Bool(SM->IsScriptRunning(Name));
    }
    return FScriptValue::Bool(false);
}

FScriptValue FScriptNativeAPI::NativeCanRunScript(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    FString Name = Args[0].AsString();

    UScriptManager* SM = GetScriptManager();
    if (SM)
    {
        // If not loaded, we can try to load it
        if (!SM->IsScriptLoaded(Name))
        {
            // Try loading to check existence and compilability
            FString LoadedName = SM->LoadScript(Name);
            if (LoadedName.IsEmpty())
            {
                return FScriptValue::Bool(false); // Cannot load/compile
            }
        }
        // Script is loaded, now check if it's currently running, and if there are other conditions (e.g., resource pooling)
        // TODO: Add more sophisticated checks here (e.g., maximum concurrent scripts, mission script flags)
        return FScriptValue::Bool(!SM->IsScriptRunning(Name)); // Can run if not already running
    }
    return FScriptValue::Bool(false); 
}

FScriptValue FScriptNativeAPI::NativeIsMissionScript(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    FString Name = Args[0].AsString();

    UScriptManager* SM = GetScriptManager();
    if (SM)
    {
        const TMap<FString, FCompiledScript>& LoadedScriptsMap = SM->GetLoadedScriptsMap();
        const FCompiledScript* Script = LoadedScriptsMap.Find(Name); // Find returns a const pointer, or nullptr if not found
        if (Script && Script->Bytecode.IsValid())
        {
            return FScriptValue::Bool(Script->Bytecode->Metadata.bIsMission);
        }
    }
    return FScriptValue::Bool(false);
}

UScriptManager* FScriptNativeAPI::GetScriptManager()
{
    if (GEngine)
    {
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
            {
                if (UWorld* World = Context.World())
                {
                    if (UGameInstance* GI = World->GetGameInstance())
                    {
                        return GI->GetSubsystem<UScriptManager>();
                    }
                }
            }
        }
    }
    return nullptr;
}