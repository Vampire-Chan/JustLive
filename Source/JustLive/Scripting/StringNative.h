// StringNative.h
// String native function registration

#pragma once

#include "CoreMinimal.h"
#include "ScriptVM.h"

class JUSTLIVE_API FStringNativeReg
{
public:
    static void RegisterFunctions(FScriptVM* VM);

private:
    static FScriptValue Len(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Substring(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Find(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue ToUpper(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue ToLower(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Replace(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Trim(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Split(FScriptVM* VM, const TArray<FScriptValue>& Args); // Returns List Handle
    static FScriptValue Contains(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue FromChar(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue ToChar(FScriptVM* VM, const TArray<FScriptValue>& Args);
};
