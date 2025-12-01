# Animation Blueprint Structure for JustLive Ped Character

## Overview
This document describes the complete Animation Blueprint structure for the Ped character system. The AnimBP uses `UPedAnimInstance` as its parent class and implements a hierarchical state machine.

---

## Animation Blueprint: ABP_Ped

**Parent Class:** `UPedAnimInstance` (C++ class in `/Source/JustLive/Gameplay/Peds/Animation/PedAnimInstance.h`)

**Skeleton:** `Humanoid_Skeleton`

---

## AnimGraph Structure

```
[Output Pose]
    ← [Layered Blend Per Bone] (Upper Body Slot for weapons/aiming)
        ← Slot: UpperBody
        ← Base Pose: [Main State Machine]
```

---

## Main State Machine

### States:

1. **Locomotion** (Default Entry State)
2. **Cover**
3. **Crouched**
4. **Combat**
5. **Swimming**

### Transitions:

#### From Locomotion:
- **To Cover:** `CoverState != ECoverState::None`
- **To Crouched:** `Stance == EPedStance::Crouch && CoverState == ECoverState::None`
- **To Combat:** `bIsAiming == true && CoverState == ECoverState::None`
- **To Swimming:** `CurrentState == EPedState::Swim`

#### From Cover:
- **To Locomotion:** `CoverState == ECoverState::None && !bIsAiming`
- **To Combat:** `bIsAiming == true`

#### From Crouched:
- **To Locomotion:** `Stance == EPedStance::Stand`
- **To Combat:** `bIsAiming == true`

#### From Combat:
- **To Locomotion:** `bIsAiming == false && Stance == EPedStance::Stand`
- **To Crouched:** `bIsAiming == false && Stance == EPedStance::Crouch`
- **To Cover:** `CoverState != ECoverState::None`

#### From Swimming:
- **To Locomotion:** `CurrentState != EPedState::Swim`

---

## 1. Locomotion State

### Sub-State Machine: Locomotion_SM

#### States:

##### **Idle/Walk/Run** (Blend Space State)
- **Blend Space:** `BS_Locomotion` (2D: Speed X Direction)
  - X-Axis: Speed (0 to 900)
  - Y-Axis: Direction (-180 to 180)
- **Animations:**
  - Idle (0, 0)
  - Walk Forward (300, 0)
  - Walk Backward (300, 180)
  - Walk Left (300, -90)
  - Walk Right (300, 90)
  - Run Forward (600, 0)
  - Sprint Forward (900, 0)

##### **Jump Start**
- **Animation:** `Anim_Jump_Start`
- **Transition Out:** When animation ends → Jump Loop

##### **Jump Loop**
- **Animation:** `Anim_Jump_Loop` (Looping)
- **Transition Out:** When `bIsInAir == false` → Jump Land

##### **Jump Land**
- **Animation:** `Anim_Jump_Land`
- **Transition Out:** When animation ends → Idle/Walk/Run

##### **Fall**
- **Animation:** `Anim_Fall_Loop` (Looping)
- **Transition Out:** When `bIsInAir == false` → Jump Land

#### Transitions:

- **Idle/Walk/Run → Jump Start:** `bIsInAir == true && Velocity.Z > 200`
- **Idle/Walk/Run → Fall:** `bIsInAir == true && Velocity.Z < -100`
- **Jump Land → Idle/Walk/Run:** Animation fully blended

---

## 2. Cover State

### Sub-State Machine: Cover_SM

#### States:

##### **Cover Low Idle**
- **Animation:** `Anim_Cover_Low_Idle`
- **Condition:** `CoverState == ECoverState::Low`

##### **Cover High Idle**
- **Animation:** `Anim_Cover_High_Idle`
- **Condition:** `CoverState == ECoverState::High`

##### **Peek Left**
- **Animation:** `Anim_Cover_Peek_Left`
- **Triggered by:** Input action or AI decision

##### **Peek Right**
- **Animation:** `Anim_Cover_Peek_Right`

