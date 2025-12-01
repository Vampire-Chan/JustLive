// SFXPlayer.h
// Dedicated SFX Player subsystem for managing one-shots and looping effects.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Components/AudioComponent.h"
#include "SFXPlayer.generated.h"

/**
 * Handles sound effects (2D UI sounds, 3D World sounds).
 */
UCLASS()
class JUSTLIVE_API USFXPlayer : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(class UAudioManager* InOwner);

    // ========================================================================
    // Playback
    // ========================================================================
    
    // Fire and forget 2D sound
    UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
    void PlayOneShot2D(const FString& SoundId, float Volume = 1.0f, float Pitch = 1.0f);

    // Fire and forget 3D sound
    UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
    void PlayOneShotAtLocation(const FString& SoundId, FVector Location, float Volume = 1.0f, float Pitch = 1.0f);

    // Looping sound (returns handle for control)
    UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
    int32 PlayLoopAtLocation(const FString& SoundId, FVector Location, float Volume = 1.0f);

    // ========================================================================
    // Control
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
    void StopLoop(int32 Handle);

    UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
    void UpdateLoopLocation(int32 Handle, FVector NewLocation);

    UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
    void StopAll();

private:
    UPROPERTY()
    class UAudioManager* OwnerManager;

    // Active looping sounds
    UPROPERTY()
    TMap<int32, UAudioComponent*> ActiveLoops;

    int32 NextHandleId = 1;
};
