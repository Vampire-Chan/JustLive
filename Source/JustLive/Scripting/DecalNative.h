// DecalNative.h
// Decal native function registration

#pragma once

#include "CoreMinimal.h"
#include "ScriptVM.h"

class JUSTLIVE_API FDecalNativeReg
{
public:
    static void RegisterFunctions(FScriptVM* VM);

private:
    static FScriptValue SpawnDecal(FScriptVM* VM, const TArray<FScriptValue>& Args);
};
