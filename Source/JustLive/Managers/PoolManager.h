#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/Actor.h"
#include "PoolManager.generated.h"

/**
 * Pool Manager
 * Handles efficient spawning and recycling of actors.
 */
UCLASS()
class JUSTLIVE_API UPoolManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UPoolManager* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Spawns an actor from the pool or creates a new one if pool is empty.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pooling", meta = (WorldContext = "WorldContextObject"))
	AActor* SpawnFromPool(UClass* ActorClass, const FTransform& Transform, UObject* WorldContextObject);

	/**
	 * Returns an actor to the pool.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pooling")
	void ReturnToPool(AActor* Actor);

private:
	// Map of Class -> List of Inactive Actors
	TMap<UClass*, TArray<AActor*>> Pool;
};