##### **Peek Up** (High Cover only)
- **Animation:** `Anim_Cover_Peek_Up`

##### **Vault Over**
- **Animation:** `Anim_Vault_Over`
- **Transition Out:** When animation ends → Locomotion State

#### Transitions:

- **Low Idle ↔ High Idle:** `CoverState` changes
- **Any Idle → Peek:** Input trigger
- **Peek → Idle:** Animation ends or input released
- **Any → Vault:** Vault input pressed

---

## 3. Crouched State

### Sub-State Machine: Crouch_SM

#### States:

##### **Crouch Idle/Walk** (Blend Space State)
- **Blend Space:** `BS_Crouch` (1D: Speed)
  - Idle (0)
  - Crouch Walk Forward (300)
  - Crouch Walk Backward (300, mirrored)

##### **Crouch Turn** (Optional)
- **Blend Space:** `BS_Crouch_Turn` (Direction-based)

#### Transitions:

- **Idle ↔ Walk:** Based on `Speed` value

---

## 4. Combat State

### Sub-State Machine: Combat_SM

#### States:

##### **Combat Idle/Walk** (Blend Space State)
- **Blend Space:** `BS_Combat` (2D: Speed X Direction)
  - Combat Idle (0, 0)
  - Combat Walk Forward (300, 0)
  - Combat Walk Backward (300, 180)
  - Combat Strafe Left (300, -90)
  - Combat Strafe Right (300, 90)

##### **Aiming** (Additive Layer)
- **Aim Offset:** `AO_Aim` (2D: Yaw X Pitch)
  - Uses `AimPitch` and calculated Yaw
  - Blended additively on top of base pose

##### **Reload**
- **Animation:** `Anim_Reload` (weapon-specific)
- **Triggered by:** Reload input
- **Transition Out:** When animation ends

##### **Melee Attack**
- **Animation Sequence:** `Anim_Melee_Swing`
- **Triggered by:** Melee input
- **Transition Out:** When animation ends

#### Layering:

```
[Combat Base Pose]
    ← [Additive: Aim Offset]
        ← [Slot: UpperBody] (for reload/weapon handling)
```

---

## 5. Swimming State

### Sub-State Machine: Swimming_SM

#### States:

##### **Surface Idle**
- **Animation:** `Anim_Swim_Surface_Idle`
- **Condition:** `Speed < 50 && !bIsDiving`

##### **Surface Swim**
- **Blend Space:** `BS_Swim_Surface` (1D: Speed)
  - Idle (0)
  - Slow Swim (200)
  - Fast Swim (400)

##### **Dive Transition**
- **Animation:** `Anim_Swim_Dive`
- **Triggered by:** Dive input
- **Transition Out:** When animation ends → Underwater

##### **Underwater Swim**
- **Blend Space:** `BS_Swim_Underwater` (3D: Speed X Pitch X Yaw)
  - Allows full 3D movement underwater

##### **Surface Transition**
- **Animation:** `Anim_Swim_Surface_Up`
- **Triggered by:** Surface input or oxygen low
- **Transition Out:** When animation ends → Surface Idle

#### Transitions:

- **Surface Idle ↔ Surface Swim:** Based on `Speed`
- **Surface → Dive:** Dive input
- **Underwater → Surface:** Surface input
- **Any → Drowning:** Oxygen == 0 (death animation)

---

## Animation Slots

### Slot Names:
1. **UpperBody** - For upper body animations (reload, weapon handling, gestures)
2. **FullBody** - For full body overrides (climbing, vehicle entry)
3. **FacialExpression** - For facial animations (optional)

### Usage in Code:
```cpp
// Play reload animation in UpperBody slot
PlaySlotAnimationAsDynamicMontage(ReloadAnim, "UpperBody", 0.2f, 0.2f);
```

---

## Blend Spaces to Create

### 1. BS_Locomotion (2D)
- **X-Axis:** Speed (0-900)
- **Y-Axis:** Direction (-180 to 180)
- **Grid:** 5x5
- **Animations:** Idle, Walk (F/B/L/R), Run Fwd, Sprint Fwd

