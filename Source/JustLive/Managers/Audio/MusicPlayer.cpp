// MusicPlayer.cpp
// Dedicated Music Player subsystem

#include "MusicPlayer.h"
#include "../AudioManager.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"

void UMusicPlayer::Initialize(UAudioManager* InOwner)
{
    OwnerManager = InOwner;
    // Create a persistent audio component
    if (OwnerManager)
    {
        // We create it, but don't attach it yet. It's a virtual 2D source by default.
        AudioComponent = UGameplayStatics::SpawnSound2D(OwnerManager, nullptr, 1.0f, 1.0f, 0.0f, nullptr, false, false);
        if (AudioComponent)
        {
            AudioComponent->OnAudioFinished.AddDynamic(this, &UMusicPlayer::OnTrackFinished);
            AudioComponent->bAutoDestroy = false; // Keep it alive
        }
    }
}

void UMusicPlayer::PlayTrack(const FString& SoundId)
{
    // Basic 2D Play
    AddToPlaylist(SoundId);
    CurrentTrackIndex = Playlist.Num() - 1;
    PlayCurrentIndex();
}

void UMusicPlayer::Stop()
{
    if (AudioComponent)
    {
        AudioComponent->Stop();
    }
}

void UMusicPlayer::Pause()
{
    if (AudioComponent)
    {
        AudioComponent->SetPaused(true);
    }
}

void UMusicPlayer::Resume()
{
    if (AudioComponent)
    {
        AudioComponent->SetPaused(false);
    }
}

void UMusicPlayer::Next()
{
    if (Playlist.Num() == 0) return;

    if (bShuffle)
    {
        CurrentTrackIndex = FMath::RandRange(0, Playlist.Num() - 1);
    }
    else
    {
        CurrentTrackIndex++;
        if (CurrentTrackIndex >= Playlist.Num())
        {
            if (LoopMode == EMusicLoopMode::Playlist)
            {
                CurrentTrackIndex = 0;
            }
            else
            {
                Stop();
                return;
            }
        }
    }
    PlayCurrentIndex();
}

void UMusicPlayer::Previous()
{
    if (Playlist.Num() == 0) return;

    CurrentTrackIndex--;
    if (CurrentTrackIndex < 0)
    {
        if (LoopMode == EMusicLoopMode::Playlist)
        {
            CurrentTrackIndex = Playlist.Num() - 1;
        }
        else
        {
            CurrentTrackIndex = 0; // Clamp to start
        }
    }
    PlayCurrentIndex();
}

void UMusicPlayer::SetVolume(float Volume)
{
    if (AudioComponent)
    {
        AudioComponent->SetVolumeMultiplier(Volume);
    }
}

void UMusicPlayer::SetLoopMode(EMusicLoopMode Mode)
{
    LoopMode = Mode;
}

void UMusicPlayer::SetShuffle(bool bEnable)
{
    bShuffle = bEnable;
}

void UMusicPlayer::AddToPlaylist(const FString& SoundId)
{
    Playlist.Add(SoundId);
}

void UMusicPlayer::ClearPlaylist()
{
    Playlist.Empty();
    CurrentTrackIndex = -1;
    Stop();
}

void UMusicPlayer::PlayCurrentIndex()
{
    if (!AudioComponent || !Playlist.IsValidIndex(CurrentTrackIndex)) return;

    // TODO: Resolve SoundId to USoundBase using DataManager/AudioManager lookup
    // For now, assuming SoundId is a path or we have a resolver
    // USoundBase* Sound = OwnerManager->GetSoundFromId(Playlist[CurrentTrackIndex]);
    
    // Placeholder loading mechanism
    FString Path = Playlist[CurrentTrackIndex];
    USoundBase* Sound = Cast<USoundBase>(StaticLoadObject(USoundBase::StaticClass(), nullptr, *Path));

    if (Sound)
    {
        AudioComponent->SetSound(Sound);
        AudioComponent->Play();
    }
}

void UMusicPlayer::OnTrackFinished()
{
    if (LoopMode == EMusicLoopMode::SingleTrack)
    {
        PlayCurrentIndex(); // Replay same
    }
    else
    {
        Next(); // Auto advance
    }
}
