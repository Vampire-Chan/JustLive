#include "MapLoader.h"
#include "Kismet/GameplayStatics.h"
#include "JsonUtilities.h"
#include "../UI/UIManager.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "VFS/VFSManager.h"

void UMapLoader::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UMapLoader::Deinitialize()
{
	Super::Deinitialize();
}

void UMapLoader::LoadMap(const FString& JsonFilePath)
{
	if (bIsLoading) return;

	UE_LOG(LogTemp, Log, TEXT("Starting Map Load: %s"), *JsonFilePath);

	// 1. Switch UI to Loading Screen (Fake 1s load)
	if (UUIManager* UI = GetGameInstance()->GetSubsystem<UUIManager>())
	{
		UI->TriggerLoadingScreen("Reading Map File...", 1.0f);
	}

	// ... (rest of function)



	// 2. Read File (Synchronous for now, but file IO is fast for text)
	// Resolve path if it uses VFS (e.g. @data/maps/test.jmap)
	FString PhysicalPath = JsonFilePath;
	if (JsonFilePath.StartsWith("@"))
	{
		if (UVFSManager* VFS = GetGameInstance()->GetSubsystem<UVFSManager>())
		{
			PhysicalPath = VFS->Resolve(JsonFilePath);
		}
	}

	FString JsonContent;
	if (FFileHelper::LoadFileToString(JsonContent, *PhysicalPath))
	{
		ParseMapData(JsonContent);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load map file: %s"), *PhysicalPath);
		if (UUIManager* UI = GetGameInstance()->GetSubsystem<UUIManager>())
		{
			UI->UpdateLoadingProgress(1.0f, "Error: Map Not Found");
			// Fallback or stay stuck? For now, let's just abort.
			bIsLoading = false; 
			UI->SwitchToState(EUIState::MainMenu);
		}
	}
}

void UMapLoader::ParseMapData(const FString& JsonContent)
{
	// simple JSON structure: { "props": [ { ... }, { ... } ] }
	
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* PropsArray;
		if (JsonObject->TryGetArrayField(TEXT("props"), PropsArray))
		{
			SpawnQueue.Empty();
			for (const auto& Val : *PropsArray)
			{
				TSharedPtr<FJsonObject> PropObj = Val->AsObject();
				if (PropObj.IsValid())
				{
					FMapPropData Data;
					Data.AssetPath = PropObj->GetStringField(TEXT("asset"));
					
					// Parse Location
					const TArray<TSharedPtr<FJsonValue>>* PosArray;
					if (PropObj->TryGetArrayField(TEXT("pos"), PosArray) && PosArray->Num() >= 3)
					{
						Data.Location.X = (*PosArray)[0]->AsNumber();
						Data.Location.Y = (*PosArray)[1]->AsNumber();
						Data.Location.Z = (*PosArray)[2]->AsNumber();
					}

					// Parse Rotation
					const TArray<TSharedPtr<FJsonValue>>* RotArray;
					if (PropObj->TryGetArrayField(TEXT("rot"), RotArray) && RotArray->Num() >= 3)
					{
						Data.Rotation.Pitch = (*RotArray)[0]->AsNumber();
						Data.Rotation.Yaw   = (*RotArray)[1]->AsNumber();
						Data.Rotation.Roll  = (*RotArray)[2]->AsNumber();
					}

					// Parse Scale
					const TArray<TSharedPtr<FJsonValue>>* ScaleArray;
					if (PropObj->TryGetArrayField(TEXT("scale"), ScaleArray) && ScaleArray->Num() >= 3)
					{
						Data.Scale.X = (*ScaleArray)[0]->AsNumber();
						Data.Scale.Y = (*ScaleArray)[1]->AsNumber();
						Data.Scale.Z = (*ScaleArray)[2]->AsNumber();
					}

					Data.bSimulatePhysics = PropObj->GetBoolField(TEXT("physics"));

					SpawnQueue.Add(Data);
				}
			}

			TotalItemsToSpawn = SpawnQueue.Num();
			ItemsSpawned = 0;
			bIsLoading = true; // Start Tick Processing

			UE_LOG(LogTemp, Log, TEXT("Map Parsed. Objects to spawn: %d"), TotalItemsToSpawn);
		}
	}
}

void UMapLoader::Tick(float DeltaTime)
{
	if (!bIsLoading) return;

	// Process Spawning with Time Slicing
	ProcessSpawnQueue(MAX_SPAWN_TIME_MS);

	// Update UI
	if (TotalItemsToSpawn > 0)
	{
		float Progress = (float)ItemsSpawned / (float)TotalItemsToSpawn;
		
		if (UUIManager* UI = GetGameInstance()->GetSubsystem<UUIManager>())
		{
			FString Status = FString::Printf(TEXT("Spawning Objects (%d/%d)..."), ItemsSpawned, TotalItemsToSpawn);
			UI->UpdateLoadingProgress(Progress, Status);
		}
	}

	// Check Completion
	if (SpawnQueue.Num() == 0)
	{
		FinishLoading();
	}
}

void UMapLoader::ProcessSpawnQueue(float TimeBudgetMs)
{
	double StartTime = FPlatformTime::Seconds();

	while (SpawnQueue.Num() > 0)
	{
		// Check Time Budget
		double CurrentTime = FPlatformTime::Seconds();
		if ((CurrentTime - StartTime) * 1000.0f > TimeBudgetMs)
		{
			break; // Stop for this frame
		}

		// Spawn One Item
		FMapPropData Data = SpawnQueue.Pop();
		ItemsSpawned++;

		// Logic to Spawn Actor
		if (UWorld* World = GetWorld())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			// Load Asset (Soft Object Path logic would be better for async, but let's assume pre-loaded or blocking load for individual assets)
			// Ideally, assets should be pre-loaded in an "Asset Loading" phase before "Spawning Phase".
			// For this prototype, we just attempt load.
			
			// Note: In a real GMod scenario, we'd use the VFS to find the uasset.
			UObject* MeshObj = StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *Data.AssetPath);
			if (UStaticMesh* Mesh = Cast<UStaticMesh>(MeshObj))
			{
				AStaticMeshActor* NewProp = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Data.Location, Data.Rotation, SpawnParams);
				if (NewProp)
				{
					NewProp->GetStaticMeshComponent()->SetStaticMesh(Mesh);
					NewProp->SetActorScale3D(Data.Scale);
					
					if (Data.bSimulatePhysics)
					{
						NewProp->GetStaticMeshComponent()->SetSimulatePhysics(true);
						NewProp->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
					}
				}
			}
		}
	}
}

void UMapLoader::FinishLoading()
{
	bIsLoading = false;
	UE_LOG(LogTemp, Log, TEXT("Map Loading Complete."));

	if (UUIManager* UI = GetGameInstance()->GetSubsystem<UUIManager>())
	{
		UI->UpdateLoadingProgress(1.0f, "Ready!");
		// We let UIManager handle the transition after the fake timer expires
	}
}
