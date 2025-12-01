#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Components/SplineComponent.h"
#include "NavigationManager.generated.h"

class APed;

/**
 * Navigation Manager
 * Centralized API for Ped navigation (NavMesh, Splines, Direct).
 */
UCLASS()
class JUSTLIVE_API UNavigationManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UNavigationManager* Get(const UObject* WorldContextObject);

	/**
	 * Moves a Ped to a specific location.
	 * @param Ped - The Ped to move.
	 * @param TargetLocation - Destination.
	 * @param bUseNavMesh - If true, uses Pathfinding. If false, moves directly (blindly).
	 */
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void MoveToLocation(APed* Ped, FVector TargetLocation, bool bUseNavMesh = true);

	/**
	 * Makes a Ped follow a spline path.
	 * @param Ped - The Ped to move.
	 * @param Spline - The spline component to follow.
	 * @param Speed - Movement speed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void FollowSpline(APed* Ped, USplineComponent* Spline, float Speed = 600.0f);

	/**
	 * Stops the Ped's movement.
	 */
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void StopMovement(APed* Ped);
};
