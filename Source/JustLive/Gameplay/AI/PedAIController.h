#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "PedAIController.generated.h"

class APed;

UENUM(BlueprintType)
enum class EPedAIState : uint8
{
	Idle,
	Patrol,
	Combat_Attack,
	Combat_Flee,
	Dead
};

/**
 * AI Controller for Peds
 * Implements a simple Hit-and-Run behavior using a C++ State Machine.
 */
UCLASS()
class JUSTLIVE_API APedAIController : public AAIController
{
	GENERATED_BODY()

public:
	APedAIController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

public:
	// --- State Machine ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	EPedAIState CurrentState = EPedAIState::Idle;

	void SetState(EPedAIState NewState);

	// --- Sensing ---
	UFUNCTION()
	void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	AActor* TargetActor;

	// --- Combat Params ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float FleeRange = 500.0f; // If target is closer than this, flee

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float SafeDistance = 2000.0f; // Distance to run to when fleeing

	// --- Helpers ---
	bool CanSeeTarget() const;
	void FaceTarget();
	void ShootTarget();

private:
	UPROPERTY()
	APed* ControlledPed;

	FTimerHandle TimerHandle_StateUpdate;
	void UpdateStateLogic();

	// State Handlers
	void HandleIdle();
	void HandlePatrol();
	void HandleAttack();
	void HandleFlee();
	
	// Navigation
	FVector GetRandomPointInRadius(FVector Origin, float Radius);
};
