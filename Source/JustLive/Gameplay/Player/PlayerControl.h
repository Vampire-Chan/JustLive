#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerControl.generated.h"

/**
 * Custom Player Controller for JustLive.
 * Handles Input routing and UI interactions.
 */
UCLASS()
class JUSTLIVE_API APlayerControl : public APlayerController
{
	GENERATED_BODY()
	
public:
	APlayerControl();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
};
