#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CoreGameInstance.generated.h"

/**
 * Custom Game Instance
 * Manages the boot sequence and global game state.
 */
UCLASS()
class JUSTLIVE_API UCoreGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

private:
	void BootGame();
};
