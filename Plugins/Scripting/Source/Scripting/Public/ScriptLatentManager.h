// Copyright Vampire Game Project. All Rights Reserved.
// Manages latent (sleeping/waiting) script execution without blocking the game thread.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ScriptVM.h"
#include "ScriptLatentManager.generated.h"

/**
 * Represents a script VM that is currently paused/sleeping
 */
struct FPendingLatentScript
{
    TSharedPtr<FScriptVM> VM;
    double ResumeTime; // World time in seconds when script should resume
    
    FPendingLatentScript(TSharedPtr<FScriptVM> InVM, double InResumeTime)
        : VM(InVM)
        , ResumeTime(InResumeTime)
    {}
};

/**
 * Subsystem to handle latent script actions (Sleep, Wait, etc.)
 */
UCLASS()
class SCRIPTING_API UScriptLatentManager : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    // Subsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // FTickableGameObject interface
    virtual void Tick(float DeltaTime) override;
    virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
    virtual TStatId GetStatId() const override { return TStatId(); }

    /**
     * Request a script to sleep for a duration
     * @param VM - The VM instance to pause
     * @param DurationSeconds - How long to sleep
     */
    void RequestSleep(TSharedPtr<FScriptVM> VM, float DurationSeconds);

private:
    // List of scripts currently sleeping
    TArray<FPendingLatentScript> SleepingScripts;
};

