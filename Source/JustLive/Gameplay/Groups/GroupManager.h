#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RelationshipTypes.h"
#include "GroupManager.generated.h"

class APed;

/**
 * Group Manager
 * Manages faction standings and relationships between Peds.
 */
UCLASS()
class JUSTLIVE_API UGroupManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UGroupManager* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Sets the relationship between two groups.
	 */
	UFUNCTION(BlueprintCallable, Category = "Groups")
	void SetGroupRelationship(FName GroupA, FName GroupB, ERelationship Relationship);

	/**
	 * Gets the relationship between two Peds based on their Groups.
	 */
	UFUNCTION(BlueprintCallable, Category = "Groups")
	ERelationship GetRelationship(APed* PedA, APed* PedB) const;

	/**
	 * Gets the relationship between two Groups.
	 */
	UFUNCTION(BlueprintCallable, Category = "Groups")
	ERelationship GetGroupRelationship(FName GroupA, FName GroupB) const;

	/**
	 * Creates a new relationship group.
	 */
	UFUNCTION(BlueprintCallable, Category = "Groups")
	void CreateGroup(FName GroupName);

	/**
	 * Removes a relationship group.
	 */
	UFUNCTION(BlueprintCallable, Category = "Groups")
	void RemoveGroup(FName GroupName);

private:
	// Map of GroupName -> GroupData
	UPROPERTY()
	TMap<FName, FRelationshipGroup> Groups;
};
