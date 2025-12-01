#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/DirectionalLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "WorldCycleManager.generated.h"

/**
 * World Cycle Manager
 * Controls Time of Day, Weather, and Sky state.
 * We use a WorldSubsystem because it's inherently tied to the current Level/World.
 */
UCLASS()
class JUSTLIVE_API UWorldCycleManager : public UWorldSubsystem, public FTickableGameObject
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
	 * Sets the time of day (0.0 - 24.0).
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldCycle")
	void SetTimeOfDay(float NewTime);

	/**
	 * Gets current time of day.
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldCycle")
	float GetTimeOfDay() const { return TimeOfDay; }

	/**
	 * Sets how fast time passes (1.0 = real time, 60.0 = 1 game minute per real second).
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldCycle")
	void SetTimeSpeed(float NewSpeed) { TimeSpeed = NewSpeed; }

	/**
	 * Forces a refresh of actor references (Sun, Sky, etc.) from the current world.
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldCycle")
	void RefreshActors();

private:
	void UpdateSunPosition();
	void UpdateAtmosphere();

	float TimeOfDay = 12.0f; // Start at noon
	float TimeSpeed = 0.0f;  // Paused by default until loaded

	// Actor References (Weak to avoid keeping dead actors alive)
	TWeakObjectPtr<ADirectionalLight> SunLight;
	TWeakObjectPtr<AActor> SkyAtmosphere;
	TWeakObjectPtr<AActor> HeightFog;
};
