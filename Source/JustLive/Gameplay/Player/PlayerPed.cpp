#include "PlayerPed.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "../Peds/Components/AnimationComponent.h"
#include "../Peds/Components/InventoryComponent.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"

APlayerPed::APlayerPed()
{
	// 1. Camera Boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// 2. Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// 3. Movement Settings
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Rates
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Load Enhanced Input Assets (Using generic names as per user request)
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultContextFinder(TEXT("/Game/Input/IMC_OnFoot"));
	if (DefaultContextFinder.Succeeded()) DefaultMappingContext = DefaultContextFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> WeaponContextFinder(TEXT("/Game/Input/IMC_OnWeaponEquip"));
	if (WeaponContextFinder.Succeeded()) WeaponMappingContext = WeaponContextFinder.Object;

	// IA_2d for Move (WASD)
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(TEXT("/Game/Input/IA_2d"));
	if (MoveActionFinder.Succeeded()) MoveAction = MoveActionFinder.Object;

	// IA_2d (or IA_Float?) for Look. Assuming IA_2d for MouseXY based on standard usage.
	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionFinder(TEXT("/Game/Input/IA_2d")); 
	if (LookActionFinder.Succeeded()) LookAction = LookActionFinder.Object;

	// IA_Bool for EVERYTHING (Dispatcher)
	static ConstructorHelpers::FObjectFinder<UInputAction> BoolActionFinder(TEXT("/Game/Input/IA_Bool"));
	if (BoolActionFinder.Succeeded()) GenericBoolAction = BoolActionFinder.Object;
}

void APlayerPed::BeginPlay()
{
	Super::BeginPlay();
	AddInputMapping(DefaultMappingContext, 0);
	AddInputMapping(WeaponMappingContext, 1); // Add Weapon Context too

	// Spawn Default Weapon
	SpawnDefaultWeapon();
}

void APlayerPed::SpawnDefaultWeapon()
{
	if (InventoryComp)
	{
		// Create a default weapon info (Pillar Gun)
		FWeaponInfo DefaultWeapon;
		DefaultWeapon.Name = "Pillar Gun";
		DefaultWeapon.Slot = EWeaponSlot::Primary;
		DefaultWeapon.Damage = 25.0f;
		DefaultWeapon.Range = 5000.0f;
		DefaultWeapon.FireRate = 0.1f;
		DefaultWeapon.ClipSize = 30;
		DefaultWeapon.MaxAmmoReserve = 120;
		DefaultWeapon.FireType = EFireType::Automatic;
		DefaultWeapon.AmmoType = EAmmoType::Rifle;
		
		DefaultWeapon.AmmoType = EAmmoType::Rifle;
		
		// Explicitly zero out pointers to prevent garbage crash
		DefaultWeapon.Icon = nullptr;
		DefaultWeapon.SoundFire = nullptr;
		DefaultWeapon.SoundReload = nullptr;
		DefaultWeapon.SoundEmpty = nullptr;
		DefaultWeapon.VFXMuzzleFlash = nullptr;
		DefaultWeapon.VFXImpact = nullptr;
		DefaultWeapon.ProjectileClass = nullptr;
		DefaultWeapon.AnimFire = nullptr;
		DefaultWeapon.AnimReload = nullptr;

		DefaultWeapon.AnimReload = nullptr;

		// Load Assets
		// USER REQUEST: Use new Skeletal Mesh Gun
		DefaultWeapon.WeaponSkeletalMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Gun.Gun"));
		
		// Fallback to Pillar if Skeletal Mesh fails (though user said it exists)
		if (!DefaultWeapon.WeaponSkeletalMesh)
		{
			DefaultWeapon.WeaponMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/StarterContent/Architecture/Pillar_50x500.Pillar_50x500"));
		}
		else
		{
			DefaultWeapon.WeaponMesh = nullptr; // Ensure Static Mesh is null if using Skeletal
		}
		
		// Add and Equip
		InventoryComp->AddWeapon(DefaultWeapon, 120);
		InventoryComp->EquipWeaponSlot(EWeaponSlot::Primary);
	}
}

void APlayerPed::AddInputMapping(UInputMappingContext* Context, int32 Priority)
{
	if (!Context) return;
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (!Subsystem->HasMappingContext(Context))
			{
				Subsystem->AddMappingContext(Context, Priority);
			}
		}
	}
}

void APlayerPed::RemoveInputMapping(UInputMappingContext* Context)
{
	if (!Context) return;
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(Context);
		}
	}
}

