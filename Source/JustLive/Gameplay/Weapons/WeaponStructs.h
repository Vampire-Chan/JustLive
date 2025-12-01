#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WeaponEnums.h"
#include "WeaponStructs.generated.h"

class AProjectileBase;
class USoundBase;
class UParticleSystem;
class UAnimSequence;
class UStaticMesh;
class UNiagaraSystem;

/**
 * Static Weapon Data (Data Table Row)
 * Defines the properties of a weapon type.
 */
USTRUCT(BlueprintType)
struct FWeaponInfo : public FTableRowBase
{
	GENERATED_BODY()

	// --- Identity ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	UTexture2D* Icon;

	// --- Classification ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classification")
	EWeaponSlot Slot = EWeaponSlot::Primary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classification", meta = (Bitmask, BitmaskEnum = "EWeaponFlags"))
	int32 Flags = 0; // Use int32 for bitmask support in editor

	// --- Combat Stats ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Damage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Range = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float FireRate = 0.1f; // Seconds between shots

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Spread = 1.0f; // Accuracy deviation

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Recoil = 1.0f; // Kickback

	// --- Ammo ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	EAmmoType AmmoType = EAmmoType::Rifle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	int32 ClipSize = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	int32 MaxAmmoReserve = 300; // Max ammo in pocket

	// --- Behavior ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	EFireType FireType = EFireType::Automatic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	EWeaponDamageType DamageType = EWeaponDamageType::Bullet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	TSubclassOf<AProjectileBase> ProjectileClass; // For projectiles

	// --- Assets ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	UStaticMesh* WeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	USkeletalMesh* WeaponSkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	FName MuzzleSocket = FName("Muzzle");

	// --- Audio ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundFire;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundReload;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* SoundEmpty;

	// --- VFX ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* VFXMuzzleFlash; // Using Niagara preferred

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	UNiagaraSystem* VFXImpact;

	// --- Animation ---
	// Placeholder for Procedural Animation Data or specific AnimMontages
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimSequence* AnimFire;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimSequence* AnimReload;
	
	// Helper to check flags
	bool HasFlag(EWeaponFlags Flag) const
	{
		return (Flags & static_cast<int32>(Flag)) != 0;
	}
};

/**
 * Dynamic Weapon Instance Data
 * Tracks the state of a specific weapon in inventory.
 */
USTRUCT(BlueprintType)
struct FWeaponInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString WeaponId; // Key to FWeaponInfo

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 AmmoInClip = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 AmmoInReserve = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FString> Attachments;

	/** Cached Info for spawning (Temporary until DB) */
	UPROPERTY(BlueprintReadOnly)
	FWeaponInfo CachedInfo;

	// Runtime reference to the spawned actor (if equipped)
	UPROPERTY(BlueprintReadOnly)
	class AWeaponBase* WeaponActor = nullptr;
};
