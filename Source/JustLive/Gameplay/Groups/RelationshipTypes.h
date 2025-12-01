#pragma once

#include "CoreMinimal.h"
#include "RelationshipTypes.generated.h"

UENUM(BlueprintType)
enum class ERelationship : uint8
{
	Hate,       // Attack on sight
	Dislike,    // Verbal abuse, will attack if provoked
	Neutral,    // Ignore
	Like,       // Friendly greetings, helpful
	Respect,    // Will help if attacked
	Companion   // Follows and protects
};

USTRUCT(BlueprintType)
struct FRelationshipGroup
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName GroupName;

	// Map of Other Group Name -> Relationship Level
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, ERelationship> Relationships;
};
