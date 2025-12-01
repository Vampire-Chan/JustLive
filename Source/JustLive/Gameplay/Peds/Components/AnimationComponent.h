#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Animation/Core/AnimationEnums.h"
#include "AnimationComponent.generated.h"

/**
 * Animation Component
 * Manages character animation state
 * Now uses centralized enums from AnimationEnums.h
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class JUSTLIVE_API UAnimationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAnimationComponent();

protected:
	virtual void BeginPlay() override;

public:	
	/** Current movement state */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	EMovementState MovementState = EMovementState::Idle;

	/** Current stance */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	EStance Stance = EStance::Standing;

	/** Current cover state */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	ECoverState CoverState = ECoverState::None;

	/** Current combat state */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	ECombatState CombatState = ECombatState::Unarmed;

	/** Current swimming state */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	ESwimmingState SwimmingState = ESwimmingState::None;

	/** Set movement state */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void SetMovementState(EMovementState NewState);

	/** Set stance */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void SetStance(EStance NewStance);

	/** Check if in specific movement state */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	bool IsInMovementState(EMovementState State) const;
};
