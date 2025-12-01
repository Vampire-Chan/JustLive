#pragma once

#include "CoreMinimal.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Peds/Components/AttributesComponent.h" // For EPedType
#include "PedSpawnerTest.generated.h"

class APed;
class UPedGroup;

/**
 * Ped Spawner Test Actor
 * Spawns a grid of Peds to verify spawning, visuals, and squad logic.
 */
UCLASS()
class JUSTLIVE_API APedSpawnerTest : public AActor
{
	GENERATED_BODY()
	
public:	
	APedSpawnerTest();

protected:
	virtual void BeginPlay() override;

public:	
	// Number of Peds to spawn per type
	UPROPERTY(EditAnywhere, Category = "Test")
	int32 CountPerType = 5;

	// Spawn Area Extents (500 x 500)
	UPROPERTY(EditAnywhere, Category = "Test")
	FVector SpawnArea = FVector(500.0f, 500.0f, 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Test")
	void SpawnTestPeds();

private:
	void CreateSquadForType(EPedType Type, int32 Count);
};
