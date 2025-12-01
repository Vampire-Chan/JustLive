// SpeechPlayer.cpp
// Dedicated Speech Player subsystem

#include "SpeechPlayer.h"
#include "../AudioManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void USpeechPlayer::Initialize(UAudioManager* InOwner)
{
    OwnerManager = InOwner;
}

int32 USpeechPlayer::PlaySpeech(const FString& SoundId, FVector Location)
{
    if (!OwnerManager) return -1;

    USoundBase* Sound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *SoundId));
    if (!Sound) return -1;

    // Speech typically has higher priority and distinct attenuation
    UAudioComponent* AC = UGameplayStatics::SpawnSoundAtLocation(OwnerManager, Sound, Location);
    
    if (AC)
    {
        int32 Handle = NextHandleId++;
        ActiveSpeech.Add(Handle, AC);
        
        return Handle;
    }
    return -1;
}

void USpeechPlayer::StopSpeech(int32 Handle)
{
    if (UAudioComponent** AC = ActiveSpeech.Find(Handle))
    {
        if (*AC && (*AC)->IsValidLowLevel())
        {
            (*AC)->Stop();
        }
        ActiveSpeech.Remove(Handle);
    }
}

void USpeechPlayer::StopAll()
{
    for (auto& Pair : ActiveSpeech)
    {
        if (Pair.Value && Pair.Value->IsValidLowLevel())
        {
            Pair.Value->Stop();
        }
    }
    ActiveSpeech.Empty();
}

bool USpeechPlayer::IsSpeaking(int32 Handle) const
{
    if (const UAudioComponent* const* AC = ActiveSpeech.Find(Handle))
    {
        return (*AC && (*AC)->IsValidLowLevel() && (*AC)->IsPlaying());
    }
    return false;
}
