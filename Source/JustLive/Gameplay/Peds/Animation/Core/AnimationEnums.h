#pragma once

#include "CoreMinimal.h"
#include "AnimationEnums.generated.h"

/**
 * Animation Enumerations for Pure C++ Animation System
 * Modular design - add new states without breaking existing code
 */

// ============================================================================
// MOVEMENT STATES
// ============================================================================

UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Idle,
	Walking,
	Running,
	Sprinting,
	Jumping,
	Falling,
	Landing,
	Sliding,
	Climbing,
	// Add more as needed
};

UENUM(BlueprintType)
enum class EStance : uint8
{
	Standing,
	Crouching,
	Prone,
	// Add more as needed
};

// ============================================================================
// OPTIONAL MODULE STATES (Add when implementing those systems)
// ============================================================================

UENUM(BlueprintType)
enum class ECoverState : uint8
{
	None,
	EnteringCover,
	InCoverLow,
	InCoverHigh,
	PeekingLeft,
	PeekingRight,
	PeekingUp,
	ExitingCover,
	VaultingOver
};

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	Unarmed,
	Armed,
	Aiming,
	Firing,
	Reloading,
	Melee
};

UENUM(BlueprintType)
enum class ESwimmingState : uint8
{
	None,
	SurfaceIdle,
	SurfaceSwimming,
	Diving,
	Underwater,
	Surfacing
};

// ============================================================================
// ANIMATION LAYERS (Slot System)
// ============================================================================

UENUM(BlueprintType)
enum class EAnimationLayer : uint8
{
	// Base layers (exclusive - only one can be active)
	BaseLocomotion = 0,      // Walking, running, sprinting
	BaseAction = 1,          // Climbing, vehicle entry, etc.
	
	// Additive layers (can stack on top of base)
	UpperBody = 10,          // Reload, weapon handling
	LeftArm = 11,            // Left arm specific
	RightArm = 12,           // Right arm specific
	Head = 13,               // Head/face animations
	LowerBody = 14,          // Lower body specific (locks movement if flagged)
	
	// Special layers
	AimOffset = 20,          // Aim offset (additive)
	Recoil = 21,             // Weapon recoil (additive)
	Facial = 22,             // Facial expressions
	
	// Full body overrides (highest priority)
	FullBodyOverride = 30,   // Cutscenes, death, etc.
	
	MAX = 31
};

// ============================================================================
// ANIMATION FLAGS (Bitfield for combining behaviors)
// ============================================================================

