#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSequence.h"
#include "AnimationEnums.h"
#include "Curves/CurveFloat.h"
#include "AnimationStructs.generated.h"

/**
 * Animation Data Structures for Pure C++ Animation System
 */

// ============================================================================
// ANIMATION REQUEST (GTA5-Style API)
// ============================================================================

USTRUCT(BlueprintType)
struct JUSTLIVE_API FAnimationRequest
{
	GENERATED_BODY()

	// ========== Animation Data ==========
	
	/** Animation sequence to play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* Animation = nullptr;

	/** Which layer to play this animation in */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAnimationLayer Layer = EAnimationLayer::BaseLocomotion;

	// ========== Blending ==========
	
	/** How to blend this animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAnimBlendMode BlendMode = EAnimBlendMode::Blend;

	/** Time to blend in (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BlendInTime = 0.2f;

	/** Time to blend out (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BlendOutTime = 0.2f;

	/** Blend in curve type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBlendCurveType BlendInCurve = EBlendCurveType::EaseInOut;

	/** Blend out curve type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBlendCurveType BlendOutCurve = EBlendCurveType::EaseInOut;

	/** Custom blend in curve (if BlendInCurve == Custom) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* CustomBlendInCurve = nullptr;

	/** Custom blend out curve (if BlendOutCurve == Custom) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* CustomBlendOutCurve = nullptr;

	// ========== Playback Control ==========
	
	/** Playback speed multiplier (1.0 = normal, 2.0 = double speed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate = 1.0f;

	/** Starting position in animation (0.0 to 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartPosition = 0.0f;

	/** If true, sync playback speed with movement speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSyncWithMovementSpeed = false;

	/** Movement speed at which animation plays at 1.0x (for syncing) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ReferenceMovementSpeed = 600.0f;

	// ========== Flags ==========
	
	/** Animation behavior flags (bitfield) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "EAnimationFlags"))
	int32 Flags = 0;

	// ========== Priority ==========
	
	/** Animation priority (higher = more important) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = static_cast<int32>(EAnimationPriority::Normal);

	// ========== Bone Masking ==========
	
	/** Which bones this animation affects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBoneMaskPreset BoneMask = EBoneMaskPreset::FullBody;

	/** Custom bone names (if BoneMask == Custom) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> CustomBoneNames;

	/** Layer weight (0.0 to 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LayerWeight = 1.0f;

	// ========== Animation Context ==========
	
	/** What type of object is being animated */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAnimationContext AnimationContext = EAnimationContext::Character;

	/** Vehicle animation type (if Context == Vehicle) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EVehicleAnimationType VehicleAnimType = EVehicleAnimationType::DoorFrontLeft;

	/** Weapon detail animation (if Context == Weapon) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponDetailAnimation WeaponDetailAnim = EWeaponDetailAnimation::BoltCycle;

	/** World object animation (if Context == WorldObject) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWorldObjectAnimation WorldObjectAnim = EWorldObjectAnimation::DoorOpen;

	/** Target bone/socket for detail animations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName TargetBone = NAME_None;

	// ========== Pooling ==========
	
	/** Use animation pooling (recommended for NPCs) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUsePooling = true;

	/** Pool slot index (set by pool manager) */
	int32 PoolSlotIndex = INDEX_NONE;

	// ========== Callbacks ==========
	
	/** Called when animation completes */
	TFunction<void()> OnComplete;

	/** Called when animation is interrupted */
	TFunction<void()> OnInterrupted;

	/** Called every frame while playing */
	TFunction<void(float)> OnUpdate;

	// ========== Helper Functions ==========
	
	bool HasFlag(EAnimationFlags Flag) const
	{
		return (Flags & static_cast<int32>(Flag)) != 0;
	}

	void AddFlag(EAnimationFlags Flag)
	{
		Flags |= static_cast<int32>(Flag);
	}

	void RemoveFlag(EAnimationFlags Flag)
	{
		Flags &= ~static_cast<int32>(Flag);
	}

	// Quick setup helpers
	static FAnimationRequest FullBodyLocked(UAnimSequence* Anim, float BlendIn = 0.2f)
	{
		FAnimationRequest Request;
		Request.Animation = Anim;
		Request.Layer = EAnimationLayer::FullBodyOverride;
		Request.BlendInTime = BlendIn;
		Request.Flags = static_cast<int32>(EAnimationFlags::FullBodyLocked);
		Request.Priority = static_cast<int32>(EAnimationPriority::High);
		return Request;
	}

	static FAnimationRequest UpperBodyOnly(UAnimSequence* Anim, float BlendIn = 0.2f)
	{
		FAnimationRequest Request;
		Request.Animation = Anim;
		Request.Layer = EAnimationLayer::UpperBody;
		Request.BlendInTime = BlendIn;
		Request.BoneMask = EBoneMaskPreset::UpperBody;
		Request.Flags = static_cast<int32>(EAnimationFlags::Interruptible);
		return Request;
	}

	static FAnimationRequest Additive(UAnimSequence* Anim, EAnimationLayer Layer = EAnimationLayer::AimOffset)
	{
		FAnimationRequest Request;
		Request.Animation = Anim;
		Request.Layer = Layer;
		Request.BlendMode = EAnimBlendMode::Additive;
		Request.BlendInTime = 0.1f;
		Request.BlendOutTime = 0.1f;
		return Request;
	}

	// ========== Specialized Animation Helpers ==========

