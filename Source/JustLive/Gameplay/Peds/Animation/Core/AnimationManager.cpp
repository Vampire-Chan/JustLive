#include "AnimationManager.h"
#include "AnimationPoolManager.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Kismet/KismetMathLibrary.h"

UAnimationManager::UAnimationManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UAnimationManager::BeginPlay()
{
	Super::BeginPlay();
	
	CacheSkeletalMesh();
	InitializeDefaultLayers();
}

void UAnimationManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!SkeletalMesh)
	{
		CacheSkeletalMesh();
		return;
	}
	
	UpdateAnimationLayers(DeltaTime);
}

// ============================================================================
// MAIN API
// ============================================================================

bool UAnimationManager::PlayAnimation(const FAnimationRequest& Request)
{
	if (!Request.Animation)
	{
		UE_LOG(LogTemp, Warning, TEXT("AnimationManager: Cannot play null animation"));
		return false;
	}
	
	if (!CanPlayAnimation(Request))
	{
		UE_LOG(LogTemp, Log, TEXT("AnimationManager: Animation blocked by higher priority"));
		return false;
	}
	
	// Check pooling if enabled
	if (bPoolingEnabled && PoolManager && Request.bUsePooling)
	{
		int32 PoolSlot = INDEX_NONE;
		if (!PoolManager->RequestAnimationSlot(Request.Priority, PoolSlot))
		{
			UE_LOG(LogTemp, Warning, TEXT("AnimationManager: Animation denied - pool full"));
			return false;
		}
		// Store pool slot for later release
		const_cast<FAnimationRequest&>(Request).PoolSlotIndex = PoolSlot;
	}
	
	// Get or create layer state
	FAnimationLayerState* LayerState = GetLayerState(Request.Layer);
	if (!LayerState)
	{
		// Create new layer
		FAnimationLayerState NewLayer;
		NewLayer.Layer = Request.Layer;
		AnimationLayers.Add(Request.Layer, NewLayer);
		LayerState = GetLayerState(Request.Layer);
	}
	
	// Interrupt current animation if needed (higher priority wins)
	if (LayerState->bIsPlaying && Request.Priority > LayerState->Priority)
	{
		InterruptLayer(Request.Layer, Request);
	}
	
	// Setup new animation
	LayerState->CurrentAnimation = Request.Animation;
	LayerState->CurrentTime = Request.StartPosition * Request.Animation->GetPlayLength();
	LayerState->PlayRate = Request.PlayRate;
	LayerState->Flags = Request.Flags;
	LayerState->Priority = Request.Priority;
	LayerState->BoneMask = Request.BoneMask;
	LayerState->CustomBoneNames = Request.CustomBoneNames;
	LayerState->BlendMode = Request.BlendMode;
	
	// Setup blending
	LayerState->bIsBlendingIn = (Request.BlendInTime > 0.0f);
	LayerState->bIsBlendingOut = false;
	LayerState->BlendInDuration = Request.BlendInTime;
	LayerState->BlendOutDuration = Request.BlendOutTime;
	LayerState->BlendInTimeRemaining = Request.BlendInTime;
	LayerState->BlendOutTimeRemaining = 0.0f;
	LayerState->BlendInCurve = Request.BlendInCurve;
	LayerState->BlendOutCurve = Request.BlendOutCurve;
	LayerState->CustomBlendInCurve = Request.CustomBlendInCurve;
	LayerState->CustomBlendOutCurve = Request.CustomBlendOutCurve;
	LayerState->TargetBlendWeight = Request.LayerWeight;
	
	if (Request.BlendInTime <= 0.0f)
	{
		LayerState->BlendWeight = Request.LayerWeight;
	}
	else
	{
		LayerState->BlendWeight = 0.0f;
	}
	
	// Setup playback
	LayerState->bIsPlaying = true;
	LayerState->bIsLooping = Request.HasFlag(EAnimationFlags::Looping);
	LayerState->bSyncWithMovementSpeed = Request.bSyncWithMovementSpeed;
	LayerState->ReferenceMovementSpeed = Request.ReferenceMovementSpeed;
	
	// Setup callbacks
	LayerState->OnComplete = Request.OnComplete;
	LayerState->OnInterrupted = Request.OnInterrupted;
	LayerState->OnUpdate = Request.OnUpdate;
	
	// Fire event
	OnAnimationStarted.Broadcast(Request.Layer, Request.Animation);
	
	UE_LOG(LogTemp, Log, TEXT("AnimationManager: Playing %s in layer %d"), 
		*Request.Animation->GetName(), static_cast<int32>(Request.Layer));
	
	return true;
}