void APlayerPed::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Moving
		if (MoveAction)
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerPed::Move);

		// Looking
		if (LookAction)
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerPed::Look);

		// Generic Bool Dispatcher
		if (GenericBoolAction)
		{
			// Bind both Started (Press) and Completed (Release) to the same dispatcher
			EnhancedInputComponent->BindAction(GenericBoolAction, ETriggerEvent::Started, this, &APlayerPed::Input_GenericAction);
			EnhancedInputComponent->BindAction(GenericBoolAction, ETriggerEvent::Completed, this, &APlayerPed::Input_GenericAction);
		}

		// Generic Float Dispatcher (Mouse Wheel)
		if (GenericFloatAction)
		{
			EnhancedInputComponent->BindAction(GenericFloatAction, ETriggerEvent::Triggered, this, &APlayerPed::Input_CameraZoom);
		}
	}
}

void APlayerPed::Input_GenericAction(const FInputActionValue& Value)
{
	bool bIsPressed = Value.Get<bool>();
	
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		// --- JUMP (Space Bar) ---
		if (PC->IsInputKeyDown(EKeys::SpaceBar))
		{
			if (bIsPressed) Input_Jump(Value);
			else StopJumping();
		}
		
		// --- SPRINT (Left Shift) ---
		if (PC->IsInputKeyDown(EKeys::LeftShift))
		{
			if (bIsPressed) Input_Sprint(Value);
			else Input_StopSprint(Value);
		}

		// --- CROUCH (Left Ctrl) ---
		if (PC->IsInputKeyDown(EKeys::LeftControl))
		{
			if (bIsPressed) Input_Crouch(Value);
		}

		// --- FIRE (Left Mouse) ---
		if (PC->IsInputKeyDown(EKeys::LeftMouseButton))
		{
			if (bIsPressed) Input_StartFire(Value);
			else Input_StopFire(Value);
		}

		// --- AIM (Right Mouse) ---
		if (PC->IsInputKeyDown(EKeys::RightMouseButton))
		{
			if (bIsPressed) Input_StartAim(Value);
			else Input_StopAim(Value);
		}
		
		// --- RELOAD (R) ---
		if (PC->IsInputKeyDown(EKeys::R))
		{
			if (bIsPressed && InventoryComp) InventoryComp->ReloadWeapon();
		}
	}
}

// ... (Existing Functions)

void APlayerPed::Input_CameraZoom(const FInputActionValue& Value)
{
	float ZoomVal = Value.Get<float>(); // Mouse Wheel Axis (+1 or -1)
	
	if (ZoomVal != 0.0f)
	{
		// Adjust Target Zoom
		TargetCameraZoom = FMath::Clamp(TargetCameraZoom - (ZoomVal * 50.0f), 150.0f, 800.0f);
	}
}

void APlayerPed::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smooth Zoom Interpolation
	if (CameraBoom)
	{
		float CurrentLen = CameraBoom->TargetArmLength;
		if (!FMath::IsNearlyEqual(CurrentLen, TargetCameraZoom, 1.0f))
		{
			CameraBoom->TargetArmLength = FMath::FInterpTo(CurrentLen, TargetCameraZoom, DeltaTime, 10.0f);
		}
	}
}


void APlayerPed::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
		
		// Update Gait based on movement
		if (AnimationComp && AnimationComp->MovementState != EMovementState::Sprinting)
		{
			if (MovementVector.Size() > 0.5f)
			{
				SetGait(EMovementState::Running);
			}
			else
			{
				SetGait(EMovementState::Walking);
			}
		}
	}
}

void APlayerPed::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APlayerPed::Input_Jump(const FInputActionValue& Value)
{
	Jump();
}

void APlayerPed::Input_Sprint(const FInputActionValue& Value)
{
	SetGait(EMovementState::Sprinting);
}

void APlayerPed::Input_StopSprint(const FInputActionValue& Value)
{
	SetGait(EMovementState::Running);
}

void APlayerPed::Input_Crouch(const FInputActionValue& Value)
{
	if (AnimationComp && AnimationComp->Stance == EStance::Crouching)
	{
		SetStance(EStance::Standing);
	}
	else
	{
		SetStance(EStance::Crouching);
	}
}

void APlayerPed::Input_StartAim(const FInputActionValue& Value)
{
	StartAim();
}

void APlayerPed::Input_StopAim(const FInputActionValue& Value)
{
	StopAim();
}

void APlayerPed::StartAim()
{
	Super::StartAim();

	// Enable Strafing (Face Camera)
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	
	// Optional: Slow down when aiming
	GetCharacterMovement()->MaxWalkSpeed = 200.0f; 
}

void APlayerPed::StopAim()
{
	Super::StopAim();

	// Disable Strafing (Face Movement)
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	
	// Restore Speed (handled by SetGait usually, but ensure default)
	if (AnimationComp && AnimationComp->MovementState == EMovementState::Running)
	{
		GetCharacterMovement()->MaxWalkSpeed = 400.0f;
	}
}

void APlayerPed::Input_StartFire(const FInputActionValue& Value)
{
	StartFire();
}

void APlayerPed::Input_StopFire(const FInputActionValue& Value)
{
	StopFire();
}
