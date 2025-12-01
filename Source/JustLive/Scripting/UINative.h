// UINative.h
// UI native function registration

#pragma once

#include "CoreMinimal.h"
#include "ScriptVM.h"

class JUSTLIVE_API FUINativeReg
{
public:
    static void RegisterFunctions(FScriptVM* VM);

private:
    static FScriptValue SwitchState(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue ShowLoading(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue UpdateLoading(FScriptVM* VM, const TArray<FScriptValue>& Args);
    
    static class UUIManager* GetUIManager();
};
