// MusicPlayer.h
// Dedicated Music Player subsystem for managing tracks, playlists, and playback states.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Components/AudioComponent.h"
#include "MusicPlayer.generated.h"

UENUM(BlueprintType)
enum class EMusicLoopMode : uint8
{
    None,
    SingleTrack,
    Playlist
};

/**
 * Handles background music, radios, and playlist management.
 */
UCLASS()
class JUSTLIVE_API UMusicPlayer : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(class UAudioManager* InOwner);

    // ========================================================================
    // Playback Control
    // ========================================================================
    
    UFUNCTION(BlueprintCallable, Category = "Audio|Music")
    void PlayTrack(const FString& SoundId);

    UFUNCTION(BlueprintCallable, Category = "Audio|Music")
    void Stop();

    UFUNCTION(BlueprintCallable, Category = "Audio|Music")
    void Pause();

    UFUNCTION(BlueprintCallable, Category = "Audio|Music")
    void Resume();

    UFUNCTION(BlueprintCallable, Category = "Audio|Music")
    void Next();

    UFUNCTION(BlueprintCallable, Category = "Audio|Music")
    void Previous();

    // ========================================================================
    // Settings
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Audio|Music")
    void SetVolume(float Volume);

    UFUNCTION(BlueprintCallable, Category = "Audio|Music")
    void SetLoopMode(EMusicLoopMode Mode);

    UFUNCTION(BlueprintCallable, Category = "Audio|Music")
    void SetShuffle(bool bEnable);

    // ========================================================================
    // Playlist Management
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Audio|Music")
    void AddToPlaylist(const FString& SoundId);

    UFUNCTION(BlueprintCallable, Category = "Audio|Music")
    void ClearPlaylist();

private:
    void PlayCurrentIndex();
    void OnTrackFinished();

    UPROPERTY()
    class UAudioManager* OwnerManager;

    UPROPERTY()
    UAudioComponent* AudioComponent;

    // State
    UPROPERTY()
    TArray<FString> Playlist;

    UPROPERTY()
    int32 CurrentTrackIndex = -1;

    UPROPERTY()
    EMusicLoopMode LoopMode = EMusicLoopMode::None;

    UPROPERTY()
    bool bShuffle = false;

    UPROPERTY()
    TArray<int32> ShuffleHistory; // To track history for "Previous" functionality
};