### 2. BS_Crouch (1D)
- **Axis:** Speed (0-300)
- **Animations:** Crouch Idle, Crouch Walk Fwd

### 3. BS_Combat (2D)
- **X-Axis:** Speed (0-400)
- **Y-Axis:** Direction (-180 to 180)
- **Animations:** Combat Idle, Combat Walk (F/B/L/R)

### 4. BS_Swim_Surface (1D)
- **Axis:** Speed (0-400)
- **Animations:** Surface Idle, Slow Swim, Fast Swim

### 5. BS_Swim_Underwater (3D)
- **X-Axis:** Speed (0-400)
- **Y-Axis:** Pitch (-90 to 90)
- **Z-Axis:** Yaw (-180 to 180)

### 6. AO_Aim (Aim Offset 2D)
- **X-Axis:** Yaw (-90 to 90)
- **Y-Axis:** Pitch (-90 to 90)
- **Animations:** Aim poses at different angles

---

## Animation Notifies

### Custom Notifies to Create:

1. **AN_Footstep** - Triggers footstep sound
2. **AN_ShellEject** - Ejects bullet casing
3. **AN_MuzzleFlash** - Triggers muzzle flash VFX
4. **AN_ReloadComplete** - Signals reload finished
5. **AN_MeleeHit** - Checks for melee hit detection
6. **AN_JumpApex** - Marks jump apex for physics
7. **AN_Splash** - Water splash effect

---

## Implementation Steps

### Step 1: Create Blend Spaces
1. Right-click in Content Browser → Animation → Blend Space
2. Select `Humanoid_Skeleton`
3. Configure axes as described above
4. Add animation samples at appropriate coordinates

### Step 2: Create Aim Offset
1. Right-click → Animation → Aim Offset
2. Configure Yaw/Pitch axes
3. Add aim pose animations

### Step 3: Create Animation Blueprint
1. Right-click → Animation → Animation Blueprint
2. Select `Humanoid_Skeleton`
3. Set Parent Class to `PedAnimInstance`

### Step 4: Build State Machines
1. Open ABP_Ped
2. In AnimGraph, add State Machine node
3. Create states as described above
4. Add transitions with conditions
5. Wire to Output Pose

### Step 5: Add Layering
1. Add "Layered blend per bone" node
2. Configure bone layers (Spine for upper body)
3. Add animation slots

### Step 6: Test
1. Assign ABP_Ped to Ped character
2. Test all states in PIE
3. Adjust blend times and transition conditions

---

## Variables Exposed from C++

From `UPedAnimInstance`:

- `float Speed` - Current movement speed
- `float Direction` - Movement direction relative to actor
- `bool bIsMoving` - Is character moving
- `bool bIsInAir` - Is character in air
- `bool bIsAccelerating` - Is character accelerating
- `EPedState CurrentState` - Current ped state
- `EPedStance Stance` - Current stance (Stand/Crouch/Prone)
- `ECoverState CoverState` - Current cover state
- `bool bIsAiming` - Is character aiming
- `float AimPitch` - Aim pitch for aim offset

---

## Performance Considerations

1. **LOD System:** Create simplified animation blueprints for distant characters
2. **Update Rate Optimization:** Use "Update Rate Optimizations" node
3. **Bone Reduction:** Disable unnecessary bones for NPCs
4. **Parallel Evaluation:** Enable parallel animation evaluation in Project Settings

---

## Future Enhancements

1. **Procedural IK:**
   - Foot IK for uneven terrain
   - Hand IK for weapon grips
   - Look-at IK for head tracking

2. **Physics Animations:**
   - Ragdoll blending
   - Hit reactions
   - Cloth simulation

3. **Facial Animations:**
   - Lip sync
   - Eye tracking
   - Emotional expressions

4. **Contextual Animations:**
   - Ladder climbing
   - Vehicle entry/exit
   - Object interaction
