#include "PedAIController.h"
#include "../Peds/Ped.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../Peds/Components/InventoryComponent.h"
#include "../Weapons/WeaponBase.h"
#include "Navigation/PathFollowingComponent.h"

APedAIController::APedAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	// Setup Perception
	SetPerceptionComponent(*CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp")));
	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 2000.0f;
	SightConfig->LoseSightRadius = 2500.0f;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	GetPerceptionComponent()->ConfigureSense(*SightConfig);
	GetPerceptionComponent()->SetDominantSense(SightConfig->GetSenseImplementation());
	GetPerceptionComponent()->OnPerceptionUpdated.AddDynamic(this, &APedAIController::OnPerceptionUpdated);
}

void APedAIController::BeginPlay()
{
	Super::BeginPlay();
	
	// Start Logic Loop
	GetWorldTimerManager().SetTimer(TimerHandle_StateUpdate, this, &APedAIController::UpdateStateLogic, 0.5f, true);
}

void APedAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ControlledPed = Cast<APed>(InPawn);
}

void APedAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Per-frame updates (e.g. facing target)
	if (CurrentState == EPedAIState::Combat_Attack && TargetActor)
	{
		FaceTarget();
	}
}

void APedAIController::SetState(EPedAIState NewState)
{
	if (CurrentState == NewState) return;
	CurrentState = NewState;
	
	// On Enter State Logic
	if (CurrentState == EPedAIState::Combat_Flee)
	{
		StopMovement();
		HandleFlee(); // Trigger immediately
	}
}

void APedAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	for (AActor* Actor : UpdatedActors)
	{
		// Simple logic: If we see a Player, target them
		if (Actor->ActorHasTag("Player") || Actor->IsA(APawn::StaticClass())) // Assuming Player is Pawn for now
		{
			// Check if hostile? For now, assume hostile.
			TargetActor = Actor;
			SetState(EPedAIState::Combat_Attack);
			return;
		}
	}
}

void APedAIController::UpdateStateLogic()
{
	if (!ControlledPed) return;

	switch (CurrentState)
	{
	case EPedAIState::Idle:
		HandleIdle();
		break;
	case EPedAIState::Patrol:
		HandlePatrol();
		break;
	case EPedAIState::Combat_Attack:
		HandleAttack();
		break;
	case EPedAIState::Combat_Flee:
		HandleFlee();
		break;
	}
}

void APedAIController::HandleIdle()
{
	// Randomly switch to Patrol
	if (FMath::FRand() < 0.1f)
	{
		SetState(EPedAIState::Patrol);
	}
}

void APedAIController::HandlePatrol()
{
	// If not moving, pick a random point
	if (GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		FVector Dest = GetRandomPointInRadius(ControlledPed->GetActorLocation(), 1000.0f);
		MoveToLocation(Dest);
	}
}

void APedAIController::HandleAttack()
{
	if (!TargetActor)
	{
		SetState(EPedAIState::Idle);
		return;
	}

	float Dist = FVector::Dist(ControlledPed->GetActorLocation(), TargetActor->GetActorLocation());

	// Too close? Flee!
	if (Dist < FleeRange)
	{
		SetState(EPedAIState::Combat_Flee);
		return;
	}

	// In Range? Shoot!
	if (Dist <= AttackRange && CanSeeTarget())
	{
		StopMovement();
		ShootTarget();
	}
	else
	{
		// Chase
		MoveToActor(TargetActor, AttackRange * 0.8f);
	}
}

void APedAIController::HandleFlee()
{
	if (!TargetActor)
	{
		SetState(EPedAIState::Idle);
		return;
	}

	// Run away from target
	FVector DirToTarget = (TargetActor->GetActorLocation() - ControlledPed->GetActorLocation()).GetSafeNormal();
	FVector FleeDir = -DirToTarget;
	FVector FleeDest = ControlledPed->GetActorLocation() + (FleeDir * SafeDistance);

	// Find valid point on NavMesh
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FNavLocation Result;
		if (NavSys->GetRandomPointInNavigableRadius(FleeDest, 500.0f, Result))
		{
			MoveToLocation(Result.Location);
		}
		else
		{
			MoveToLocation(FleeDest); // Try anyway
		}
	}

	// Check if safe
	float Dist = FVector::Dist(ControlledPed->GetActorLocation(), TargetActor->GetActorLocation());
	if (Dist > SafeDistance * 0.8f)
	{
		SetState(EPedAIState::Combat_Attack); // Turn and fight
	}
}

bool APedAIController::CanSeeTarget() const
{
	return LineOfSightTo(TargetActor);
}

void APedAIController::FaceTarget()
{
	if (TargetActor)
	{
		SetFocus(TargetActor);
	}
}

void APedAIController::ShootTarget()
{
	if (ControlledPed && ControlledPed->InventoryComp)
	{
		// Equip weapon if needed
		if (ControlledPed->InventoryComp->CurrentSlot == EWeaponSlot::Unarmed)
		{
			ControlledPed->InventoryComp->EquipWeaponSlot(EWeaponSlot::Primary);
		}

		if (AWeaponBase* Weapon = ControlledPed->InventoryComp->GetCurrentWeapon())
		{
			Weapon->StartFire();
			// Stop firing after a burst?
			// Weapon handles auto-fire loop. We should stop it eventually.
			// For simple AI, just start fire.
			// Ideally, we'd have a "StopFire" call after some time.
		}
	}
}

FVector APedAIController::GetRandomPointInRadius(FVector Origin, float Radius)
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FNavLocation Result;
		if (NavSys->GetRandomPointInNavigableRadius(Origin, Radius, Result))
		{
			return Result.Location;
		}
	}
	return Origin;
}
