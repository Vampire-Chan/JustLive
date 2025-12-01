#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Core/AnimationEnums.h"
#include "../Components/AnimationComponent.h"
#include "PedAnimInstance.generated.h"

class APed;
class UCharacterMovementComponent;

/**
 * Ped Animation Instance
 * The C++ backend for the Ped's Animation Blueprint.
 * Updates variables like Speed, Direction, and State for the AnimGraph.
 */
UCLASS()
class JUSTLIVE_API UPedAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPedAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

protected:
	// References
	UPROPERTY(BlueprintReadOnly, Category = "References")
	APed* PedOwner;

	UPROPERTY(BlueprintReadOnly, Category = "References")
	UCharacterMovementComponent* MovementComp;

	UPROPERTY(BlueprintReadOnly, Category = "References")
	UAnimationComponent* AnimComp;

public:
	// --- Animation Variables (Read by AnimGraph) ---

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsMoving;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsAccelerating;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	EMovementState MovementState;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	EStance Stance;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	ECoverState CoverState;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAiming;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float AimPitch;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float AimYaw;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsReloading;

	// Swimming
	UPROPERTY(BlueprintReadOnly, Category = "Swimming")
	bool bIsSwimming;

	UPROPERTY(BlueprintReadOnly, Category = "Swimming")
	bool bIsUnderwater;

	UPROPERTY(BlueprintReadOnly, Category = "Swimming")
	float SwimPitch;

	// Parkour/Movement
	UPROPERTY(BlueprintReadOnly, Category = "Parkour")
	bool bIsClimbing;

	UPROPERTY(BlueprintReadOnly, Category = "Parkour")
	bool bIsVaulting;

	UPROPERTY(BlueprintReadOnly, Category = "Parkour")
	float LeanAmount;

	// Cover
	UPROPERTY(BlueprintReadOnly, Category = "Cover")
	bool bIsPeeking;

	UPROPERTY(BlueprintReadOnly, Category = "Cover")
	float PeekDirection; // -1 = Left, 0 = None, 1 = Right

	// Weapon Info
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bHasWeapon;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bIsMelee;
};