UENUM(BlueprintType, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EAnimationFlags : uint8
{
	None = 0,
	
	// Movement control
	LockMovement = 1 << 0,           // Disable movement input
	LockRotation = 1 << 1,           // Disable rotation input
	LockLowerBody = 1 << 2,          // Lock lower body (no walking while this plays)
	
	// Playback control
	UseRootMotion = 1 << 3,          // Apply root motion from animation
	Looping = 1 << 4,                // Loop animation
	
	// Interruption
	Interruptible = 1 << 5,          // Can be interrupted by higher priority
	
	// Synchronization
	SyncWithLocomotion = 1 << 6,     // Sync playback speed with movement speed
	
	// Special
	IgnoreTimeDilation = 1 << 7,     // Play at normal speed even in slow-mo
	
	// Common combinations (for convenience)
	FullBodyLocked = LockMovement | LockRotation | LockLowerBody
};
ENUM_CLASS_FLAGS(EAnimationFlags);

// ============================================================================
// BLEND MODES
// ============================================================================

UENUM(BlueprintType)
enum class EAnimBlendMode : uint8
{
	Replace,        // Replace current animation in layer
	Blend,          // Smooth blend with current
	Additive,       // Add on top (for aim offset, recoil, etc.)
	Override,       // Immediate replace, no blend
	LayerBlend      // Blend using layer weight
};

// ============================================================================
// BLEND CURVE TYPES
// ============================================================================

UENUM(BlueprintType)
enum class EBlendCurveType : uint8
{
	Linear,         // Linear interpolation
	EaseIn,         // Slow start, fast end
	EaseOut,        // Fast start, slow end
	EaseInOut,      // Slow start and end
	Cubic,          // Cubic curve
	Custom          // Use custom curve asset
};

// ============================================================================
// ANIMATION PRIORITY LEVELS
// ============================================================================

UENUM(BlueprintType)
enum class EAnimationPriority : uint8
{
	Lowest = 0,
	Low = 25,
	Normal = 50,
	High = 75,
	Highest = 100,
	Critical = 200,      // Cannot be interrupted (death, cutscene, etc.)
};

// ============================================================================
// LOCOMOTION BLEND MODES (For swappable locomotion sets)
// ============================================================================

UENUM(BlueprintType)
enum class ELocomotionSet : uint8
{
	Default,         // Normal walk/run/sprint
	Combat,          // Combat strafing
	Injured,         // Limping, wounded
	Stealth,         // Sneaking
	Drunk,           // Stumbling
	Custom1,         // User-defined
	Custom2,
	Custom3
};

// ============================================================================
// BONE MASK PRESETS (For layer blending)
// ============================================================================

UENUM(BlueprintType)
enum class EBoneMaskPreset : uint8
{
	FullBody,        // All bones
	UpperBody,       // Spine and above
	LowerBody,       // Pelvis and below
	LeftArm,         // Left arm chain
	RightArm,        // Right arm chain
	Head,            // Head and neck
	Spine,           // Spine only
	Arms,            // Both arms
	Legs,            // Both legs
	Custom           // Use custom bone list
};

// ============================================================================
// VEHICLE ANIMATION TYPES (For vehicle-specific animations)
// ============================================================================

UENUM(BlueprintType)
enum class EVehicleAnimationType : uint8
{
	// Doors
	DoorFrontLeft,
	DoorFrontRight,
	DoorRearLeft,
	DoorRearRight,
	
	// Hood/Trunk
	Hood,
	Trunk,
	
	// Windows
	WindowFrontLeft,
	WindowFrontRight,
	WindowRearLeft,
	WindowRearRight,
	
	// Special
	Convertible,      // Convertible roof
	Spoiler,          // Active spoiler
	PopupHeadlights,  // Popup headlights
	
	// Entry/Exit
	EnterDriver,
	EnterPassenger,
	ExitDriver,
	ExitPassenger,
	
	// Damage
	DamagedIdle,      // Vehicle damaged state
	Exploding         // Explosion sequence
};

// ============================================================================
// WEAPON DETAIL ANIMATIONS (Bolt, lever, eject, etc.)
// ============================================================================

UENUM(BlueprintType)
enum class EWeaponDetailAnimation : uint8
{
	// Bolt actions
	BoltOpen,
	BoltClose,
	BoltCycle,        // Full cycle
	
	// Lever actions
	LeverOpen,
	LeverClose,
	LeverCycle,
	
	// Slide/Charging handle
	SlideBack,
	SlideForward,
	SlideCycle,
	ChargingHandle,
	
	// Shell ejection
	ShellEject,
	ShellEjectDouble, // Double barrel
	
	// Magazine
	MagRelease,
	MagInsert,
	MagDrop,
	
	// Cylinder (revolvers)
	CylinderOpen,
	CylinderClose,
	CylinderSpin,
	
	// Pump action
	PumpBack,
	PumpForward,
	PumpCycle,
	
	// Safety/Selector
	SafetyOn,
	SafetyOff,
	SelectorSwitch,
	
	// Hammer
	HammerCock,
	HammerRelease,
	
	// Trigger
	TriggerPress,
	TriggerRelease,
	
	// Bipod/Attachments
	BipodDeploy,
	BipodRetract,
	
	// Barrel
	BarrelBreak,      // Break-action
	BarrelClose
};

// ============================================================================
// WORLD OBJECT ANIMATIONS (Doors, levers, buttons, etc.)
// ============================================================================

UENUM(BlueprintType)
enum class EWorldObjectAnimation : uint8
{
	// Doors
	DoorOpen,
	DoorClose,
	DoorKick,         // Forced entry
	DoorPeek,         // Peek through door
	
	// Windows
	WindowOpen,
	WindowClose,
	WindowBreak,
	
	// Drawers/Cabinets
	DrawerOpen,
	DrawerClose,
	CabinetOpen,
	CabinetClose,
	
	// Levers/Switches
	LeverPull,
	LeverPush,
	SwitchFlip,
	ButtonPress,
	
	// Valves/Wheels
	ValveTurn,
	WheelTurn,
	
	// Ladders/Climbing
	LadderClimbUp,
	LadderClimbDown,
	LadderMount,
	LadderDismount,
	
	// Containers
	ChestOpen,
	ChestClose,
	CrateOpen,
	
	// Interactive
	PickupObject,
	PlaceObject,
	ThrowObject,
	
	// Vending/ATM
	VendingMachineUse,
	ATMUse,
	
	// Misc
	SitDown,
	StandUp,
	LeanAgainst
};

// ============================================================================
// ANIMATION SLOT CONTEXT (What is being animated)
// ============================================================================

UENUM(BlueprintType)
enum class EAnimationContext : uint8
{
	Character,        // Character/Ped animation
	Vehicle,          // Vehicle part animation
	Weapon,           // Weapon detail animation
	WorldObject,      // World object animation
	Prop              // Dynamic prop animation
};

