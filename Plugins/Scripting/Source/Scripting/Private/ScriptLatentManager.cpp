// Copyright Vampire Game Project. All Rights Reserved.

#include "ScriptLatentManager.h"
#include "ScriptLogger.h"
#include "Engine/World.h"

void UScriptLatentManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    SCRIPT_LOG(TEXT("Latent Script Manager Initialized"));
}

void UScriptLatentManager::Deinitialize()
{
    SleepingScripts.Empty();
    Super::Deinitialize();
}

void UScriptLatentManager::Tick(float DeltaTime)
{
    if (SleepingScripts.Num() == 0)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    double CurrentTime = World->GetTimeSeconds();

    // Iterate backwards so we can remove items safely
    for (int32 i = SleepingScripts.Num() - 1; i >= 0; --i)
    {
        FPendingLatentScript& Script = SleepingScripts[i];
        
        if (CurrentTime >= Script.ResumeTime)
        {
            // Time to wake up!
            TSharedPtr<FScriptVM> VM = Script.VM;
            
            // Remove from list before executing (in case it sleeps again immediately)
            SleepingScripts.RemoveAt(i);
            
            if (VM.IsValid())
            {
                // Resume execution
                // Note: We need to ensure the VM is in a state where it *can* be resumed.
                // The Resume() method (which we will add to ScriptVM) should handle this.
                VM->Resume();
            }
        }
    }
}

void UScriptLatentManager::RequestSleep(TSharedPtr<FScriptVM> VM, float DurationSeconds)
{
    if (!VM.IsValid())
    {
        SCRIPT_LOG_ERROR(TEXT("Attempted to sleep invalid VM"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    double ResumeTime = World->GetTimeSeconds() + DurationSeconds;
    SleepingScripts.Add(FPendingLatentScript(VM, ResumeTime));
    
    // Set VM state to Paused so it stops its current execution loop
    // We will need to expose a method on VM for this or handle it via return value
    VM->Pause(); 
}

