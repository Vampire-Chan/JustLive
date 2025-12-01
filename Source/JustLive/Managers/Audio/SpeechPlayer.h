// SpeechPlayer.h
// Dedicated Speech Player subsystem for character dialogue.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Components/AudioComponent.h"
#include "SpeechPlayer.generated.h"

/**
 * Handles character speech and dialogue.
 */
UCLASS()
class JUSTLIVE_API USpeechPlayer : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(class UAudioManager* InOwner);

    // ========================================================================
    // Playback
    // ========================================================================
    
    UFUNCTION(BlueprintCallable, Category = "Audio|Speech")
    int32 PlaySpeech(const FString& SoundId, FVector Location);

    UFUNCTION(BlueprintCallable, Category = "Audio|Speech")
    void StopSpeech(int32 Handle);

    UFUNCTION(BlueprintCallable, Category = "Audio|Speech")
    void StopAll();

    UFUNCTION(BlueprintCallable, Category = "Audio|Speech")
    bool IsSpeaking(int32 Handle) const;

private:
    UPROPERTY()
    class UAudioManager* OwnerManager;

    // Active speech lines
    UPROPERTY()
    TMap<int32, UAudioComponent*> ActiveSpeech;

    int32 NextHandleId = 1;
};
