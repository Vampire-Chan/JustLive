#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Animation/AnimSequence.h"
#include "AnimationPoolManager.generated.h"

/** Animation slot data */
struct FAnimationSlot
{
	bool bInUse = false;
	int32 Priority = 0;
	float StartTime = 0.0f;
	
	void Reset()
	{
		bInUse = false;
		Priority = 0;
		StartTime = 0.0f;
	}
};

/**
 * Animation Pool Manager
 * Manages pooling of animation instances to prevent performance issues
 * Limits concurrent animations and reuses animation data
 */
UCLASS()
class JUSTLIVE_API UAnimationPoolManager : public UObject
{
	GENERATED_BODY()

public:
	UAnimationPoolManager();

	/**
	 * Initialize the pool manager
	 * @param MaxConcurrentAnimations - Maximum number of animations that can play simultaneously
	 */
	void Initialize(int32 MaxConcurrentAnimations = 100);

	/**
	 * Request to play an animation (may be denied if pool is full)
	 * @param Priority - Animation priority (higher = more important)
	 * @param OutSlotIndex - If successful, returns the slot index to use
	 * @return true if animation can play
	 */
	bool RequestAnimationSlot(int32 Priority, int32& OutSlotIndex);

	/**
	 * Release an animation slot when animation finishes
	 * @param SlotIndex - The slot to release
	 */
	void ReleaseAnimationSlot(int32 SlotIndex);

	/**
	 * Get current pool usage
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Pool")
	float GetPoolUsagePercent() const;

	/**
	 * Get number of active animations
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Pool")
	int32 GetActiveAnimationCount() const;

	/**
	 * Force stop lowest priority animations to make room
	 * @param Count - Number of slots to free
	 */
	void FreeLowestPrioritySlots(int32 Count = 1);

	/**
	 * Set maximum concurrent animations (can be changed at runtime)
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Pool")
	void SetMaxConcurrentAnimations(int32 NewMax);

private:
	/** Pool of animation slots */
	TArray<FAnimationSlot> AnimationSlots;

	/** Maximum concurrent animations */
	int32 MaxConcurrentAnimations = 100;

	/** Current active count */
	int32 ActiveCount = 0;

	/** Find lowest priority slot */
	int32 FindLowestPrioritySlot() const;
};
