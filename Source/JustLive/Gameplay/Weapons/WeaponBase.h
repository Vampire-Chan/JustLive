#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponStructs.h"
#include "WeaponBase.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class AProjectileBase;
class APed;

UCLASS()
class JUSTLIVE_API AWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeaponBase();

protected:
	virtual void BeginPlay() override;

public:	
	// --- Initialization ---
	
	/** Initialize weapon with data */
	void Initialize(const FWeaponInfo& Info, FWeaponInstance* Instance);

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* WeaponMeshSkeletal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* MuzzleLocation;

	// --- Data ---
	
	/** Static weapon data */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FWeaponInfo WeaponInfo;

	/** Dynamic instance data (pointer to inventory struct) */
	FWeaponInstance* WeaponInstance;

	// --- State ---
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bIsAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bIsFiring = false;

	// --- Actions ---
	
	virtual void StartFire();
	virtual void StopFire();
	
	virtual void StartAim();
	virtual void StopAim();
	
	virtual void Reload();
	
	void AttachToPed(APed* Ped);
	void DetachFromPed();

	// --- Getters ---
	
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetMaxClipSize() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanFire() const;

protected:
	virtual void Fire();
	virtual void FireBullet();
	virtual void FireProjectile();
	
	FTimerHandle TimerHandle_Fire;
	
	UPROPERTY()
	APed* MyOwner;

	// Helpers
	FVector GetMuzzleLocation() const;
	FRotator GetMuzzleRotation() const;
};
