#include "PoolManager.h"
#include "Kismet/GameplayStatics.h"

UPoolManager* UPoolManager::Get(const UObject* WorldContextObject)
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		return GI->GetSubsystem<UPoolManager>();
	}
	return nullptr;
}

void UPoolManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("PoolManager Initialized."));
}

AActor* UPoolManager::SpawnFromPool(UClass* ActorClass, const FTransform& Transform, UObject* WorldContextObject)
{
	if (!ActorClass || !WorldContextObject) return nullptr;

	UWorld* World = WorldContextObject->GetWorld();
	if (!World) return nullptr;

	// Check if we have an available actor in the pool
	if (Pool.Contains(ActorClass) && Pool[ActorClass].Num() > 0)
	{
		AActor* PooledActor = Pool[ActorClass].Pop();
		if (PooledActor && IsValid(PooledActor))
		{
			PooledActor->SetActorTransform(Transform);
			PooledActor->SetActorHiddenInGame(false);
			PooledActor->SetActorEnableCollision(true);
			PooledActor->SetActorTickEnabled(true);
			
			// Reset physics if needed
			if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(PooledActor->GetRootComponent()))
			{
				RootPrim->SetSimulatePhysics(false); // Default to false, let caller enable
				RootPrim->SetPhysicsLinearVelocity(FVector::ZeroVector);
				RootPrim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			}

			// Call a custom "OnSpawnFromPool" interface if we had one, 
			// but for now just returning it is enough.
			return PooledActor;
		}
	}

	// Create new if pool is empty
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AActor>(ActorClass, Transform, SpawnParams);
}

void UPoolManager::ReturnToPool(AActor* Actor)
{
	if (!Actor || !IsValid(Actor)) return;

	// Disable the actor
	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);

	// Stop movement
	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
	{
		RootPrim->SetSimulatePhysics(false);
	}

	// Add to pool
	UClass* ActorClass = Actor->GetClass();
	if (!Pool.Contains(ActorClass))
	{
		Pool.Add(ActorClass, TArray<AActor*>());
	}
	Pool[ActorClass].Add(Actor);
}
