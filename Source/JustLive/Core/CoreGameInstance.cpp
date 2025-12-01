#include "CoreGameInstance.h"
#include "DataManager.h"
#include "ModManager.h"

void UCoreGameInstance::Init()
{
	Super::Init();
	
	UE_LOG(LogTemp, Log, TEXT("CoreGameInstance: Initializing..."));
	BootGame();
}

void UCoreGameInstance::Shutdown()
{
	Super::Shutdown();
}

void UCoreGameInstance::BootGame()
{
	// 1. Get DataManager
	UDataManager* DataMgr = UDataManager::Get(this);
	if (!DataMgr)
	{
		UE_LOG(LogTemp, Error, TEXT("CoreGameInstance: DataManager is missing!"));
		return;
	}


	// 2. Trigger Boot Sequence
	// This ensures VFS is ready, Mods are scanned, and initial data is loaded.
	DataMgr->Boot();
}