void UAnimationManager::StopAnimation(EAnimationLayer Layer, float BlendOutTime)
{
	FAnimationLayerState* LayerState = GetLayerState(Layer);
	if (!LayerState || !LayerState->bIsPlaying)
	{
		return;
	}
	
	if (BlendOutTime <= 0.0f)
	{
		// Instant stop
		LayerState->bIsPlaying = false;
		LayerState->BlendWeight = 0.0f;
		OnAnimationFinished.Broadcast(Layer, LayerState->CurrentAnimation);
		
		if (LayerState->OnComplete)
		{
			LayerState->OnComplete();
		}
	}
	else
	{
		// Blend out
		LayerState->bIsBlendingOut = true;
		LayerState->BlendOutTimeRemaining = BlendOutTime;
		LayerState->BlendOutDuration = BlendOutTime;
	}
}

void UAnimationManager::StopAllAnimations(float BlendOutTime)
{
	for (auto& Pair : AnimationLayers)
	{
		StopAnimation(Pair.Key, BlendOutTime);
	}
}

void UAnimationManager::PauseAnimation(EAnimationLayer Layer)
{
	FAnimationLayerState* LayerState = GetLayerState(Layer);
	if (LayerState)
	{
		LayerState->PlayRate = 0.0f;
	}
}

void UAnimationManager::ResumeAnimation(EAnimationLayer Layer)
{
	FAnimationLayerState* LayerState = GetLayerState(Layer);
	if (LayerState)
	{
		LayerState->PlayRate = 1.0f;
	}
}

// ============================================================================
// LAYER MANAGEMENT
// ============================================================================

void UAnimationManager::AddLayer(EAnimationLayer Layer, float Weight)
{
	if (!AnimationLayers.Contains(Layer))
	{
		FAnimationLayerState NewLayer;
		NewLayer.Layer = Layer;
		NewLayer.BlendWeight = Weight;
		NewLayer.TargetBlendWeight = Weight;
		AnimationLayers.Add(Layer, NewLayer);
	}
}

void UAnimationManager::RemoveLayer(EAnimationLayer Layer, float BlendOutTime)
{
	StopAnimation(Layer, BlendOutTime);
	// Layer will be removed after blend out completes
}

void UAnimationManager::SetLayerWeight(EAnimationLayer Layer, float Weight, float BlendTime)
{
	FAnimationLayerState* LayerState = GetLayerState(Layer);
	if (LayerState)
	{
		LayerState->TargetBlendWeight = FMath::Clamp(Weight, 0.0f, 1.0f);
		
		if (BlendTime <= 0.0f)
		{
			LayerState->BlendWeight = LayerState->TargetBlendWeight;
		}
		// Else: will blend over time in UpdateLayerBlending
	}
}

float UAnimationManager::GetLayerWeight(EAnimationLayer Layer) const
{
	const FAnimationLayerState* LayerState = GetLayerState(Layer);
	return LayerState ? LayerState->BlendWeight : 0.0f;
}

// ============================================================================
// PLAYBACK CONTROL
// ============================================================================

void UAnimationManager::SetAnimationTime(EAnimationLayer Layer, float Time)
{
	FAnimationLayerState* LayerState = GetLayerState(Layer);
	if (LayerState && LayerState->CurrentAnimation)
	{
		LayerState->CurrentTime = FMath::Clamp(Time, 0.0f, LayerState->GetAnimationLength());
	}
}

void UAnimationManager::SetAnimationPlayRate(EAnimationLayer Layer, float PlayRate)
{
	FAnimationLayerState* LayerState = GetLayerState(Layer);
	if (LayerState)
	{
		LayerState->PlayRate = PlayRate;
	}
}

float UAnimationManager::GetAnimationProgress(EAnimationLayer Layer) const
{
	const FAnimationLayerState* LayerState = GetLayerState(Layer);
	return LayerState ? LayerState->GetNormalizedTime() : 0.0f;
}

// ============================================================================
// QUERIES
// ============================================================================

bool UAnimationManager::IsPlayingAnimation(EAnimationLayer Layer) const
{
	const FAnimationLayerState* LayerState = GetLayerState(Layer);
	return LayerState && LayerState->bIsPlaying;
}

UAnimSequence* UAnimationManager::GetCurrentAnimation(EAnimationLayer Layer) const
{
	const FAnimationLayerState* LayerState = GetLayerState(Layer);
	return LayerState ? LayerState->CurrentAnimation : nullptr;
}

bool UAnimationManager::IsMovementLocked() const
{
	for (const auto& Pair : AnimationLayers)
	{
		if (Pair.Value.bIsPlaying && Pair.Value.HasFlag(EAnimationFlags::LockMovement))
		{
			return true;
		}
	}
	return false;
}

