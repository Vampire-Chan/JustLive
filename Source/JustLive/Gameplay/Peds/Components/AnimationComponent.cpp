#include "AnimationComponent.h"

UAnimationComponent::UAnimationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAnimationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAnimationComponent::SetMovementState(EMovementState NewState)
{
	MovementState = NewState;
}

void UAnimationComponent::SetStance(EStance NewStance)
{
	Stance = NewStance;
}

bool UAnimationComponent::IsInMovementState(EMovementState State) const
{
	return MovementState == State;
}
