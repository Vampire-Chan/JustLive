#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/AttributesComponent.h" // For EAgeGroup
#include "Animation/Core/AnimationEnums.h"
#include "Ped.generated.h"

// Forward Declarations
class UInventoryComponent;
class UAttributesComponent;
class UAnimationComponent;
class USpeechComponent;
class UPedGroup; // Added for CurrentSquad
class UTextRenderComponent; // Added for DebugTextComp

USTRUCT(BlueprintType)
struct FCharacterData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FString Id;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FString Name;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FLinearColor SkinColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	float Health = 100.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	float Armor = 0.0f;
};

UCLASS()
class JUSTLIVE_API APed : public ACharacter
{
	GENERATED_BODY()

public:
	APed();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 * Initializes the Ped with data from XML.
	 */
	UFUNCTION(BlueprintCallable, Category = "Character")
	void InitializeCharacter(const FCharacterData& Data);

	/**
	 * Sets the skin color and material properties.
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Visuals")
	void SetSkin(FLinearColor Color, float Metallic = 0.0f, float Specular = 0.5f, float Roughness = 0.5f);

	// Input Handlers
	virtual void StartAim();
	virtual void StopAim();
	virtual void StartFire();
	virtual void StopFire();

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInventoryComponent* InventoryComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAttributesComponent* AttributesComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAnimationComponent* AnimationComp;

	/** Main Animation Manager (Pure C++ System) */
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	class UAnimationManager* AnimManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpeechComponent* SpeechComp;

	// ========================================================================
	// LOCOMOTION API
	// ========================================================================

	/** Set the desired gait (Walk, Run, Sprint) */
	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void SetGait(EMovementState NewGait);

	/** Set the stance (Stand, Crouch, Prone) */
	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void SetStance(EStance NewStance);

	/** Set the locomotion animation style */
	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void SetLocomotionStyle(ELocomotionSet NewStyle);

	/** Stop all movement immediately */
	UFUNCTION(BlueprintCallable, Category = "Locomotion")
	void StopMovement();

	/** Check if currently moving */
	UFUNCTION(BlueprintPure, Category = "Locomotion")
	bool IsMoving() const;

	// Relationship Group (e.g., "Police", "Ballas")

	// Relationship Group (e.g., "Police", "Ballas")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	FName GroupID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	EAgeGroup AgeGroup = EAgeGroup::Adult;

	// Current Squad
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	class UPedGroup* CurrentSquad;

	// Debug Text Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	class UTextRenderComponent* DebugTextComp;

private:
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;
};
