#pragma once

#include "CoreMinimal.h"
#include "../Peds/Ped.h"
#include "InputActionValue.h"
#include "PlayerPed.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

/**
 * The Player's Character.
 * Extends APed with Camera, SpringArm, and Input Bindings.
 */
UCLASS()
class JUSTLIVE_API APlayerPed : public APed
{
	GENERATED_BODY()

public:
	APlayerPed();

protected:
	virtual void BeginPlay() override;

public:
	// Camera Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	// Enhanced Input Assets
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* WeaponMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* CrouchAction;

	// Generic Actions (User Defined)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* GenericBoolAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* GenericFloatAction;

	// Input Binding
	// Input Binding
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// Tick
	virtual void Tick(float DeltaTime) override;

	// Helper to Add/Remove Contexts
	void AddInputMapping(UInputMappingContext* Context, int32 Priority);
	void RemoveInputMapping(UInputMappingContext* Context);

	// --- Input Handlers ---
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	
	// Generic Dispatcher for IA_Bool
	void Input_GenericAction(const FInputActionValue& Value);

	// Re-implementing these to take ActionValue if needed, or just wrapper
	void Input_Jump(const FInputActionValue& Value);
	void Input_Sprint(const FInputActionValue& Value);
	void Input_StopSprint(const FInputActionValue& Value);
	void Input_Crouch(const FInputActionValue& Value);
	
	void Input_StartAim(const FInputActionValue& Value);
	void Input_StopAim(const FInputActionValue& Value);
	void Input_StartFire(const FInputActionValue& Value);
	void Input_StopFire(const FInputActionValue& Value);

	// --- Overrides ---
	virtual void StartAim() override;
	virtual void StopAim() override;

	// --- Weapon ---
	void SpawnDefaultWeapon();

	// --- Camera Zoom ---
	void Input_CameraZoom(const FInputActionValue& Value);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	float TargetCameraZoom = 300.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	float BaseTurnRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	float BaseLookUpRate;
};
