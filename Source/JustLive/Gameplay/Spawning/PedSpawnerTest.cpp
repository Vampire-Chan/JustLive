#include "PedSpawnerTest.h"
#include "PedSpawner.h"
#include "../Peds/Ped.h"
#include "../Groups/PedGroup.h"
#include "../Peds/Components/AttributesComponent.h"

APedSpawnerTest::APedSpawnerTest()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APedSpawnerTest::BeginPlay()
{
	Super::BeginPlay();
	
	// Auto-spawn on start
	SpawnTestPeds();
}

void APedSpawnerTest::SpawnTestPeds()
{
	// Spawn Squads for major types
	CreateSquadForType(EPedType::Police, CountPerType);
	CreateSquadForType(EPedType::Gang1, CountPerType);
	CreateSquadForType(EPedType::Civilian, CountPerType);
	CreateSquadForType(EPedType::Military, CountPerType);
}

void APedSpawnerTest::CreateSquadForType(EPedType Type, int32 Count)
{
	if (Count <= 0) return;

	// Create a Squad
	UPedGroup* Squad = NewObject<UPedGroup>(this);
	
	for (int32 i = 0; i < Count; i++)
	{
		// Random Location in Area
		FVector Origin = GetActorLocation();
		FVector RandomLoc = Origin + FMath::RandPointInBox(FBox(-SpawnArea, SpawnArea));
		RandomLoc.Z = Origin.Z + 100.0f; // Slight offset up

		// Random Color
		FLinearColor RandomColor = FLinearColor::MakeRandomColor();

		// Spawn
		APed* NewPed = UPedSpawner::SpawnPed(this, RandomLoc, Type, "", RandomColor);
		if (NewPed)
		{
			// Add to Squad
			Squad->AddMember(NewPed);

			// First one is Leader
			if (i == 0)
			{
				Squad->SetLeader(NewPed);
			}
		}
	}
}
