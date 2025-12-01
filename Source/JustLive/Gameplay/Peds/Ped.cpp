#include "Ped.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Components/InventoryComponent.h"
#include "Components/AttributesComponent.h"
#include "Components/AnimationComponent.h"
#include "Animation/Core/AnimationManager.h"
#include "Animation/PedAnimInstance.h" // Added
#include "Components/SpeechComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "../Groups/PedGroup.h"
#include "../Weapons/WeaponBase.h"
#include "../AI/PedAIController.h"

APed::APed()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// AI Configuration
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = APedAIController::StaticClass();

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: Mesh is inherited from ACharacter
	// We assume the user will set the Skeletal Mesh in the Blueprint child or via Data

	// Create Components
	InventoryComp = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComp"));
	AttributesComp = CreateDefaultSubobject<UAttributesComponent>(TEXT("AttributesComp"));
	AnimationComp = CreateDefaultSubobject<UAnimationComponent>(TEXT("AnimationComp"));
	AnimManager = CreateDefaultSubobject<UAnimationManager>(TEXT("AnimManager"));
	SpeechComp = CreateDefaultSubobject<USpeechComponent>(TEXT("SpeechComp"));

	// Debug Text
	DebugTextComp = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DebugTextComp"));
	DebugTextComp->SetupAttachment(RootComponent);
	DebugTextComp->SetRelativeLocation(FVector(0, 0, 100)); // Modified
	DebugTextComp->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	DebugTextComp->SetTextRenderColor(FColor::White);
	DebugTextComp->SetWorldSize(20.0f);
	DebugTextComp->SetVisibility(false); // Added

	// --- Visuals (Skeletal Mesh for Animation) ---
	// CENTER-ROOT SKELETON: No offset needed (0,0,0)
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	
	// TEMPORARY: User requested Capsule Visuals
	GetMesh()->SetVisibility(false);
	GetCapsuleComponent()->SetHiddenInGame(false);

	// Explicitly ensure no skeletal mesh is set by default from C++ to force data-driven approach
	GetMesh()->SetSkeletalMesh(nullptr);
	// Set our custom C++ AnimInstance class by default
	GetMesh()->SetAnimInstanceClass(UPedAnimInstance::StaticClass());

	// Dynamic Material will be created in BeginPlay or via SetSkin if a base material exists on the mesh
}

void APed::BeginPlay()
{
	Super::BeginPlay();
	
	// Create Dynamic Material for Skeletal Mesh if a base material is present
	if (GetMesh() && GetMesh()->GetNumMaterials() > 0)
	{
		DynamicMaterial = GetMesh()->CreateAndSetMaterialInstanceDynamic(0);
		// Also set for other slots dynamically
		for (int32 i = 1; i < GetMesh()->GetNumMaterials(); ++i)
		{
			GetMesh()->CreateAndSetMaterialInstanceDynamic(i);
		}
	}
}

void APed::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update Debug Text
	if (DebugTextComp && AttributesComp)
	{
		FString TypeStr = UEnum::GetValueAsString(AttributesComp->PedType);
		// Remove "EPedType::" prefix
		TypeStr.RemoveFromStart("EPedType::");

		FString RoleStr = "Lone";
		if (CurrentSquad)
		{
			if (CurrentSquad->Leader == this) RoleStr = "Leader";
			else RoleStr = "Follower";
		}

		FString DebugMsg = FString::Printf(TEXT("%s | %s\n%s"), *TypeStr, *GroupID.ToString(), *RoleStr);
		DebugTextComp->SetText(FText::FromString(DebugMsg));
		
		// Face Camera
		if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			DebugTextComp->SetWorldRotation(Cam->GetCameraRotation());
		}
	}
}

void APed::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Input for APed (if needed for AI debugging, etc.)
	// Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void APed::StartAim()
{
	if (InventoryComp && InventoryComp->CurrentWeaponActor)
	{
		InventoryComp->CurrentWeaponActor->StartAim();
	}
}

void APed::StopAim()
{
	if (InventoryComp && InventoryComp->CurrentWeaponActor)
	{
		InventoryComp->CurrentWeaponActor->StopAim();
	}
}

void APed::StartFire()
{
	if (InventoryComp && InventoryComp->CurrentWeaponActor)
	{
		InventoryComp->CurrentWeaponActor->StartFire();
	}
}

void APed::StopFire()
{
	if (InventoryComp && InventoryComp->CurrentWeaponActor)
	{
		InventoryComp->CurrentWeaponActor->StopFire();
	}
}

void APed::InitializeCharacter(const FCharacterData& Data)
{
	// Apply Data
	SetSkin(Data.SkinColor);
	
	// TODO: Initialize Components with Data
	// if (AttributesComp) AttributesComp->SetHealth(Data.Health);
}

void APed::SetSkin(FLinearColor Color, float Metallic, float Specular, float Roughness)
{
	if (DynamicMaterial)
	{
		// Update all materials on the mesh
		if (GetMesh())
		{
			for (int32 i = 0; i < GetMesh()->GetNumMaterials(); i++)
			{
				if (UMaterialInstanceDynamic* MI = Cast<UMaterialInstanceDynamic>(GetMesh()->GetMaterial(i)))
				{
					MI->SetVectorParameterValue(FName("Color"), Color);
					MI->SetScalarParameterValue(FName("Metallic"), Metallic);
					MI->SetScalarParameterValue(FName("Specular"), Specular);
					MI->SetScalarParameterValue(FName("Roughness"), Roughness);
				}
			}
		}
	}
	else
	{
		// Try to create it again if it failed in BeginPlay
		if (GetMesh())
		{
			DynamicMaterial = GetMesh()->CreateAndSetMaterialInstanceDynamic(0);
			if (DynamicMaterial)
			{
				SetSkin(Color, Metallic, Specular, Roughness);
			}
		}
	}
}

// ============================================================================
// LOCOMOTION API
// ============================================================================

void APed::SetGait(EMovementState NewGait)
{
	if (AnimationComp)
	{
		AnimationComp->SetMovementState(NewGait);
	}
	
	// Update max walk speed based on gait
	if (GetCharacterMovement())
	{
		float TargetSpeed = 0.0f;
		switch (NewGait)
		{
			case EMovementState::Walking: TargetSpeed = 200.0f; break;
			case EMovementState::Running: TargetSpeed = 400.0f; break;
			case EMovementState::Sprinting: TargetSpeed = 600.0f; break;
			default: TargetSpeed = 200.0f; break;
		}
		GetCharacterMovement()->MaxWalkSpeed = TargetSpeed;
	}
}

void APed::SetStance(EStance NewStance)
{
	if (AnimationComp)
	{
		AnimationComp->SetStance(NewStance);
	}
	
	if (NewStance == EStance::Crouching)
	{
		Crouch();
	}
	else
	{
		UnCrouch();
	}
}

void APed::SetLocomotionStyle(ELocomotionSet NewStyle)
{
	if (AnimManager)
	{
		AnimManager->SetLocomotionSet(NewStyle);
	}
}

void APed::StopMovement()
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}

bool APed::IsMoving() const
{
	return GetVelocity().SizeSquared() > 1.0f;
}
