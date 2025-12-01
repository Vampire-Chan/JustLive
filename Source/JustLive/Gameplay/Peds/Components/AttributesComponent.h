#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributesComponent.generated.h"

UENUM(BlueprintType)
enum class EPedType : uint8
{
	Invalid,
	Player,
	PlayerNetwork_1,
	PlayerNetwork_2,
	PlayerNetwork_3,
	PlayerNetwork_4,
	PlayerNetwork_5,
	PlayerNetwork_6,
	PlayerNetwork_7,
	PlayerNetwork_8,
	CoOpPlayer,
	Police,
	Commando,
	Military,
	Paramedic,
	Firefighter,
	Gang1,
	Gang2,
	Gang3,
	Gang4,
	Gang5,
	Gang6,
	Gang7,
	Gang8,
	Civilian
};

UENUM(BlueprintType)
enum class EPedGender : uint8
{
	Male,
	Female
};

UENUM(BlueprintType)
enum class EAgeGroup : uint8
{
	Child,		// 0-13
	Teen,		// 13-17
	Young,		// 18-25
	Adult,		// 26-35
	Mature,		// 36-50
	Old			// 51-100
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class JUSTLIVE_API UAttributesComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAttributesComponent();

protected:
	virtual void BeginPlay() override;

public:	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stats")
	float Health = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stats")
	float Armor = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stats")
	EPedType PedType = EPedType::Civilian;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stats")
	EPedGender Gender = EPedGender::Male;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stats")
	EAgeGroup AgeGroup = EAgeGroup::Adult;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flags")
	bool bIsInvincible = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Flags")
	bool bIsFrozen = false;

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void TakeDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	bool IsDead() const;
};
