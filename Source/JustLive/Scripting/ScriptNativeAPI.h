// ScriptNativeAPI.h
// Native API bridge for scripting system

#pragma once

#include "CoreMinimal.h"
#include "ScriptVM.h"

/**
 * Main Native API Registration Class
 */
class JUSTLIVE_API FScriptNativeAPI
{
public:
    /**
     * Register all native functions with the VM
     */
    static void RegisterNativeFunctions(FScriptVM* VM);

private:
    // Utility Functions
    static FScriptValue NativeLog(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue NativePrint(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue NativeSleep(FScriptVM* VM, const TArray<FScriptValue>& Args);

    // Script Management Functions
    static FScriptValue NativeLoadScript(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue NativeRunScript(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue NativeDoesScriptExist(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue NativeIsScriptRunning(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue NativeCanRunScript(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue NativeIsMissionScript(FScriptVM* VM, const TArray<FScriptValue>& Args);

    // Helper to get ScriptManager
    static class UScriptManager* GetScriptManager();
};