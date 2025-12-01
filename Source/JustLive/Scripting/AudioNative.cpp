// AudioNative.cpp
// Audio native function registration

#include "AudioNative.h"
#include "ScriptLogger.h"
#include "../Managers/AudioManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MathNative.h" // For vector helper if needed, but we can do manual extraction

void FAudioNativeReg::RegisterFunctions(FScriptVM* VM)
{
    if (!VM) return;

    SCRIPT_LOG(TEXT("[AUDIO NATIVE REG] Registering audio functions..."));

    VM->RegisterNativeFunction(TEXT("Audio_PlaySound"), PlaySound);
    VM->RegisterNativeFunction(TEXT("Audio_PlayMusic"), PlayMusic);
    VM->RegisterNativeFunction(TEXT("Audio_StopMusic"), StopMusic);
    
    // Music
    VM->RegisterNativeFunction(TEXT("Music_Next"), Music_Next);
    VM->RegisterNativeFunction(TEXT("Music_Prev"), Music_Prev);
    VM->RegisterNativeFunction(TEXT("Music_Pause"), Music_Pause);
    VM->RegisterNativeFunction(TEXT("Music_Resume"), Music_Resume);
    VM->RegisterNativeFunction(TEXT("Music_SetVolume"), Music_SetVolume);
    VM->RegisterNativeFunction(TEXT("Music_SetShuffle"), Music_SetShuffle);
    
    // SFX
    VM->RegisterNativeFunction(TEXT("SFX_PlayLoop"), SFX_PlayLoop);
    VM->RegisterNativeFunction(TEXT("SFX_StopLoop"), SFX_StopLoop);

    SCRIPT_LOG(TEXT("[AUDIO NATIVE REG] Registered audio functions"));
}

FScriptValue FAudioNativeReg::PlaySound(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    
    FString SoundId = Args[0].ToString();
    FVector Location = FVector::ZeroVector;
    
    // Optional location argument (Array [x,y,z])
    if (Args.Num() > 1 && Args[1].IsArray() && Args[1].ArrayValue.Num() >= 3)
    {
        const TArray<FScriptValue>& Vec = Args[1].ArrayValue;
        Location.X = Vec[0].AsNumber();
        Location.Y = Vec[1].AsNumber();
        Location.Z = Vec[2].AsNumber();
    }
    
    UAudioManager* AM = GetAudioManager();
    if (AM)
    {
        AM->PlaySound(SoundId, Location);
        return FScriptValue::Bool(true);
    }
    
    return FScriptValue::Bool(false);
}

FScriptValue FAudioNativeReg::PlayMusic(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    
    FString Path = Args[0].ToString();
    
    UAudioManager* AM = GetAudioManager();
    if (AM)
    {
        AM->PlayBackgroundMusic(Path);
        return FScriptValue::Bool(true);
    }
    
    return FScriptValue::Bool(false);
}

FScriptValue FAudioNativeReg::StopMusic(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    UAudioManager* AM = GetAudioManager();
    if (AM)
    {
        AM->StopBackgroundMusic();
        return FScriptValue::Bool(true);
    }
    
    return FScriptValue::Bool(false);
}

// ============================================================================
// Music Player
// ============================================================================

FScriptValue FAudioNativeReg::Music_Next(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (UAudioManager* AM = GetAudioManager())
    {
        if (AM->GetMusicPlayer()) AM->GetMusicPlayer()->Next();
        return FScriptValue::Bool(true);
    }
    return FScriptValue::Bool(false);
}

FScriptValue FAudioNativeReg::Music_Prev(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (UAudioManager* AM = GetAudioManager())
    {
        if (AM->GetMusicPlayer()) AM->GetMusicPlayer()->Previous();
        return FScriptValue::Bool(true);
    }
    return FScriptValue::Bool(false);
}

FScriptValue FAudioNativeReg::Music_Pause(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (UAudioManager* AM = GetAudioManager())
    {
        if (AM->GetMusicPlayer()) AM->GetMusicPlayer()->Pause();
        return FScriptValue::Bool(true);
    }
    return FScriptValue::Bool(false);
}

FScriptValue FAudioNativeReg::Music_Resume(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (UAudioManager* AM = GetAudioManager())
    {
        if (AM->GetMusicPlayer()) AM->GetMusicPlayer()->Resume();
        return FScriptValue::Bool(true);
    }
    return FScriptValue::Bool(false);
}

FScriptValue FAudioNativeReg::Music_SetVolume(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    float Vol = (float)Args[0].AsNumber();
    
    if (UAudioManager* AM = GetAudioManager())
    {
        if (AM->GetMusicPlayer()) AM->GetMusicPlayer()->SetVolume(Vol);
        return FScriptValue::Bool(true);
    }
    return FScriptValue::Bool(false);
}

FScriptValue FAudioNativeReg::Music_SetShuffle(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    bool bShuffle = Args[0].AsBool();
    
    if (UAudioManager* AM = GetAudioManager())
    {
        if (AM->GetMusicPlayer()) AM->GetMusicPlayer()->SetShuffle(bShuffle);
        return FScriptValue::Bool(true);
    }
    return FScriptValue::Bool(false);
}

// ============================================================================
// SFX Player
// ============================================================================

FScriptValue FAudioNativeReg::SFX_PlayLoop(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Number(-1);
    FString SoundId = Args[0].ToString();
    
    FVector Location = FVector::ZeroVector;
    if (Args[1].IsArray() && Args[1].ArrayValue.Num() >= 3)
    {
        const TArray<FScriptValue>& Vec = Args[1].ArrayValue;
        Location.X = Vec[0].AsNumber();
        Location.Y = Vec[1].AsNumber();
        Location.Z = Vec[2].AsNumber();
    }
    
    float Vol = (Args.Num() > 2) ? (float)Args[2].AsNumber() : 1.0f;
    
    if (UAudioManager* AM = GetAudioManager())
    {
        if (AM->GetSFXPlayer())
        {
            return FScriptValue::Number(AM->GetSFXPlayer()->PlayLoopAtLocation(SoundId, Location, Vol));
        }
    }
    return FScriptValue::Number(-1);
}

FScriptValue FAudioNativeReg::SFX_StopLoop(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    int32 Handle = (int32)Args[0].AsNumber();
    
    if (UAudioManager* AM = GetAudioManager())
    {
        if (AM->GetSFXPlayer())
        {
            AM->GetSFXPlayer()->StopLoop(Handle);
            return FScriptValue::Bool(true);
        }
    }
    return FScriptValue::Bool(false);
}

UAudioManager* FAudioNativeReg::GetAudioManager()
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
                        return GI->GetSubsystem<UAudioManager>();
                    }
                }
            }
        }
    }
    return nullptr;
}
