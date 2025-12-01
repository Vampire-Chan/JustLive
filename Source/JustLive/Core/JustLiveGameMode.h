#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "JustLiveGameMode.generated.h"

/**
 * Custom Game Mode
 * Triggers the UI startup sequence.
 */
UCLASS()
class JUSTLIVE_API AJustLiveGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AJustLiveGameMode();
	virtual void BeginPlay() override;
};
