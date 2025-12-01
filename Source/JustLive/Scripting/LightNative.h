// LightNative.h
// Light native function registration

#pragma once

#include "CoreMinimal.h"
#include "ScriptVM.h"

class JUSTLIVE_API FLightNativeReg
{
public:
    static void RegisterFunctions(FScriptVM* VM);

private:
    static FScriptValue SetLightColor(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue SetLightIntensity(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue ToggleLight(FScriptVM* VM, const TArray<FScriptValue>& Args);
    
    // Helper to find light actor by name/tag
    static class ALight* FindLight(const FString& Name);
};
