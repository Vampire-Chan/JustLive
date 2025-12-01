#include "PhysicsManager.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "../Gameplay/Peds/Ped.h"
#include "Components/CapsuleComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"

void UPhysicsManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

// --- Force Application ---

void UPhysicsManager::ApplyImpulse(UPrimitiveComponent* Component, FVector Impulse, FVector Location, FName BoneName)
{
	if (Component && Component->IsSimulatingPhysics(BoneName))
	{
		Component->AddImpulseAtLocation(Impulse, Location, BoneName);
	}
}

void UPhysicsManager::ApplyImpulseToActor(AActor* Actor, FVector Impulse, FVector Location, FName BoneName)
{
	if (!Actor) return;

	// Check for Ped first (might need ragdoll trigger)
	if (APed* Ped = Cast<APed>(Actor))
	{
		// If force is strong enough, trigger ragdoll?
		// For now, just apply if already simulating
		if (USkeletalMeshComponent* Mesh = Ped->GetMesh())
		{
			if (Mesh->IsSimulatingPhysics(BoneName))
			{
				Mesh->AddImpulseAtLocation(Impulse, Location, BoneName);
			}
		}
	}
	else if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
	{
		ApplyImpulse(RootPrim, Impulse, Location, BoneName);
	}
}

void UPhysicsManager::ApplyRadialImpulse(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bDestructibleDamage)
{
	// Use RadialForceComponent for robust impulse application
	if (UWorld* World = GetWorld())
	{
		// Create a temporary actor to hold the component, or just use NewObject with World as Outer?
		// Components usually need an Actor to function properly (RegisterComponent).
		// We can spawn a temporary actor.
		
		AActor* ImpulseActor = World->SpawnActor<AActor>(AActor::StaticClass(), Origin, FRotator::ZeroRotator);
		if (ImpulseActor)
		{
			URadialForceComponent* ForceComp = NewObject<URadialForceComponent>(ImpulseActor);
			ForceComp->RegisterComponent();
			ForceComp->SetWorldLocation(Origin);
			ForceComp->Radius = Radius;
			ForceComp->ImpulseStrength = Strength;
			ForceComp->bImpulseVelChange = false; // Use Force, not Velocity Change (unless requested)
			ForceComp->Falloff = Falloff;
			ForceComp->DestructibleDamage = bDestructibleDamage ? Strength : 0.0f;
			
			ForceComp->FireImpulse();
			
			ImpulseActor->Destroy();
		}
	}
}

// --- Ragdoll Control ---

void UPhysicsManager::TriggerRagdoll(APed* Ped, FVector HitDirection, float HitStrength, FName HitBone)
{
	if (!Ped) return;

	USkeletalMeshComponent* Mesh = Ped->GetMesh();
	UCapsuleComponent* Capsule = Ped->GetCapsuleComponent();

	if (Mesh && Capsule)
	{
		// 1. Disable Capsule Collision
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// 2. Enable Mesh Physics
		Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetAllBodiesSimulatePhysics(true);
		Mesh->SetSimulatePhysics(true);
		Mesh->WakeAllRigidBodies();
		Mesh->bBlendPhysics = true;

		// 3. Apply Initial Force
		if (HitStrength > 0.0f)
		{
			FVector Impulse = HitDirection.GetSafeNormal() * HitStrength;
			Mesh->AddImpulseAtLocation(Impulse, Mesh->GetBoneLocation(HitBone), HitBone);
		}
	}
}

void UPhysicsManager::RecoverFromRagdoll(APed* Ped)
{
	if (!Ped) return;

	// Complex logic: snapshot pose, blend to animation, re-enable capsule.
	// For now, simple reset:
	USkeletalMeshComponent* Mesh = Ped->GetMesh();
	UCapsuleComponent* Capsule = Ped->GetCapsuleComponent();

	if (Mesh && Capsule)
	{
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionProfileName(TEXT("CharacterMesh"));
		// Re-attach to capsule? usually mesh is child of capsule.
		// Reset relative transform?
		Mesh->AttachToComponent(Capsule, FAttachmentTransformRules::SnapToTargetIncludingScale);
		Mesh->SetRelativeLocation(FVector(0, 0, -90)); // Standard offset
		Mesh->SetRelativeRotation(FRotator(0, -90, 0));

		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

// --- Explosions ---

void UPhysicsManager::CreateExplosion(FVector Location, float Radius, float Damage, float ForceStrength, AActor* Instigator)
{
	// 1. Apply Damage
	UGameplayStatics::ApplyRadialDamage(GetWorld(), Damage, Location, Radius, UDamageType::StaticClass(), TArray<AActor*>(), Instigator);

	// 2. Apply Physics Force
	ApplyRadialImpulse(Location, Radius, ForceStrength, RIF_Linear, true);

	// 3. VFX/Sound (Handled by caller or we can add args)
}
