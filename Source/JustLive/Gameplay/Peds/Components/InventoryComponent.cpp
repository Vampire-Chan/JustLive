#include "InventoryComponent.h"
#include "../../Weapons/WeaponBase.h"
#include "../Ped.h"
#include "Engine/World.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

// --- Weapon Management ---

void UInventoryComponent::AddWeapon(const FWeaponInfo& WeaponInfo, int32 AmmoReserve)
{
	// Create Instance on Heap
	TSharedPtr<FWeaponInstance> NewInstance = MakeShared<FWeaponInstance>();
	NewInstance->WeaponId = WeaponInfo.Name;
	NewInstance->AmmoInClip = WeaponInfo.ClipSize; // Full clip by default
	NewInstance->AmmoInReserve = AmmoReserve;
	NewInstance->CachedInfo = WeaponInfo; // Cache the info
	
	// Add to Inventory (Overwrite if exists)
	Inventory.Add(WeaponInfo.Slot, NewInstance);

	// If we are unarmed or this is the first weapon, equip it? 
	if (CurrentSlot == WeaponInfo.Slot)
	{
		EquipWeaponSlot(WeaponInfo.Slot);
	}
}

void UInventoryComponent::RemoveWeapon(EWeaponSlot Slot)
{
	if (Inventory.Contains(Slot))
	{
		// If equipped, unequip first
		if (CurrentSlot == Slot)
		{
			if (CurrentWeaponActor)
			{
				CurrentWeaponActor->DetachFromPed();
				CurrentWeaponActor->Destroy();
				CurrentWeaponActor = nullptr;
			}
			CurrentSlot = EWeaponSlot::Unarmed;
		}
		
		Inventory.Remove(Slot);
	}
}

void UInventoryComponent::EquipWeaponSlot(EWeaponSlot Slot)
{
	// 1. Check if we have a weapon in this slot
	if (!Inventory.Contains(Slot) && Slot != EWeaponSlot::Unarmed)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipWeaponSlot: No weapon in slot %d"), (int32)Slot);
		return;
	}

	// 2. Destroy current weapon
	if (CurrentWeaponActor)
	{
		CurrentWeaponActor->DetachFromPed();
		CurrentWeaponActor->Destroy();
		CurrentWeaponActor = nullptr;
		
		// Update previous instance pointer to null
		if (Inventory.Contains(CurrentSlot))
		{
			Inventory[CurrentSlot]->WeaponActor = nullptr;
		}
	}

	// 3. Update Slot
	CurrentSlot = Slot;

	// 4. Spawn new weapon (if not Unarmed)
	if (Slot != EWeaponSlot::Unarmed)
	{
		TSharedPtr<FWeaponInstance> Instance = Inventory[Slot];
		if (Instance.IsValid())
		{
			SpawnWeaponActor(*Instance.Get(), Instance->CachedInfo);
		}
	}
}

bool UInventoryComponent::GetWeaponInSlot(EWeaponSlot Slot, FWeaponInstance& OutInstance) const
{
	if (const TSharedPtr<FWeaponInstance>* Found = Inventory.Find(Slot))
	{
		if (Found->IsValid())
		{
			OutInstance = *(*Found).Get();
			return true;
		}
	}
	return false;
}

// --- Ammo Management ---

void UInventoryComponent::AddAmmo(EWeaponSlot Slot, int32 Amount)
{
	if (TSharedPtr<FWeaponInstance>* InstancePtr = Inventory.Find(Slot))
	{
		if (FWeaponInstance* Instance = InstancePtr->Get())
		{
			Instance->AmmoInReserve += Amount;
			if (Instance->AmmoInReserve > 9999) Instance->AmmoInReserve = 9999;
			OnAmmoChanged.Broadcast(Instance->AmmoInClip, Instance->AmmoInReserve);
		}
	}
}

bool UInventoryComponent::ConsumeAmmo(int32 Amount)
{
	if (CurrentWeaponActor && CurrentWeaponActor->WeaponInstance)
	{
		if (CurrentWeaponActor->WeaponInstance->AmmoInClip >= Amount)
		{
			CurrentWeaponActor->WeaponInstance->AmmoInClip -= Amount;
			OnAmmoChanged.Broadcast(CurrentWeaponActor->WeaponInstance->AmmoInClip, CurrentWeaponActor->WeaponInstance->AmmoInReserve);
			return true;
		}
	}
	return false;
}

void UInventoryComponent::ReloadWeapon()
{
	if (!CurrentWeaponActor || !CurrentWeaponActor->WeaponInstance) return;

	FWeaponInstance* Instance = CurrentWeaponActor->WeaponInstance;
	const FWeaponInfo& Info = CurrentWeaponActor->WeaponInfo;

	if (Instance->AmmoInClip >= Info.ClipSize) return; // Full
	if (Instance->AmmoInReserve <= 0) return; // No reserve

	int32 Needed = Info.ClipSize - Instance->AmmoInClip;
	int32 ToTransfer = FMath::Min(Needed, Instance->AmmoInReserve);

	Instance->AmmoInReserve -= ToTransfer;
	Instance->AmmoInClip += ToTransfer;

	CurrentWeaponActor->Reload(); // Play anim/sound
	
	OnAmmoChanged.Broadcast(Instance->AmmoInClip, Instance->AmmoInReserve);
}

bool UInventoryComponent::NeedsReload() const
{
	if (CurrentWeaponActor && CurrentWeaponActor->WeaponInstance)
	{
		return CurrentWeaponActor->WeaponInstance->AmmoInClip == 0;
	}
	return false;
}

void UInventoryComponent::SpawnWeaponActor(FWeaponInstance& Instance, const FWeaponInfo& Info)
{
	if (APed* Ped = Cast<APed>(GetOwner()))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Ped;
		SpawnParams.Instigator = Ped;
		
		// Spawn Blueprint Class if provided in Info?
		// Info has `UStaticMesh* WeaponMesh`.
		// We spawn `AWeaponBase` and set mesh.
		// Or if we want custom classes, we need `TSubclassOf<AWeaponBase>` in Info.
		// I'll assume standard `AWeaponBase` for now, or use a class from Info if I add it.
		// Structs.h `FWeaponInfo` does NOT have `WeaponClass`. It has `ProjectileClass`.
		// So we spawn `AWeaponBase`.
		
		AWeaponBase* NewWeapon = GetWorld()->SpawnActor<AWeaponBase>(AWeaponBase::StaticClass(), FTransform::Identity, SpawnParams);
		if (NewWeapon)
		{
			NewWeapon->Initialize(Info, &Instance);
			NewWeapon->AttachToPed(Ped);
			
			CurrentWeaponActor = NewWeapon;
			Instance.WeaponActor = NewWeapon;
			
			OnWeaponChanged.Broadcast(NewWeapon, nullptr);
			OnAmmoChanged.Broadcast(Instance.AmmoInClip, Instance.AmmoInReserve);
		}
	}
}
