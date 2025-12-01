#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "../Peds/Components/AttributesComponent.h" // For EPedType
#include "PedSpawner.generated.h"

class APed;

/**
 * Ped Spawner
 * Helper class to spawn Peds with specific configurations.
 */
UCLASS()
class JUSTLIVE_API UPedSpawner : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Spawns a Ped at the specified location.
	 * @param WorldContextObject - Context to get the World.
	 * @param Location - Spawn Location.
	 * @param PedType - Type of Ped (Civilian, Cop, etc.).
	 * @param MeshPath - Path to Skeletal Mesh (optional).
	 * @param SkinColor - Color for M_Ped (optional).
	 * @return The spawned APed or nullptr.
	 */
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	static APed* SpawnPed(const UObject* WorldContextObject, FVector Location, EPedType PedType, FString MeshPath = "", FLinearColor SkinColor = FLinearColor::White);
};
