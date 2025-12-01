#include "InputManager.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

void UInputManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("InputManager: Initialized"));
}

void UInputManager::Deinitialize()
{
	Super::Deinitialize();
}

// ============================================================================
// CONTEXT MANAGEMENT
// ============================================================================

void UInputManager::AddContext(const UInputMappingContext* Context, int32 Priority)
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem())
	{
		if (Context)
		{
			Subsystem->AddMappingContext(Context, Priority);
		}
	}
}

void UInputManager::RemoveContext(const UInputMappingContext* Context)
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem())
	{
		if (Context)
		{
			Subsystem->RemoveMappingContext(Context);
		}
	}
}

void UInputManager::ClearAllContexts()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem())
	{
		Subsystem->ClearAllMappings();
	}
}

// ============================================================================
// CONTROL MODE
// ============================================================================

void UInputManager::SetControlMode(EInputControlMode NewMode)
{
	CurrentControlMode = NewMode;
	
	if (APlayerController* PC = GetPlayerController())
	{
		switch (NewMode)
		{
			case EInputControlMode::GameOnly:
				PC->SetInputMode(FInputModeGameOnly());
				PC->bShowMouseCursor = false;
				break;
				
			case EInputControlMode::UIOnly:
			{
				FInputModeUIOnly InputMode;
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				PC->SetInputMode(InputMode);
				PC->bShowMouseCursor = true;
				break;
			}
				
			case EInputControlMode::GameAndUI:
			{
				FInputModeGameAndUI InputMode;
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				InputMode.SetHideCursorDuringCapture(false);
				PC->SetInputMode(InputMode);
				PC->bShowMouseCursor = true;
				break;
			}
		}
		
		UE_LOG(LogTemp, Log, TEXT("InputManager: Control Mode set to %d"), (int32)NewMode);
	}
}

void UInputManager::SetCursorVisible(bool bVisible)
{
	if (APlayerController* PC = GetPlayerController())
	{
		PC->bShowMouseCursor = bVisible;
	}
}

// ============================================================================
// HELPERS
// ============================================================================

UEnhancedInputLocalPlayerSubsystem* UInputManager::GetEnhancedInputSubsystem() const
{
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		return ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	}
	return nullptr;
}

APlayerController* UInputManager::GetPlayerController() const
{
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		return LocalPlayer->GetPlayerController(GetWorld());
	}
	return nullptr;
}
