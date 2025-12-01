#include "AnimationPoolManager.h"
#include "Engine/World.h"

UAnimationPoolManager::UAnimationPoolManager()
{
	MaxConcurrentAnimations = 100;
	ActiveCount = 0;
}

void UAnimationPoolManager::Initialize(int32 MaxConcurrent)
{
	MaxConcurrentAnimations = MaxConcurrent;
	AnimationSlots.SetNum(MaxConcurrentAnimations);
	
	for (FAnimationSlot& Slot : AnimationSlots)
	{
		Slot.Reset();
	}
	
	ActiveCount = 0;
	
	UE_LOG(LogTemp, Log, TEXT("AnimationPoolManager: Initialized with %d slots"), MaxConcurrentAnimations);
}

bool UAnimationPoolManager::RequestAnimationSlot(int32 Priority, int32& OutSlotIndex)
{
	// Try to find free slot
	for (int32 i = 0; i < AnimationSlots.Num(); ++i)
	{
		if (!AnimationSlots[i].bInUse)
		{
			AnimationSlots[i].bInUse = true;
			AnimationSlots[i].Priority = Priority;
			AnimationSlots[i].StartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
			OutSlotIndex = i;
			ActiveCount++;
			return true;
		}
	}
	
	// Pool is full - check if we can interrupt lower priority
	int32 LowestPrioritySlot = FindLowestPrioritySlot();
	if (LowestPrioritySlot != INDEX_NONE && AnimationSlots[LowestPrioritySlot].Priority < Priority)
	{
		// Interrupt lower priority animation
		AnimationSlots[LowestPrioritySlot].bInUse = true;
		AnimationSlots[LowestPrioritySlot].Priority = Priority;
		AnimationSlots[LowestPrioritySlot].StartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		OutSlotIndex = LowestPrioritySlot;
		
		UE_LOG(LogTemp, Warning, TEXT("AnimationPoolManager: Interrupted slot %d (priority %d) for new animation (priority %d)"),
			LowestPrioritySlot, AnimationSlots[LowestPrioritySlot].Priority, Priority);
		
		return true;
	}
	
	// Cannot play animation - pool full and all higher priority
	UE_LOG(LogTemp, Warning, TEXT("AnimationPoolManager: Pool full, cannot play animation (priority %d)"), Priority);
	return false;
}

void UAnimationPoolManager::ReleaseAnimationSlot(int32 SlotIndex)
{
	if (AnimationSlots.IsValidIndex(SlotIndex) && AnimationSlots[SlotIndex].bInUse)
	{
		AnimationSlots[SlotIndex].Reset();
		ActiveCount--;
	}
}

float UAnimationPoolManager::GetPoolUsagePercent() const
{
	return MaxConcurrentAnimations > 0 ? (float)ActiveCount / (float)MaxConcurrentAnimations * 100.0f : 0.0f;
}

int32 UAnimationPoolManager::GetActiveAnimationCount() const
{
	return ActiveCount;
}

void UAnimationPoolManager::FreeLowestPrioritySlots(int32 Count)
{
	for (int32 i = 0; i < Count; ++i)
	{
		int32 LowestSlot = FindLowestPrioritySlot();
		if (LowestSlot != INDEX_NONE)
		{
			ReleaseAnimationSlot(LowestSlot);
		}
	}
}

void UAnimationPoolManager::SetMaxConcurrentAnimations(int32 NewMax)
{
	if (NewMax != MaxConcurrentAnimations)
	{
		MaxConcurrentAnimations = NewMax;
		AnimationSlots.SetNum(MaxConcurrentAnimations);
		
		// Reset any slots beyond the new max
		for (int32 i = NewMax; i < AnimationSlots.Num(); ++i)
		{
			if (AnimationSlots[i].bInUse)
			{
				AnimationSlots[i].Reset();
				ActiveCount--;
			}
		}
		
		UE_LOG(LogTemp, Log, TEXT("AnimationPoolManager: Max concurrent animations changed to %d"), NewMax);
	}
}

int32 UAnimationPoolManager::FindLowestPrioritySlot() const
{
	int32 LowestSlot = INDEX_NONE;
	int32 LowestPriority = INT_MAX;
	
	for (int32 i = 0; i < AnimationSlots.Num(); ++i)
	{
		if (AnimationSlots[i].bInUse && AnimationSlots[i].Priority < LowestPriority)
		{
			LowestPriority = AnimationSlots[i].Priority;
			LowestSlot = i;
		}
	}
	
	return LowestSlot;
}
