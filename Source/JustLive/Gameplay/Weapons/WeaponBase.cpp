#include "WeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectileBase.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "../Peds/Ped.h"
#include "Engine/DamageEvents.h"
#include "../../Managers/PhysicsManager.h"

AWeaponBase::AWeaponBase()
{
 	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;

	WeaponMeshSkeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshSkeletal"));
	WeaponMeshSkeletal->SetupAttachment(RootComponent);
	WeaponMeshSkeletal->SetVisibility(false); // Hidden by default

	MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	MuzzleLocation->SetupAttachment(WeaponMesh);
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();
}

void AWeaponBase::Initialize(const FWeaponInfo& Info, FWeaponInstance* Instance)
{
	WeaponInfo = Info;
	WeaponInstance = Instance;

	// Update Mesh
	if (WeaponInfo.WeaponSkeletalMesh)
	{
		// Use Skeletal Mesh
		WeaponMesh->SetVisibility(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		WeaponMeshSkeletal->SetVisibility(true);
		WeaponMeshSkeletal->SetSkeletalMesh(WeaponInfo.WeaponSkeletalMesh);
		
		// Re-attach Muzzle to Skeletal Mesh
		if (WeaponInfo.MuzzleSocket != NAME_None)
		{
			MuzzleLocation->AttachToComponent(WeaponMeshSkeletal, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponInfo.MuzzleSocket);
		}
	}
	else if (WeaponInfo.WeaponMesh)
	{
		// Use Static Mesh
		WeaponMeshSkeletal->SetVisibility(false);
		
		WeaponMesh->SetVisibility(true);
		WeaponMesh->SetStaticMesh(WeaponInfo.WeaponMesh);
		
		if (WeaponInfo.MuzzleSocket != NAME_None)
		{
			MuzzleLocation->AttachToComponent(WeaponMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponInfo.MuzzleSocket);
		}
	}
	else
	{
		// Fallback to Pillar if no mesh provided (as per user request)
		static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/StarterContent/Architecture/Pillar_50x500.Pillar_50x500"));
		if (MeshAsset.Succeeded())
		{
			WeaponMesh->SetStaticMesh(MeshAsset.Object);
			// Smaller scale as requested
			WeaponMesh->SetRelativeScale3D(FVector(0.05f, 0.05f, 0.3f)); 
			WeaponMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
		}
	}
}
// --- Actions ---

void AWeaponBase::StartFire()
{
	if (!CanFire()) 
	{
		// Play Empty Sound
		if (WeaponInfo.SoundEmpty)
		{
			UGameplayStatics::PlaySoundAtLocation(this, WeaponInfo.SoundEmpty, GetActorLocation());
		}
		return;
	}

	bIsFiring = true;

	// Fire immediately
	Fire();

	// Handle Auto Fire
	if (WeaponInfo.FireType == EFireType::Automatic)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_Fire, this, &AWeaponBase::Fire, WeaponInfo.FireRate, true);
	}
}

void AWeaponBase::StopFire()
{
	bIsFiring = false;
	GetWorldTimerManager().ClearTimer(TimerHandle_Fire);
}

void AWeaponBase::StartAim()
{
	bIsAiming = true;
}

void AWeaponBase::StopAim()
{
	bIsAiming = false;
}

void AWeaponBase::Reload()
{
	// Logic handled by InventoryComponent usually, but we can play effects here
	if (WeaponInfo.SoundReload)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponInfo.SoundReload, GetActorLocation());
	}
	
	if (WeaponInfo.AnimReload && MyOwner)
	{
		// Play Anim on Ped
		// MyOwner->PlayAnimMontage(WeaponInfo.AnimReload);
	}
}

void AWeaponBase::AttachToPed(APed* Ped)
{
	MyOwner = Ped;
	if (MyOwner && MyOwner->GetMesh())
	{
		// Attach to Right Hand by default, or use a specific socket defined in WeaponInfo?
		// For now, assume "weapon_r" socket exists (as per our skeleton spec)
		FName SocketName = TEXT("weapon_r");
		AttachToComponent(MyOwner->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
	}
}

void AWeaponBase::DetachFromPed()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	MyOwner = nullptr;
}

// --- Firing Logic ---

void AWeaponBase::Fire()
{
	if (!CanFire())
	{
		StopFire();
		return;
	}

	// Consume Ammo (Update Instance)
	if (WeaponInstance)
	{
		WeaponInstance->AmmoInClip--;
	}

	// Play Sound
	if (WeaponInfo.SoundFire)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponInfo.SoundFire, GetActorLocation());
	}

	// Play VFX
	if (WeaponInfo.VFXMuzzleFlash)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(WeaponInfo.VFXMuzzleFlash, MuzzleLocation, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
	}

	// Fire Bullet or Projectile
	if (WeaponInfo.ProjectileClass)
	{
		FireProjectile();
	}
	else
	{
		FireBullet();
	}
}

