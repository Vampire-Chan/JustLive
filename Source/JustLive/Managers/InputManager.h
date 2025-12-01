#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "InputMappingContext.h"
#include "InputManager.generated.h"

/**
 * Control Mode for the Input Manager
 */
UENUM(BlueprintType)
enum class EInputControlMode : uint8
{
	GameOnly,       // Cursor hidden, input to game
	UIOnly,         // Cursor visible, input to UI
	GameAndUI       // Cursor visible, input to both
};

/**
 * Input Manager Subsystem
 * Manages input contexts, control modes, and mouse cursor state.
 */
UCLASS()
class JUSTLIVE_API UInputManager : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ========================================================================
	// CONTEXT MANAGEMENT
	// ========================================================================

	/**
	 * Add an input mapping context with priority
	 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void AddContext(const UInputMappingContext* Context, int32 Priority);

	/**
	 * Remove an input mapping context
	 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void RemoveContext(const UInputMappingContext* Context);

	/**
	 * Remove all contexts
	 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void ClearAllContexts();

	// ========================================================================
	// CONTROL MODE (Game vs UI)
	// ========================================================================

	/**
	 * Set the input control mode (Game, UI, or Both)
	 * Handles cursor visibility and input routing
	 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetControlMode(EInputControlMode NewMode);

	/**
	 * Get current control mode
	 */
	UFUNCTION(BlueprintPure, Category = "Input")
	EInputControlMode GetControlMode() const { return CurrentControlMode; }

	/**
	 * Set mouse cursor visibility directly
	 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetCursorVisible(bool bVisible);

	// ========================================================================
	// HELPERS
	// ========================================================================

	/**
	 * Get the Enhanced Input Subsystem
	 */
	class UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem() const;

	/**
	 * Get the Player Controller
	 */
	APlayerController* GetPlayerController() const;

private:
	EInputControlMode CurrentControlMode = EInputControlMode::GameOnly;
};
