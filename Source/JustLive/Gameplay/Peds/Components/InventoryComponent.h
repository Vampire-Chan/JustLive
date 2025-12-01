#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../../Weapons/WeaponStructs.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponChanged, AWeaponBase*, NewWeapon, AWeaponBase*, OldWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32, ClipAmmo, int32, ReserveAmmo);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class JUSTLIVE_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;

public:	
	// --- Weapon Management ---

	/** Add a weapon to the inventory (replaces existing in slot if any) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddWeapon(const FWeaponInfo& WeaponInfo, int32 AmmoReserve);

	/** Remove weapon from specific slot */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RemoveWeapon(EWeaponSlot Slot);

	/** Equip weapon from specific slot */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void EquipWeaponSlot(EWeaponSlot Slot);

	/** Get currently equipped weapon actor */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	AWeaponBase* GetCurrentWeapon() const { return CurrentWeaponActor; }

	/** Get weapon info for a slot */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetWeaponInSlot(EWeaponSlot Slot, FWeaponInstance& OutInstance) const;

	// --- Ammo Management ---

	/** Add ammo to a specific weapon type/slot */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddAmmo(EWeaponSlot Slot, int32 Amount);

	/** Consume ammo from current weapon (Clip -> Reserve) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ConsumeAmmo(int32 Amount = 1);

	/** Reload current weapon */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ReloadWeapon();

	/** Check if current weapon needs reload */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool NeedsReload() const;

	// --- Events ---
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnWeaponChanged OnWeaponChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnAmmoChanged OnAmmoChanged;

	// --- State ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	AWeaponBase* CurrentWeaponActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	EWeaponSlot CurrentSlot = EWeaponSlot::Unarmed;

private:
	/** Inventory: Slot -> Weapon Instance (Heap Allocated for stability) */
	TMap<EWeaponSlot, TSharedPtr<FWeaponInstance>> Inventory;

	/** Spawns the weapon actor for the given instance */
	void SpawnWeaponActor(FWeaponInstance& Instance, const FWeaponInfo& Info);
};
