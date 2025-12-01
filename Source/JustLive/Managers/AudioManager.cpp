#include "AudioManager.h"
#include "../Core/DataManager.h"
#include "XmlFile.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "MetasoundSource.h"
#include "Sound/SoundAttenuation.h"
#include "Audio/MusicPlayer.h"
#include "Audio/SFXPlayer.h"
#include "Audio/SpeechPlayer.h"

UAudioManager* UAudioManager::Get(const UObject* WorldContextObject)
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		return GI->GetSubsystem<UAudioManager>();
	}
	return nullptr;
}

void UAudioManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency(UDataManager::StaticClass());

	// Initialize Sub-Players
	MusicPlayer = NewObject<UMusicPlayer>(this);
	MusicPlayer->Initialize(this);

	SFXPlayer = NewObject<USFXPlayer>(this);
	SFXPlayer->Initialize(this);

	SpeechPlayer = NewObject<USpeechPlayer>(this);
	SpeechPlayer->Initialize(this);

	// Load Core Assets
	// Paths found: Content/Templates/MS_*.uasset and Content/Attenuation/S_Template.uasset
	MS_Spatial = LoadObject<UMetaSoundSource>(nullptr, TEXT("/Game/Templates/MS_Spatial.MS_Spatial"));
	MS_NonSpatial = LoadObject<UMetaSoundSource>(nullptr, TEXT("/Game/Templates/MS_NonSpatial.MS_NonSpatial"));
	S_Attenuation = LoadObject<USoundAttenuation>(nullptr, TEXT("/Game/Attenuation/S_Template.S_Template"));

	if (!MS_Spatial) UE_LOG(LogTemp, Error, TEXT("AudioManager: Failed to load MS_Spatial!"));
	if (!MS_NonSpatial) UE_LOG(LogTemp, Error, TEXT("AudioManager: Failed to load MS_NonSpatial!"));
	if (!S_Attenuation) UE_LOG(LogTemp, Error, TEXT("AudioManager: Failed to load S_Template!"));

	UE_LOG(LogTemp, Log, TEXT("AudioManager Initialized."));
}

void UAudioManager::Deinitialize()
{
	if (MusicPlayer) MusicPlayer->Stop();
	if (SFXPlayer) SFXPlayer->StopAll();
	if (SpeechPlayer) SpeechPlayer->StopAll();

	Super::Deinitialize();
}

UDataManager* UAudioManager::GetDataManager() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UDataManager>();
	}
	return nullptr;
}

void UAudioManager::LoadSoundDefinitions(const FString& VirtualPath)
{
	UDataManager* DataMgr = GetDataManager();
	if (!DataMgr) return;

	TSharedPtr<FXmlFile> Xml = DataMgr->ParseXML(VirtualPath);
	if (!Xml.IsValid() || !Xml->GetRootNode()) return;

	FXmlNode* Root = Xml->GetRootNode();
	for (const auto& Node : Root->GetChildrenNodes())
	{
		if (Node->GetTag().Equals(TEXT("Sound")))
		{
			FSoundDefinition Def;
			Def.Id = Node->GetAttribute(TEXT("id"));
			Def.FilePath = Node->GetAttribute(TEXT("path"));
			
			FString VolStr = Node->GetAttribute(TEXT("volume"));
			if (!VolStr.IsEmpty()) Def.Volume = FCString::Atof(*VolStr);

			FString PitchStr = Node->GetAttribute(TEXT("pitch"));
			if (!PitchStr.IsEmpty()) Def.Pitch = FCString::Atof(*PitchStr);

			FString PanStr = Node->GetAttribute(TEXT("pan"));
			if (!PanStr.IsEmpty()) Def.Pan = FCString::Atof(*PanStr);

			FString StartStr = Node->GetAttribute(TEXT("start_time"));
			if (!StartStr.IsEmpty()) Def.StartTime = FCString::Atof(*StartStr);

			FString PlayBackStr = Node->GetAttribute(TEXT("playback_time"));
			if (!PlayBackStr.IsEmpty()) Def.PlayBackTime = FCString::Atof(*PlayBackStr);

			FString SpatialStr = Node->GetAttribute(TEXT("spatial"));
			if (!SpatialStr.IsEmpty()) Def.bSpatial = SpatialStr.ToBool();

			FString RadiusStr = Node->GetAttribute(TEXT("radius"));
			if (!RadiusStr.IsEmpty()) Def.AttenuationRadius = FCString::Atof(*RadiusStr);

			FString FalloffStr = Node->GetAttribute(TEXT("falloff"));
			if (!FalloffStr.IsEmpty()) Def.FalloffDistance = FCString::Atof(*FalloffStr);

			SoundDefs.Add(Def.Id, Def);
			UE_LOG(LogTemp, Log, TEXT("AudioManager: Registered sound '%s'"), *Def.Id);
		}
	}
}

