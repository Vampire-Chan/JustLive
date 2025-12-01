#include "GroupManager.h"
#include "../Peds/Ped.h"
#include "Kismet/GameplayStatics.h"

UGroupManager* UGroupManager::Get(const UObject* WorldContextObject)
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		return GI->GetSubsystem<UGroupManager>();
	}
	return nullptr;
}

void UGroupManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// Default Groups Initialization
	// These can be modified at runtime via CreateGroup/RemoveGroup
	TArray<FName> DefaultGroups = {
		FName("Police"), FName("Military"), FName("Player"), FName("CoopPlayer"), 
		FName("NetworkPlayer"), FName("Commando"), FName("Medic"), FName("Fireman"), 
		FName("Civilian"), 
		FName("Gang1"), FName("Gang2"), FName("Gang3"), FName("Gang4"), 
		FName("Gang5"), FName("Gang6"), FName("Gang7"), FName("Gang8")
	};

	for (const FName& GroupName : DefaultGroups)
	{
		if (!Groups.Contains(GroupName))
		{
			Groups.Add(GroupName, FRelationshipGroup{ GroupName });
		}
	}

	// Default Relationships
	SetGroupRelationship(FName("Police"), FName("Gang1"), ERelationship::Hate);
	SetGroupRelationship(FName("Gang1"), FName("Police"), ERelationship::Hate);
	SetGroupRelationship(FName("Civilian"), FName("Gang1"), ERelationship::Dislike);
}

void UGroupManager::SetGroupRelationship(FName GroupA, FName GroupB, ERelationship Relationship)
{
	// Ensure groups exist
	if (!Groups.Contains(GroupA)) Groups.Add(GroupA, FRelationshipGroup{ GroupA });
	if (!Groups.Contains(GroupB)) Groups.Add(GroupB, FRelationshipGroup{ GroupB });

	Groups[GroupA].Relationships.Add(GroupB, Relationship);
	
	// Relationships are not necessarily symmetric (A can hate B, but B can be neutral to A),
	// but usually they are. For now, let's keep it asymmetric for flexibility.
}

ERelationship UGroupManager::GetRelationship(APed* PedA, APed* PedB) const
{
	if (!PedA || !PedB) return ERelationship::Neutral;
	if (PedA == PedB) return ERelationship::Like; // Self love

	// Get Groups from Peds
	FName GroupA = PedA->GroupID;
	FName GroupB = PedB->GroupID;

	return GetGroupRelationship(GroupA, GroupB);
}

ERelationship UGroupManager::GetGroupRelationship(FName GroupA, FName GroupB) const
{
	if (GroupA.IsNone() || GroupB.IsNone()) return ERelationship::Neutral;
	if (GroupA == GroupB) return ERelationship::Like; // Same group usually likes each other

	if (const FRelationshipGroup* GroupData = Groups.Find(GroupA))
	{
		if (const ERelationship* Rel = GroupData->Relationships.Find(GroupB))
		{
			return *Rel;
		}
	}

	return ERelationship::Neutral; // Default
}

void UGroupManager::CreateGroup(FName GroupName)
{
	if (!Groups.Contains(GroupName))
	{
		Groups.Add(GroupName, FRelationshipGroup{ GroupName });
		UE_LOG(LogTemp, Log, TEXT("GroupManager: Created group '%s'"), *GroupName.ToString());
	}
}

void UGroupManager::RemoveGroup(FName GroupName)
{
	if (Groups.Remove(GroupName) > 0)
	{
		// Also remove references in other groups?
		// For now, we leave them as they might be re-added later, or we can clean up.
		UE_LOG(LogTemp, Log, TEXT("GroupManager: Removed group '%s'"), *GroupName.ToString());
	}
}
