#include "JustLiveGameMode.h"
#include "../UI/UIManager.h"
#include "../Gameplay/Player/PlayerControl.h"
#include "../Gameplay/Player/PlayerPed.h"

AJustLiveGameMode::AJustLiveGameMode()
{
	// Use Pure C++ Classes
	DefaultPawnClass = APlayerPed::StaticClass();
	PlayerControllerClass = APlayerControl::StaticClass();
}

void AJustLiveGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UUIManager* UI = UUIManager::Get(this))
	{
		UI->SwitchToState(EUIState::Startup);
	}
}