	/** Create vehicle animation request */
	static FAnimationRequest VehicleAnimation(UAnimSequence* Anim, EVehicleAnimationType VehicleType, FName BoneName = NAME_None)
	{
		FAnimationRequest Request;
		Request.Animation = Anim;
		Request.AnimationContext = EAnimationContext::Vehicle;
		Request.VehicleAnimType = VehicleType;
		Request.TargetBone = BoneName;
		Request.Layer = EAnimationLayer::FullBodyOverride;
		Request.bUsePooling = false; // Vehicle anims usually don't need pooling
		return Request;
	}

	/** Create weapon detail animation request */
	static FAnimationRequest WeaponDetailAnimation(UAnimSequence* Anim, EWeaponDetailAnimation DetailType, FName BoneName = NAME_None)
	{
		FAnimationRequest Request;
		Request.Animation = Anim;
		Request.AnimationContext = EAnimationContext::Weapon;
		Request.WeaponDetailAnim = DetailType;
		Request.TargetBone = BoneName;
		Request.Layer = EAnimationLayer::RightArm; // Most weapon anims on right arm
		Request.BlendInTime = 0.05f; // Quick blend for weapon details
		Request.BlendOutTime = 0.05f;
		return Request;
	}

	/** Create world object animation request */
	static FAnimationRequest WorldObjectAnimation(UAnimSequence* Anim, EWorldObjectAnimation ObjectType)
	{
		FAnimationRequest Request;
		Request.Animation = Anim;
		Request.AnimationContext = EAnimationContext::WorldObject;
		Request.WorldObjectAnim = ObjectType;
		Request.Layer = EAnimationLayer::FullBodyOverride;
		Request.Flags = static_cast<int32>(EAnimationFlags::LockMovement | EAnimationFlags::UseRootMotion);
		return Request;
	}
};

// ============================================================================
// ANIMATION LAYER STATE (Internal)
// ============================================================================

USTRUCT()
struct JUSTLIVE_API FAnimationLayerState
{
	GENERATED_BODY()

	/** Layer this state belongs to */
	EAnimationLayer Layer = EAnimationLayer::BaseLocomotion;

	/** Current animation playing */
	UPROPERTY()
	UAnimSequence* CurrentAnimation = nullptr;

	/** Current playback time (seconds) */
	float CurrentTime = 0.0f;

	/** Current blend weight (0.0 to 1.0) */
	float BlendWeight = 0.0f;

	/** Target blend weight (for blending in/out) */
	float TargetBlendWeight = 1.0f;

	/** Playback rate */
	float PlayRate = 1.0f;

	/** Animation flags */
	int32 Flags = 0;

	/** Priority */
	int32 Priority = 0;

	/** Bone mask */
	EBoneMaskPreset BoneMask = EBoneMaskPreset::FullBody;

	/** Custom bone names */
	TArray<FName> CustomBoneNames;

	/** Blend mode */
	EAnimBlendMode BlendMode = EAnimBlendMode::Blend;

	// Blending state
	bool bIsBlendingIn = false;
	bool bIsBlendingOut = false;
	float BlendInTimeRemaining = 0.0f;
	float BlendOutTimeRemaining = 0.0f;
	float BlendInDuration = 0.2f;
	float BlendOutDuration = 0.2f;
	EBlendCurveType BlendInCurve = EBlendCurveType::Linear;
	EBlendCurveType BlendOutCurve = EBlendCurveType::Linear;
	UCurveFloat* CustomBlendInCurve = nullptr;
	UCurveFloat* CustomBlendOutCurve = nullptr;

	// Playback state
	bool bIsPlaying = false;
	bool bIsLooping = false;
	bool bSyncWithMovementSpeed = false;
	float ReferenceMovementSpeed = 600.0f;

	// Callbacks
	TFunction<void()> OnComplete;
	TFunction<void()> OnInterrupted;
	TFunction<void(float)> OnUpdate;

	// Helper functions
	bool HasFlag(EAnimationFlags Flag) const
	{
		return (Flags & static_cast<int32>(Flag)) != 0;
	}

	float GetAnimationLength() const
	{
		return CurrentAnimation ? CurrentAnimation->GetPlayLength() : 0.0f;
	}

	float GetNormalizedTime() const
	{
		float Length = GetAnimationLength();
		return Length > 0.0f ? CurrentTime / Length : 0.0f;
	}

	bool IsFinished() const
	{
		return !bIsLooping && GetNormalizedTime() >= 1.0f;
	}
};

// ============================================================================
// LOCOMOTION BLEND DATA
// ============================================================================

USTRUCT(BlueprintType)
struct JUSTLIVE_API FLocomotionBlendData
{
	GENERATED_BODY()

	/** Idle animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* IdleAnim = nullptr;

	/** Walk forward */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* WalkFwdAnim = nullptr;

	/** Walk backward */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* WalkBwdAnim = nullptr;

	/** Walk left */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* WalkLeftAnim = nullptr;

	/** Walk right */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* WalkRightAnim = nullptr;

	/** Run forward */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* RunFwdAnim = nullptr;

	/** Sprint forward */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* SprintFwdAnim = nullptr;

	/** Speed thresholds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WalkSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RunSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SprintSpeed = 900.0f;
};

// ============================================================================
// JUMP ANIMATION SET
// ============================================================================

USTRUCT(BlueprintType)
struct JUSTLIVE_API FJumpAnimationSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* JumpStartAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* JumpLoopAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* FallAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* LandAnim = nullptr;
};
