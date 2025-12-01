#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnimationEnums.h"
#include "AnimationStructs.h"
#include "AnimationManager.generated.h"

class UAnimationPoolManager;

/**
 * Animation Manager Component
 * Pure C++ animation system - manages all animation layers and playback
 * GTA5-style API with full control over animation blending and layering
 * Supports pooling, vehicle animations, weapon details, and world objects
 */
UCLASS(ClassGroup=(Animation), meta=(BlueprintSpawnableComponent))
class JUSTLIVE_API UAnimationManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAnimationManager();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========================================================================
	// MAIN API (GTA5-Style)
	// ========================================================================

	/**
	 * Play an animation with full control
	 * @param Request - Animation request with all parameters
	 * @return true if animation started playing
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	bool PlayAnimation(const FAnimationRequest& Request);

	/**
	 * Stop animation in a specific layer
	 * @param Layer - Which layer to stop
	 * @param BlendOutTime - Time to blend out (0 = instant)
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void StopAnimation(EAnimationLayer Layer, float BlendOutTime = 0.2f);

	/**
	 * Stop all animations
	 * @param BlendOutTime - Time to blend out all layers
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void StopAllAnimations(float BlendOutTime = 0.2f);

	/**
	 * Pause animation in layer
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void PauseAnimation(EAnimationLayer Layer);

	/**
	 * Resume paused animation
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void ResumeAnimation(EAnimationLayer Layer);

	// ========================================================================
	// LAYER MANAGEMENT
	// ========================================================================

	/**
	 * Add a new animation layer (for weapon-specific overrides, etc.)
	 * @param Layer - Layer to add
	 * @param Weight - Initial weight (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Layers")
	void AddLayer(EAnimationLayer Layer, float Weight = 1.0f);

	/**
	 * Remove an animation layer
	 * @param Layer - Layer to remove
	 * @param BlendOutTime - Time to blend out before removing
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Layers")
	void RemoveLayer(EAnimationLayer Layer, float BlendOutTime = 0.2f);

	/**
	 * Set layer weight (for blending between layers)
	 * @param Layer - Which layer
	 * @param Weight - Target weight (0.0 to 1.0)
	 * @param BlendTime - Time to reach target weight
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Layers")
	void SetLayerWeight(EAnimationLayer Layer, float Weight, float BlendTime = 0.2f);

	/**
	 * Get current layer weight
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Layers")
	float GetLayerWeight(EAnimationLayer Layer) const;

	// ========================================================================
	// PLAYBACK CONTROL
	// ========================================================================

	/**
	 * Set animation playback time
	 * @param Layer - Which layer
	 * @param Time - Time in seconds
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Playback")
	void SetAnimationTime(EAnimationLayer Layer, float Time);

	/**
	 * Set animation playback rate
	 * @param Layer - Which layer
	 * @param PlayRate - Playback speed multiplier
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Playback")
	void SetAnimationPlayRate(EAnimationLayer Layer, float PlayRate);

	/**
	 * Get current animation progress (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Playback")
	float GetAnimationProgress(EAnimationLayer Layer) const;

	// ========================================================================
	// QUERIES
	// ========================================================================

	/**
	 * Is an animation playing in this layer?
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Query")
	bool IsPlayingAnimation(EAnimationLayer Layer) const;

	/**
	 * Get current animation in layer
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Query")
	UAnimSequence* GetCurrentAnimation(EAnimationLayer Layer) const;

	/**
	 * Is movement locked by any animation?
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Query")
	bool IsMovementLocked() const;

	/**
	 * Is rotation locked by any animation?
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Query")
	bool IsRotationLocked() const;

	/**
	 * Is lower body locked (prevents locomotion)?
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Query")
	bool IsLowerBodyLocked() const;

	// ========================================================================
	// LOCOMOTION CONTROL
	// ========================================================================

	/**
	 * Set locomotion animation set (swappable locomotion)
	 * @param LocomotionSet - Which set to use (Default, Combat, Injured, etc.)
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void SetLocomotionSet(ELocomotionSet LocomotionSet);

	/**
	 * Update locomotion blending (called automatically, but can be manual)
	 * @param Speed - Current movement speed
	 * @param Direction - Movement direction relative to actor (-180 to 180)
	 */
	void UpdateLocomotion(float Speed, float Direction);

	// ========================================================================
	// ANIMATION POOLING
	// ========================================================================

	/**
	 * Enable/disable animation pooling
	 * @param bEnable - Enable pooling
	 * @param MaxConcurrent - Maximum concurrent animations (default 100)
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Pooling")
	void SetPoolingEnabled(bool bEnable, int32 MaxConcurrent = 100);

	/**
	 * Get pool usage percentage
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Pooling")
	float GetPoolUsagePercent() const;

	/**
	 * Get number of active pooled animations
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Pooling")
	int32 GetActivePooledAnimationCount() const;

	// ========================================================================
	// INTERNAL
	// ========================================================================

private:
	/** All animation layers */
	UPROPERTY()
	TMap<EAnimationLayer, FAnimationLayerState> AnimationLayers;

	/** Reference to skeletal mesh component */
	UPROPERTY()
	USkeletalMeshComponent* SkeletalMesh;

	/** Current locomotion set */
	ELocomotionSet CurrentLocomotionSet = ELocomotionSet::Default;

	/** Locomotion blend data for each set */
	UPROPERTY()
	TMap<ELocomotionSet, FLocomotionBlendData> LocomotionSets;

	/** Animation pool manager (optional, for performance) */
	UPROPERTY()
	UAnimationPoolManager* PoolManager;

	/** Is pooling enabled */
	bool bPoolingEnabled = false;

	// Update functions
	void UpdateAnimationLayers(float DeltaTime);
	void UpdateLayerBlending(FAnimationLayerState& LayerState, float DeltaTime);
	void UpdateLayerPlayback(FAnimationLayerState& LayerState, float DeltaTime, float CurrentMovementSpeed);
	
	// Blending helpers
	float CalculateBlendAlpha(float TimeRemaining, float Duration, EBlendCurveType CurveType, UCurveFloat* CustomCurve) const;
	
	// Layer management
	FAnimationLayerState* GetLayerState(EAnimationLayer Layer);
	const FAnimationLayerState* GetLayerState(EAnimationLayer Layer) const;
	bool CanPlayAnimation(const FAnimationRequest& Request) const;
	void InterruptLayer(EAnimationLayer Layer, const FAnimationRequest& NewRequest);

	// Initialization
	void InitializeDefaultLayers();
	void CacheSkeletalMesh();

public:
	// ========================================================================
	// EVENTS (For external systems to hook into)
	// ========================================================================

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnimationStarted, EAnimationLayer, Layer, UAnimSequence*, Animation);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnimationFinished, EAnimationLayer, Layer, UAnimSequence*, Animation);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnimationInterrupted, EAnimationLayer, Layer, UAnimSequence*, Animation);

	UPROPERTY(BlueprintAssignable, Category = "Animation|Events")
	FOnAnimationStarted OnAnimationStarted;

	UPROPERTY(BlueprintAssignable, Category = "Animation|Events")
	FOnAnimationFinished OnAnimationFinished;

	UPROPERTY(BlueprintAssignable, Category = "Animation|Events")
	FOnAnimationInterrupted OnAnimationInterrupted;
};
