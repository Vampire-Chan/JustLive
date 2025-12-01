// SFXPlayer.cpp
// Dedicated SFX Player subsystem

#include "SFXPlayer.h"
#include "../AudioManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void USFXPlayer::Initialize(UAudioManager* InOwner)
{
    OwnerManager = InOwner;
}

void USFXPlayer::PlayOneShot2D(const FString& SoundId, float Volume, float Pitch)
{
    if (!OwnerManager) return;
    
    // Placeholder loading
    USoundBase* Sound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *SoundId));
    if (Sound)
    {
        UGameplayStatics::PlaySound2D(OwnerManager, Sound, Volume, Pitch);
    }
}

void USFXPlayer::PlayOneShotAtLocation(const FString& SoundId, FVector Location, float Volume, float Pitch)
{
    if (!OwnerManager) return;

    // Placeholder loading
    USoundBase* Sound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *SoundId));
    if (Sound)
    {
        UGameplayStatics::PlaySoundAtLocation(OwnerManager, Sound, Location, Volume, Pitch);
    }
}

int32 USFXPlayer::PlayLoopAtLocation(const FString& SoundId, FVector Location, float Volume)
{
    if (!OwnerManager) return -1;

    USoundBase* Sound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *SoundId));
    if (!Sound) return -1;

    UAudioComponent* AC = UGameplayStatics::SpawnSoundAtLocation(OwnerManager, Sound, Location, FRotator::ZeroRotator, Volume, 1.0f, 0.0f, nullptr, nullptr, true); // bAutoDestroy=true? No, for loop we want control
    
    if (AC)
    {
        AC->bAutoDestroy = false; // We manage it
        int32 Handle = NextHandleId++;
        ActiveLoops.Add(Handle, AC);
        return Handle;
    }
    return -1;
}

void USFXPlayer::StopLoop(int32 Handle)
{
    if (UAudioComponent** AC = ActiveLoops.Find(Handle))
    {
        if (*AC && (*AC)->IsValidLowLevel())
        {
            (*AC)->Stop();
            (*AC)->DestroyComponent();
        }
        ActiveLoops.Remove(Handle);
    }
}

void USFXPlayer::UpdateLoopLocation(int32 Handle, FVector NewLocation)
{
    if (UAudioComponent** AC = ActiveLoops.Find(Handle))
    {
        if (*AC && (*AC)->IsValidLowLevel())
        {
            (*AC)->SetWorldLocation(NewLocation);
        }
    }
}

void USFXPlayer::StopAll()
{
    for (auto& Pair : ActiveLoops)
    {
        if (Pair.Value && Pair.Value->IsValidLowLevel())
        {
            Pair.Value->Stop();
            Pair.Value->DestroyComponent();
        }
    }
    ActiveLoops.Empty();
}
