#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "MapLoader.generated.h"

USTRUCT()
struct FMapPropData
{
	GENERATED_BODY()

	UPROPERTY()
	FString AssetPath;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY()
	FVector Scale = FVector::OneVector;

	UPROPERTY()
	bool bSimulatePhysics = false;
};

USTRUCT()
struct FMapLightData
{
	GENERATED_BODY()

	UPROPERTY()
	FString Type; // "Point", "Spot", "Rect"

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY()
	FLinearColor Color = FLinearColor::White;

	UPROPERTY()
	float Intensity = 5000.0f;

	UPROPERTY()
	float Radius = 1000.0f;
};

/**
 * Map Loader Subsystem
 * Handles asynchronous loading of custom JSON maps with progress reporting.
 * This solves the "frozen screen" issue by time-slicing the actor spawning.
 */
UCLASS()
class JUSTLIVE_API UMapLoader : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

	/**
	 * Starts loading a map from a JSON file.
	 */
	UFUNCTION(BlueprintCallable, Category = "MapLoader")
	void LoadMap(const FString& JsonFilePath);

private:
	void ParseMapData(const FString& JsonContent);
	void ProcessSpawnQueue(float TimeBudget);
	void FinishLoading();

	// State
	bool bIsLoading = false;
	TArray<FMapPropData> SpawnQueue;
	TArray<FMapLightData> SpawnLightQueue;
	int32 TotalItemsToSpawn = 0;
	int32 ItemsSpawned = 0;

	// Performance Tuning
	const float MAX_SPAWN_TIME_MS = 10.0f; // Max milliseconds to spend spawning per frame (target 60fps = 16ms total)
};
