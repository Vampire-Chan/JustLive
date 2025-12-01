// AudioNative.h
// Audio native function registration

#pragma once

#include "CoreMinimal.h"
#include "ScriptVM.h"

class JUSTLIVE_API FAudioNativeReg
{
public:
    static void RegisterFunctions(FScriptVM* VM);

private:
    static FScriptValue PlaySound(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue PlayMusic(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue StopMusic(FScriptVM* VM, const TArray<FScriptValue>& Args);
    
    // Music Player Controls
    static FScriptValue Music_Next(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Music_Prev(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Music_Pause(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Music_Resume(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Music_SetVolume(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Music_SetShuffle(FScriptVM* VM, const TArray<FScriptValue>& Args);
    
    // SFX Player Controls
    static FScriptValue SFX_PlayLoop(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue SFX_StopLoop(FScriptVM* VM, const TArray<FScriptValue>& Args);
    
    static class UAudioManager* GetAudioManager();
};
