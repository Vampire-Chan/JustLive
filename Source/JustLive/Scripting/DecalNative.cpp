// DecalNative.cpp
// Decal native function registration

#include "DecalNative.h"
#include "ScriptLogger.h"

void FDecalNativeReg::RegisterFunctions(FScriptVM* VM)
{
    if (!VM) return;

    SCRIPT_LOG(TEXT("[DECAL NATIVE REG] Registering decal functions..."));

    VM->RegisterNativeFunction(TEXT("Decal_Spawn"), SpawnDecal);

    SCRIPT_LOG(TEXT("[DECAL NATIVE REG] Registered decal functions"));
}

FScriptValue FDecalNativeReg::SpawnDecal(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    // Placeholder: DecalManager not implemented yet
    SCRIPT_LOG_WARNING(TEXT("Decal_Spawn not implemented (DecalManager missing)"));
    return FScriptValue::Bool(false);
}
