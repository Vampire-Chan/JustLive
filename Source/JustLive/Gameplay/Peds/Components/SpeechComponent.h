#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpeechComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class JUSTLIVE_API USpeechComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpeechComponent();

protected:
	virtual void BeginPlay() override;

public:	
	/**
	 * Plays a bark (short dialogue) at the Ped's location.
	 * @param BarkId - e.g., "pain", "attack", "greet"
	 */
	UFUNCTION(BlueprintCallable, Category = "Speech")
	void PlayBark(const FString& BarkId);
};