bool UAnimationManager::IsRotationLocked() const
{
	for (const auto& Pair : AnimationLayers)
	{
		if (Pair.Value.bIsPlaying && Pair.Value.HasFlag(EAnimationFlags::LockRotation))
		{
			return true;
		}
	}
	return false;
}

bool UAnimationManager::IsLowerBodyLocked() const
{
	for (const auto& Pair : AnimationLayers)
	{
		if (Pair.Value.bIsPlaying && Pair.Value.HasFlag(EAnimationFlags::LockLowerBody))
		{
			return true;
		}
	}
	return false;
}

// ============================================================================
// LOCOMOTION
// ============================================================================

void UAnimationManager::SetLocomotionSet(ELocomotionSet LocomotionSet)
{
	CurrentLocomotionSet = LocomotionSet;
	UE_LOG(LogTemp, Log, TEXT("AnimationManager: Switched to locomotion set %d"), static_cast<int32>(LocomotionSet));
}

void UAnimationManager::UpdateLocomotion(float Speed, float Direction)
{
	// This will be implemented when we create the LocomotionAnimator
	// For now, just a placeholder
}

// ============================================================================
// INTERNAL UPDATE
// ============================================================================

void UAnimationManager::UpdateAnimationLayers(float DeltaTime)
{
	// Get current movement speed for syncing
	float CurrentMovementSpeed = 0.0f;
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		CurrentMovementSpeed = Character->GetVelocity().Size();
	}
	
	// Update each layer
	TArray<EAnimationLayer> LayersToRemove;
	
	for (auto& Pair : AnimationLayers)
	{
		FAnimationLayerState& LayerState = Pair.Value;
		
		if (!LayerState.bIsPlaying && LayerState.BlendWeight <= 0.0f)
		{
			LayersToRemove.Add(Pair.Key);
			continue;
		}
		
		// Update blending
		UpdateLayerBlending(LayerState, DeltaTime);
		
		// Update playback
		if (LayerState.bIsPlaying)
		{
			UpdateLayerPlayback(LayerState, DeltaTime, CurrentMovementSpeed);
		}
	}
	
	// Remove finished layers
	for (EAnimationLayer Layer : LayersToRemove)
	{
		AnimationLayers.Remove(Layer);
	}
}

void UAnimationManager::UpdateLayerBlending(FAnimationLayerState& LayerState, float DeltaTime)
{
	// Blend in
	if (LayerState.bIsBlendingIn && LayerState.BlendInTimeRemaining > 0.0f)
	{
		LayerState.BlendInTimeRemaining -= DeltaTime;
		
		float Alpha = CalculateBlendAlpha(
			LayerState.BlendInTimeRemaining,
			LayerState.BlendInDuration,
			LayerState.BlendInCurve,
			LayerState.CustomBlendInCurve
		);
		
		LayerState.BlendWeight = FMath::Lerp(0.0f, LayerState.TargetBlendWeight, 1.0f - Alpha);
		
		if (LayerState.BlendInTimeRemaining <= 0.0f)
		{
			LayerState.bIsBlendingIn = false;
			LayerState.BlendWeight = LayerState.TargetBlendWeight;
		}
	}
	
	// Blend out
	if (LayerState.bIsBlendingOut && LayerState.BlendOutTimeRemaining > 0.0f)
	{
		LayerState.BlendOutTimeRemaining -= DeltaTime;
		
		float Alpha = CalculateBlendAlpha(
			LayerState.BlendOutTimeRemaining,
			LayerState.BlendOutDuration,
			LayerState.BlendOutCurve,
			LayerState.CustomBlendOutCurve
		);
		
		LayerState.BlendWeight = FMath::Lerp(0.0f, LayerState.TargetBlendWeight, Alpha);
		
		if (LayerState.BlendOutTimeRemaining <= 0.0f)
		{
			LayerState.bIsBlendingOut = false;
			LayerState.bIsPlaying = false;
			LayerState.BlendWeight = 0.0f;
			
			OnAnimationFinished.Broadcast(LayerState.Layer, LayerState.CurrentAnimation);
			
			if (LayerState.OnComplete)
			{
				LayerState.OnComplete();
			}
		}
	}
}