void AWeaponBase::FireBullet()
{
	FVector MuzzleStart = GetMuzzleLocation();
	FVector ShootDir = GetMuzzleRotation().Vector();
	FVector TargetPoint = MuzzleStart + (ShootDir * WeaponInfo.Range);

	// 1. Calculate Realistic Target Point (Camera Trace)
	if (MyOwner && MyOwner->IsPlayerControlled() && MyOwner->GetController())
	{
		FVector CamLoc;
		FRotator CamRot;
		MyOwner->GetController()->GetPlayerViewPoint(CamLoc, CamRot);

		FVector CamEnd = CamLoc + (CamRot.Vector() * WeaponInfo.Range);
		
		FHitResult CamHit;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(MyOwner);

		if (GetWorld()->LineTraceSingleByChannel(CamHit, CamLoc, CamEnd, ECC_Visibility, QueryParams))
		{
			TargetPoint = CamHit.ImpactPoint;
		}
		else
		{
			TargetPoint = CamEnd;
		}
	}

	// 2. Muzzle Trace (Actual Bullet Path)
	// Check if something blocks the path from Muzzle to TargetPoint
	FVector BulletDir = (TargetPoint - MuzzleStart).GetSafeNormal();
	
	// Apply Spread
	if (WeaponInfo.Spread > 0.0f)
	{
		BulletDir = FMath::VRandCone(BulletDir, FMath::DegreesToRadians(WeaponInfo.Spread));
	}

	FVector BulletEnd = MuzzleStart + (BulletDir * WeaponInfo.Range);

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (MyOwner) QueryParams.AddIgnoredActor(MyOwner);

	if (GetWorld()->LineTraceSingleByChannel(Hit, MuzzleStart, BulletEnd, ECC_Visibility, QueryParams))
	{
		// Apply Damage
		UGameplayStatics::ApplyPointDamage(Hit.GetActor(), WeaponInfo.Damage, BulletDir, Hit, MyOwner ? MyOwner->GetController() : nullptr, this, UDamageType::StaticClass());

		// Apply Physics Impulse via Manager
		if (UPhysicsManager* PhysMgr = GetWorld()->GetSubsystem<UPhysicsManager>())
		{
			// Force = Damage * Multiplier (Simplification)
			float ForceMagnitude = WeaponInfo.Damage * 1000.0f; 
			PhysMgr->ApplyImpulseToActor(Hit.GetActor(), BulletDir * ForceMagnitude, Hit.ImpactPoint, Hit.BoneName);
		}

		// Play Impact VFX
		if (WeaponInfo.VFXImpact)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), WeaponInfo.VFXImpact, Hit.Location, Hit.ImpactNormal.Rotation());
		}
	}
}

void AWeaponBase::FireProjectile()
{
	if (WeaponInfo.ProjectileClass)
	{
		// Calculate Direction similar to FireBullet
		FVector MuzzleStart = GetMuzzleLocation();
		FVector TargetPoint = MuzzleStart + (GetMuzzleRotation().Vector() * WeaponInfo.Range);

		if (MyOwner && MyOwner->IsPlayerControlled() && MyOwner->GetController())
		{
			FVector CamLoc;
			FRotator CamRot;
			MyOwner->GetController()->GetPlayerViewPoint(CamLoc, CamRot);
			FVector CamEnd = CamLoc + (CamRot.Vector() * WeaponInfo.Range);
			
			FHitResult CamHit;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			QueryParams.AddIgnoredActor(MyOwner);

			if (GetWorld()->LineTraceSingleByChannel(CamHit, CamLoc, CamEnd, ECC_Visibility, QueryParams))
			{
				TargetPoint = CamHit.ImpactPoint;
			}
			else
			{
				TargetPoint = CamEnd;
			}
		}

		FRotator SpawnRotation = (TargetPoint - MuzzleStart).Rotation();
		FTransform SpawnTransform(SpawnRotation, MuzzleStart);
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = MyOwner;

		AProjectileBase* Projectile = GetWorld()->SpawnActor<AProjectileBase>(WeaponInfo.ProjectileClass, SpawnTransform, SpawnParams);
		if (Projectile)
		{
			Projectile->Damage = WeaponInfo.Damage;
			// Projectile handles its own physics via MovementComponent
		}
	}
}

// --- Getters ---

int32 AWeaponBase::GetCurrentAmmo() const
{
	return WeaponInstance ? WeaponInstance->AmmoInClip : 0;
}

int32 AWeaponBase::GetMaxClipSize() const
{
	return WeaponInfo.ClipSize;
}

bool AWeaponBase::CanFire() const
{
	return GetCurrentAmmo() > 0;
}

// --- Helpers ---

FVector AWeaponBase::GetMuzzleLocation() const
{
	return MuzzleLocation ? MuzzleLocation->GetComponentLocation() : GetActorLocation();
}

FRotator AWeaponBase::GetMuzzleRotation() const
{
	if (MyOwner && MyOwner->IsPlayerControlled())
	{
		// If player, shoot towards camera aim?
		// For now, use muzzle rotation
	}
	return MuzzleLocation ? MuzzleLocation->GetComponentRotation() : GetActorRotation();
}
