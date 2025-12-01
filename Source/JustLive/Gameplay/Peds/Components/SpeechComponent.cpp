#include "SpeechComponent.h"
#include "JustLive/Managers/AudioManager.h"

USpeechComponent::USpeechComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USpeechComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USpeechComponent::PlayBark(const FString& BarkId)
{
	if (UAudioManager* AudioMgr = UAudioManager::Get(this))
	{
		// TODO: Construct full ID based on Ped Voice Type (e.g., "Male01_Pain")
		// For now, just play the raw ID
		AudioMgr->PlaySound(BarkId, GetOwner()->GetActorLocation());
	}
}