void UAnimationManager::UpdateLayerPlayback(FAnimationLayerState& LayerState, float DeltaTime, float CurrentMovementSpeed)
{
	if (!LayerState.CurrentAnimation)
	{
		return;
	}
	
	// Calculate effective play rate
	float EffectivePlayRate = LayerState.PlayRate;
	
	if (LayerState.bSyncWithMovementSpeed && LayerState.ReferenceMovementSpeed > 0.0f)
	{
		EffectivePlayRate *= (CurrentMovementSpeed / LayerState.ReferenceMovementSpeed);
	}
	
	// Update time
	LayerState.CurrentTime += DeltaTime * EffectivePlayRate;
	
	// Handle looping/completion
	float AnimLength = LayerState.GetAnimationLength();
	
	if (LayerState.CurrentTime >= AnimLength)
	{
		if (LayerState.bIsLooping)
		{
			LayerState.CurrentTime = FMath::Fmod(LayerState.CurrentTime, AnimLength);
		}
		else
		{
			LayerState.CurrentTime = AnimLength;
			StopAnimation(LayerState.Layer, LayerState.BlendOutDuration);
		}
	}
	
	// Call update callback
	if (LayerState.OnUpdate)
	{
		LayerState.OnUpdate(LayerState.GetNormalizedTime());
	}
}

// ============================================================================
// HELPERS
// ============================================================================

float UAnimationManager::CalculateBlendAlpha(float TimeRemaining, float Duration, EBlendCurveType CurveType, UCurveFloat* CustomCurve) const
{
	if (Duration <= 0.0f)
	{
		return 0.0f;
	}
	
	float Alpha = FMath::Clamp(TimeRemaining / Duration, 0.0f, 1.0f);
	
	switch (CurveType)
	{
		case EBlendCurveType::Linear:
			return Alpha;
			
		case EBlendCurveType::EaseIn:
			return FMath::InterpEaseIn(0.0f, 1.0f, Alpha, 2.0f);
			
		case EBlendCurveType::EaseOut:
			return FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
			
		case EBlendCurveType::EaseInOut:
			return FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
			
		case EBlendCurveType::Cubic:
			return FMath::CubicInterp(0.0f, 0.0f, 1.0f, 0.0f, Alpha);
			
		case EBlendCurveType::Custom:
			if (CustomCurve)
			{
				return CustomCurve->GetFloatValue(Alpha);
			}
			return Alpha;
			
		default:
			return Alpha;
	}
}

FAnimationLayerState* UAnimationManager::GetLayerState(EAnimationLayer Layer)
{
	return AnimationLayers.Find(Layer);
}

const FAnimationLayerState* UAnimationManager::GetLayerState(EAnimationLayer Layer) const
{
	return AnimationLayers.Find(Layer);
}

bool UAnimationManager::CanPlayAnimation(const FAnimationRequest& Request) const
{
	const FAnimationLayerState* ExistingLayer = GetLayerState(Request.Layer);
	
	if (!ExistingLayer || !ExistingLayer->bIsPlaying)
	{
		return true; // Layer is free
	}
	
	// Check priority
	if (Request.Priority > ExistingLayer->Priority)
	{
		return true; // Higher priority can interrupt
	}
	
	// Check if existing animation is interruptible
	if (ExistingLayer->HasFlag(EAnimationFlags::Interruptible))
	{
		return true;
	}
	
	return false;
}

void UAnimationManager::InterruptLayer(EAnimationLayer Layer, const FAnimationRequest& NewRequest)
{
	FAnimationLayerState* LayerState = GetLayerState(Layer);
	if (LayerState && LayerState->bIsPlaying)
	{
		OnAnimationInterrupted.Broadcast(Layer, LayerState->CurrentAnimation);
		
		if (LayerState->OnInterrupted)
		{
			LayerState->OnInterrupted();
		}
	}
}

void UAnimationManager::InitializeDefaultLayers()
{
	// Initialize base layers
	AddLayer(EAnimationLayer::BaseLocomotion, 1.0f);
}

void UAnimationManager::CacheSkeletalMesh()
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		SkeletalMesh = Character->GetMesh();
	}
	else if (AActor* Owner = GetOwner())
	{
		SkeletalMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}
}

// ============================================================================
// POOLING
// ============================================================================

void UAnimationManager::SetPoolingEnabled(bool bEnable, int32 MaxConcurrent)
{
	bPoolingEnabled = bEnable;
	
	if (bEnable)
	{
		if (!PoolManager)
		{
			PoolManager = NewObject<UAnimationPoolManager>(this);
			PoolManager->Initialize(MaxConcurrent);
		}
		else
		{
			PoolManager->SetMaxConcurrentAnimations(MaxConcurrent);
		}
		
		UE_LOG(LogTemp, Log, TEXT("AnimationManager: Pooling enabled with %d max concurrent animations"), MaxConcurrent);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("AnimationManager: Pooling disabled"));
	}
}

float UAnimationManager::GetPoolUsagePercent() const
{
	return (bPoolingEnabled && PoolManager) ? PoolManager->GetPoolUsagePercent() : 0.0f;
}

int32 UAnimationManager::GetActivePooledAnimationCount() const
{
	return (bPoolingEnabled && PoolManager) ? PoolManager->GetActiveAnimationCount() : 0;
}