void UAudioManager::PlaySound(const FString& SoundId, FVector Location)
{
	if (!SoundDefs.Contains(SoundId))
	{
		UE_LOG(LogTemp, Warning, TEXT("AudioManager: Sound ID '%s' not found"), *SoundId);
		return;
	}

	const FSoundDefinition& Def = SoundDefs[SoundId];
	UMetaSoundSource* MetaSoundToPlay = Def.bSpatial ? MS_Spatial : MS_NonSpatial;

	if (!MetaSoundToPlay) return;

	// Load the actual Wave
	USoundWave* Wave = LoadSoundWave(Def.FilePath);
	if (!Wave)
	{
		UE_LOG(LogTemp, Warning, TEXT("AudioManager: Failed to load wave for '%s'"), *SoundId);
		return;
	}

	// Spawn Audio Component
	if (UWorld* World = GetWorld())
	{
		// Clamp Values
		float ClampedVolume = FMath::Clamp(Def.Volume, 0.0f, 1.0f);
		float ClampedPitch = FMath::Clamp(Def.Pitch, -10.0f, 10.0f);
		float ClampedPan = FMath::Clamp(Def.Pan, -1.0f, 1.0f);

		// Note: We use SpawnSoundAttached or SpawnSoundAtLocation. 
		// For 2D sounds (NonSpatial), Location is ignored usually, but Pan works.
		
		UAudioComponent* AudioComp = UGameplayStatics::SpawnSoundAtLocation(
			World, 
			MetaSoundToPlay, 
			Location, 
			FRotator::ZeroRotator, 
			ClampedVolume, 
			1.0f, // Pitch Multiplier (We use the Parameter instead)
			0.0f, 
			S_Attenuation // Use our template attenuation
		);

		if (AudioComp)
		{
			// Set MetaSound Parameters
			AudioComp->SetWaveParameter(FName("SoundWave"), Wave);
			AudioComp->SetFloatParameter(FName("Volume"), ClampedVolume);
			AudioComp->SetFloatParameter(FName("Pitch"), ClampedPitch);
			AudioComp->SetFloatParameter(FName("Pan"), ClampedPan);
			AudioComp->SetFloatParameter(FName("StartTime"), Def.StartTime);

			// Bind OnFinished
			AudioComp->OnAudioFinished.AddDynamic(this, &UAudioManager::OnSoundFinished);

			// Handle PlayBackTime (Duration)
			if (Def.PlayBackTime > 0.0f)
			{
				FTimerHandle TimerHandle;
				World->GetTimerManager().SetTimer(
					TimerHandle, 
					FTimerDelegate::CreateWeakLambda(AudioComp, [AudioComp]() {
						if (AudioComp && AudioComp->IsValidLowLevel())
						{
							AudioComp->Stop();
						}
					}), 
					Def.PlayBackTime, 
					false
				);
			}

			// Override Attenuation if Spatial
			if (Def.bSpatial && S_Attenuation)
			{
				AudioComp->bOverrideAttenuation = true;
				AudioComp->AttenuationOverrides = S_Attenuation->Attenuation;
				AudioComp->AttenuationOverrides.FalloffDistance = Def.FalloffDistance;
				AudioComp->AttenuationOverrides.AttenuationShapeExtents = FVector(Def.AttenuationRadius, 0, 0);
			}
		}
	}
}

