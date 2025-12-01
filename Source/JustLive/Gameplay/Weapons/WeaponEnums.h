#pragma once

#include "CoreMinimal.h"
#include "WeaponEnums.generated.h"

/**
 * Weapon Slots for Inventory Management
 */
UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
	Unarmed     UMETA(DisplayName = "Unarmed"),
	Melee       UMETA(DisplayName = "Melee"),
	Primary     UMETA(DisplayName = "Primary (Rifle/Shotgun)"),
	Secondary   UMETA(DisplayName = "Secondary (Pistol/SMG)"),
	Heavy       UMETA(DisplayName = "Heavy (RPG/Minigun)"),
	Throwable   UMETA(DisplayName = "Throwable (Grenade)"),
	Special     UMETA(DisplayName = "Special (Gadgets)")
};

/**
 * Weapon Capabilities (Bitflags)
 */
UENUM(BlueprintType, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EWeaponFlags : uint8
{
	None = 0,
	CanAim          = 1 << 0, // Can zoom/aim down sights
	CanThrow        = 1 << 1, // Is a throwable item
	IsSniper        = 1 << 2, // Has high zoom scope
	IsSuppressed    = 1 << 3, // Is silent
	CanTakeCover    = 1 << 4, // Can be used in cover
	TwoHanded       = 1 << 5, // Requires two hands
	Automatic       = 1 << 6, // Full auto fire
	Explosive       = 1 << 7  // Causes explosion
};
ENUM_CLASS_FLAGS(EWeaponFlags);

/**
 * Firing Modes
 */
UENUM(BlueprintType)
enum class EFireType : uint8
{
	Single      UMETA(DisplayName = "Single Shot"),
	Burst       UMETA(DisplayName = "Burst Fire"),
	Automatic   UMETA(DisplayName = "Full Auto"),
	Projectile  UMETA(DisplayName = "Projectile (Physics)"),
	Melee       UMETA(DisplayName = "Melee Hit")
};

/**
 * Damage Types
 */
UENUM(BlueprintType)
enum class EWeaponDamageType : uint8
{
	Bullet,
	Explosive,
	Melee,
	Fire,
	Electric,
	Gas
};

/**
 * Ammo Types
 */
UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	None,
	Pistol,
	Rifle,
	Shotgun,
	Sniper,
	Rocket,
	Grenade,
	Fuel
};
