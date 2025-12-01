#include "AttributesComponent.h"

UAttributesComponent::UAttributesComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAttributesComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAttributesComponent::TakeDamage(float Amount)
{
	if (bIsInvincible) return;

	if (Armor > 0.0f)
	{
		float ArmorDamage = FMath::Min(Armor, Amount);
		Armor -= ArmorDamage;
		Amount -= ArmorDamage;
	}

	Health = FMath::Clamp(Health - Amount, 0.0f, MaxHealth);
}

bool UAttributesComponent::IsDead() const
{
	return Health <= 0.0f;
}
