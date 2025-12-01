#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Sound/SoundWave.h"
#include "MetasoundSource.h"
#include "Sound/SoundAttenuation.h"
#include "Audio/AudioTypes.h"
#include "Audio/MusicPlayer.h"
#include "Audio/SFXPlayer.h"
#include "Audio/SpeechPlayer.h"
#include "AudioManager.generated.h"

UCLASS()
class JUSTLIVE_API UAudioManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UAudioManager* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Accessors for new players
	UFUNCTION(BlueprintCallable, Category = "Audio")
	UMusicPlayer* GetMusicPlayer() const { return MusicPlayer; }

	UFUNCTION(BlueprintCallable, Category = "Audio")
	USFXPlayer* GetSFXPlayer() const { return SFXPlayer; }

	UFUNCTION(BlueprintCallable, Category = "Audio")
	USpeechPlayer* GetSpeechPlayer() const { return SpeechPlayer; }

	/**
	 * Loads sound definitions from an XML file.
	 * @param VirtualPath - e.g., "@data/sounds.xml"
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void LoadSoundDefinitions(const FString& VirtualPath);

	/**
	 * Plays a sound by ID.
	 * @param SoundId - ID defined in XML
	 * @param Location - World location for spatial sounds
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlaySound(const FString& SoundId, FVector Location = FVector::ZeroVector);

	/**
	 * Temporary function to play test music.
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayTestMusic();

	/**
	 * Plays a 2D background music track (stops previous).
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayBackgroundMusic(const FString& Path);

	/**
	 * Stops the currently playing background music.
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StopBackgroundMusic();

	/**
	 * Spawns a 3D spatial sound at the specified location.
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SpawnProximityMusic(const FString& Path, FVector Location);

	UFUNCTION()
	void OnSoundFinished();

private:
	TMap<FString, FSoundDefinition> SoundDefs;

	// Sub-Managers
	UPROPERTY()
	UMusicPlayer* MusicPlayer;

	UPROPERTY()
	USFXPlayer* SFXPlayer;

	UPROPERTY()
	USpeechPlayer* SpeechPlayer;

	UPROPERTY()
	UAudioComponent* CurrentMusicComponent;

	// Core Assets
	UPROPERTY()
	UMetaSoundSource* MS_Spatial;

	UPROPERTY()
	UMetaSoundSource* MS_NonSpatial;

	UPROPERTY()
	USoundAttenuation* S_Attenuation;

	class UDataManager* GetDataManager() const;
	USoundWave* LoadSoundWave(const FString& Path);
};