void UAudioManager::PlayTestMusic()
{
	// User requested slight up-pitch for testing
	PlayBackgroundMusic(TEXT("/Game/Assets/Frontend/Menu/Drum_Alanwalker.Drum_Alanwalker"));
	
	if (CurrentMusicComponent)
	{
		CurrentMusicComponent->SetFloatParameter(FName("Pitch"), 1.1f);
	}
}

void UAudioManager::PlayBackgroundMusic(const FString& Path)
{
	StopBackgroundMusic();

	if (!MS_NonSpatial) return;

	USoundWave* Wave = LoadSoundWave(Path);
	if (!Wave)
	{
		UE_LOG(LogTemp, Warning, TEXT("AudioManager: Failed to load background music at '%s'"), *Path);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		CurrentMusicComponent = UGameplayStatics::SpawnSound2D(World, MS_NonSpatial);
		if (CurrentMusicComponent)
		{
			// Clamp Values
			float ClampedVolume = FMath::Clamp(0.5f, 0.0f, 1.0f); // Default 0.5
			float ClampedPitch = FMath::Clamp(1.0f, -10.0f, 10.0f);

			CurrentMusicComponent->SetWaveParameter(FName("SoundWave"), Wave);
			CurrentMusicComponent->SetFloatParameter(FName("Volume"), ClampedVolume);
			CurrentMusicComponent->SetFloatParameter(FName("Pitch"), ClampedPitch);
			UE_LOG(LogTemp, Log, TEXT("AudioManager: Playing Background Music '%s'"), *Path);
		}
	}
}

void UAudioManager::StopBackgroundMusic()
{
	if (CurrentMusicComponent)
	{
		CurrentMusicComponent->Stop();
		CurrentMusicComponent = nullptr;
	}
}

void UAudioManager::SpawnProximityMusic(const FString& Path, FVector Location)
{
	if (!MS_Spatial || !S_Attenuation) return;

	USoundWave* Wave = LoadSoundWave(Path);
	if (!Wave)
	{
		UE_LOG(LogTemp, Warning, TEXT("AudioManager: Failed to load proximity music at '%s'"), *Path);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		UAudioComponent* AudioComp = UGameplayStatics::SpawnSoundAtLocation(
			World, 
			MS_Spatial, 
			Location, 
			FRotator::ZeroRotator, 
			1.0f, 
			1.0f, 
			0.0f, 
			S_Attenuation
		);

		if (AudioComp)
		{
			AudioComp->SetWaveParameter(FName("SoundWave"), Wave);
			AudioComp->SetFloatParameter(FName("Volume"), 1.0f);
			AudioComp->SetFloatParameter(FName("Pitch"), 1.0f);
			
			// Ensure Attenuation is active
			AudioComp->bOverrideAttenuation = true;
			AudioComp->AttenuationOverrides = S_Attenuation->Attenuation;
			
			UE_LOG(LogTemp, Log, TEXT("AudioManager: Spawned Proximity Music at %s"), *Location.ToString());
		}
	}
}

USoundWave* UAudioManager::LoadSoundWave(const FString& Path)
{
	// TODO: Implement runtime .wav loading (e.g. using RuntimeAudioImporter or similar)
	// For now, we assume Path is a package path to a USoundWave asset
	return LoadObject<USoundWave>(nullptr, *Path);
}

void UAudioManager::OnSoundFinished()
{
	UE_LOG(LogTemp, Log, TEXT("AudioManager: Sound Finished."));
	// Cleanup logic if needed (AudioComponent auto-destroys by default)
}
