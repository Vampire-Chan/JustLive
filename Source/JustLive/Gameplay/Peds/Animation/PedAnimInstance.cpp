#include "PedAnimInstance.h"
#include "../Ped.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UPedAnimInstance::UPedAnimInstance()
{
	Speed = 0.0f;
	Direction = 0.0f;
	bIsMoving = false;
	bIsInAir = false;
	bIsAccelerating = false;
	MovementState = EMovementState::Idle;
	
	// Combat
	bIsAiming = false;
	AimPitch = 0.0f;
	AimYaw = 0.0f;
	bIsReloading = false;
	
	// Swimming
	bIsSwimming = false;
	bIsUnderwater = false;
	SwimPitch = 0.0f;
	
	// Parkour
	bIsClimbing = false;
	bIsVaulting = false;
	LeanAmount = 0.0f;
	
	// Cover
	bIsPeeking = false;
	PeekDirection = 0.0f;
	
	// Weapon
	bHasWeapon = false;
	bIsMelee = false;
}

void UPedAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PedOwner = Cast<APed>(TryGetPawnOwner());
	if (PedOwner)
	{
		MovementComp = PedOwner->GetCharacterMovement();
		AnimComp = PedOwner->AnimationComp;
	}
}

void UPedAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	// Try to cache owner if missing
	if (!PedOwner)
	{
		PedOwner = Cast<APed>(TryGetPawnOwner());
		if (PedOwner)
		{
			MovementComp = PedOwner->GetCharacterMovement();
			AnimComp = PedOwner->AnimationComp;
		}
	}

	if (!PedOwner || !MovementComp) return;

	// 1. Speed
	FVector Velocity = MovementComp->Velocity;
	FVector LateralVelocity = FVector(Velocity.X, Velocity.Y, 0.0f);
	Speed = LateralVelocity.Size();

	// 2. Moving Flag
	bIsMoving = Speed > 3.0f;

	// 3. Direction (Locomotion Angle)
	FRotator Rotation = PedOwner->GetActorRotation();
	Direction = CalculateDirection(Velocity, Rotation);

	// 4. Air
	bIsInAir = MovementComp->IsFalling();

	// 5. Acceleration (for Stops/Starts)
	bIsAccelerating = MovementComp->GetCurrentAcceleration().Size() > 0.0f;

	// 6. State (from Component)
	if (AnimComp)
	{
		MovementState = AnimComp->MovementState;
		Stance = AnimComp->Stance;
		CoverState = AnimComp->CoverState;
	}

	// 7. Aim Pitch/Yaw (for AimOffset)
	FRotator AimRotation = PedOwner->GetBaseAimRotation();
	AimPitch = AimRotation.Pitch;
	if (AimPitch > 180.0f) AimPitch -= 360.0f; // Normalize to -180 to 180
	AimYaw = AimRotation.Yaw;
	
	// 8. Swimming State
	bIsSwimming = (AnimComp && AnimComp->SwimmingState != ESwimmingState::None);
	if (bIsSwimming)
	{
		// Check if underwater (Z velocity and water depth)
		bIsUnderwater = Velocity.Z < -50.0f; // Simplified check
		SwimPitch = AimPitch; // Use aim pitch for swim direction
	}
	
	// 9. Parkour States
	bIsClimbing = (MovementState == EMovementState::Climbing);
	// bIsVaulting would be set by animation montage or gameplay code
	
	// 10. Cover States
	if (CoverState != ECoverState::None)
	{
		// bIsPeeking and PeekDirection would be set by input/gameplay code
	}
	
	// 11. Weapon Info
	if (PedOwner->InventoryComp)
	{
		// TODO: Check if player has weapon equipped
		// bHasWeapon = InventoryComp->HasWeaponEquipped();
		// bIsMelee = InventoryComp->IsMeleeWeapon();
	}
}
