#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PedGroup.generated.h"

class APed;

/**
 * Ped Group (Squad)
 * Manages a specific team of Peds (Leader + Followers).
 */
UCLASS(BlueprintType)
class JUSTLIVE_API UPedGroup : public UObject
{
	GENERATED_BODY()

public:
	UPedGroup();

	// The Leader of this squad
	UPROPERTY(BlueprintReadOnly, Category = "Squad")
	APed* Leader;

	// List of members (including Leader)
	UPROPERTY(BlueprintReadOnly, Category = "Squad")
	TArray<APed*> Members;

	/**
	 * Sets the leader of the squad.
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad")
	void SetLeader(APed* NewLeader);

	/**
	 * Adds a member to the squad.
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad")
	void AddMember(APed* NewMember);

	/**
	 * Removes a member from the squad.
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad")
	void RemoveMember(APed* Member);

	/**
	 * Checks if the squad has a valid leader.
	 */
	UFUNCTION(BlueprintCallable, Category = "Squad")
	bool HasLeader() const;
};
