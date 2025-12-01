#include "NavigationManager.h"
#include "../Peds/Ped.h"
#include "../AI/PedAIController.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "NavigationSystem.h"

UNavigationManager* UNavigationManager::Get(const UObject* WorldContextObject)
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		return GI->GetSubsystem<UNavigationManager>();
	}
	return nullptr;
}

void UNavigationManager::MoveToLocation(APed* Ped, FVector TargetLocation, bool bUseNavMesh)
{
	if (!Ped) return;

	APedAIController* AI = Cast<APedAIController>(Ped->GetController());
	if (!AI) return;

	if (bUseNavMesh)
	{
		// Use standard AI MoveTo
		AI->MoveToLocation(TargetLocation);
	}
	else
	{
		// Direct Movement (Blind)
		// WARNING: This ignores obstacles!
		// We can use a simple MoveTo without pathfinding if needed, or manual velocity setting.
		// For now, we'll force a MoveTo with "Stop on Overlap" disabled and "Strafe" enabled to simulate direct movement?
		// Actually, MoveToLocation always uses NavMesh if available. 
		// To force direct, we might need to use SimpleMoveToLocation or manual input injection.
		
		// Let's use SimpleMoveToLocation for now, but it also uses NavMesh.
		// If we really want "Blind" movement, we might need to set velocity directly in Tick.
		// But for now, let's just log a warning and use standard move.
		UE_LOG(LogTemp, Warning, TEXT("NavigationManager: Direct Movement requested but fallback to NavMesh for safety."));
		AI->MoveToLocation(TargetLocation);
	}
}

void UNavigationManager::FollowSpline(APed* Ped, USplineComponent* Spline, float Speed)
{
	if (!Ped || !Spline) return;

	// TODO: Implement Spline Following Logic
	// This usually involves a Timer or Tick component that updates the "MoveTo" target 
	// to the next point on the spline, or directly interpolates the actor location (physics-less).
	
	// For a physics-based character, we should pick a point ahead on the spline and MoveTo it.
	UE_LOG(LogTemp, Log, TEXT("NavigationManager: FollowSpline called (Not Implemented yet)."));
}

void UNavigationManager::StopMovement(APed* Ped)
{
	if (!Ped) return;

	if (AAIController* AI = Cast<AAIController>(Ped->GetController()))
	{
		AI->StopMovement();
	}
}
