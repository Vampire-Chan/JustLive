#include "PedSpawner.h"
#include "../Peds/Ped.h"
#include "../Peds/Components/AttributesComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

APed* UPedSpawner::SpawnPed(const UObject* WorldContextObject, FVector Location, EPedType PedType, FString MeshPath, FLinearColor SkinColor)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Spawn the Ped
	APed* NewPed = World->SpawnActor<APed>(APed::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
	if (NewPed)
	{
		// Set Type
		if (NewPed->AttributesComp)
		{
			NewPed->AttributesComp->PedType = PedType;
		}

		// Set Group ID based on Type
		switch (PedType)
		{
		case EPedType::Player: 
		case EPedType::PlayerNetwork_1:
		case EPedType::PlayerNetwork_2:
		case EPedType::PlayerNetwork_3:
		case EPedType::PlayerNetwork_4:
		case EPedType::PlayerNetwork_5:
		case EPedType::PlayerNetwork_6:
		case EPedType::PlayerNetwork_7:
		case EPedType::PlayerNetwork_8:
			NewPed->GroupID = FName("Player"); 
			break;
		case EPedType::CoOpPlayer: NewPed->GroupID = FName("CoopPlayer"); break;
		case EPedType::Police: NewPed->GroupID = FName("Police"); break;
		case EPedType::Commando: NewPed->GroupID = FName("Commando"); break;
		case EPedType::Military: NewPed->GroupID = FName("Military"); break;
		case EPedType::Paramedic: NewPed->GroupID = FName("Medic"); break;
		case EPedType::Firefighter: NewPed->GroupID = FName("Fireman"); break;
		case EPedType::Gang1: NewPed->GroupID = FName("Gang1"); break;
		case EPedType::Gang2: NewPed->GroupID = FName("Gang2"); break;
		case EPedType::Gang3: NewPed->GroupID = FName("Gang3"); break;
		case EPedType::Gang4: NewPed->GroupID = FName("Gang4"); break;
		case EPedType::Gang5: NewPed->GroupID = FName("Gang5"); break;
		case EPedType::Gang6: NewPed->GroupID = FName("Gang6"); break;
		case EPedType::Gang7: NewPed->GroupID = FName("Gang7"); break;
		case EPedType::Gang8: NewPed->GroupID = FName("Gang8"); break;
		case EPedType::Civilian: NewPed->GroupID = FName("Civilian"); break;
		default: NewPed->GroupID = FName("Civilian"); break;
		}

		// Set Visuals
		NewPed->SetSkin(SkinColor);

		// Load Mesh if provided
		if (!MeshPath.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("PedSpawner: Mesh loading from path '%s' not yet implemented."), *MeshPath);
		}
	}

	return NewPed;
}
