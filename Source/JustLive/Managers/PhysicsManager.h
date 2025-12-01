#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PhysicsManager.generated.h"

class APed;

/**
 * Physics Manager
 * Centralizes control over physics interactions, ragdolls, and explosions.
 */
UCLASS()
class JUSTLIVE_API UPhysicsManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// --- Force Application ---

	/** Apply impulse to a specific component */
	UFUNCTION(BlueprintCallable, Category = "Physics")
	void ApplyImpulse(UPrimitiveComponent* Component, FVector Impulse, FVector Location, FName BoneName = NAME_None);

	/** Apply force to an actor (finds root primitive) */
	UFUNCTION(BlueprintCallable, Category = "Physics")
	void ApplyImpulseToActor(AActor* Actor, FVector Impulse, FVector Location, FName BoneName = NAME_None);

	/** Apply radial force (explosion effect) */
	UFUNCTION(BlueprintCallable, Category = "Physics")
	void ApplyRadialImpulse(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff = RIF_Linear, bool bDestructibleDamage = true);

	// --- Ragdoll Control ---

	/** Trigger ragdoll on a Ped with optional initial force */
	UFUNCTION(BlueprintCallable, Category = "Physics")
	void TriggerRagdoll(APed* Ped, FVector HitDirection, float HitStrength, FName HitBone);

	/** Recover from ragdoll (blend to animation) */
	UFUNCTION(BlueprintCallable, Category = "Physics")
	void RecoverFromRagdoll(APed* Ped);

	// --- Explosions ---

	/** Spawn explosion with damage and physics force */
	UFUNCTION(BlueprintCallable, Category = "Physics")
	void CreateExplosion(FVector Location, float Radius, float Damage, float ForceStrength, AActor* Instigator = nullptr);

private:
	// Helpers
	void EnableRagdoll(APed* Ped);
};
