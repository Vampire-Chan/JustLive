// UINative.cpp
// UI native function registration

#include "UINative.h"
#include "ScriptLogger.h"
#include "../UI/UIManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void FUINativeReg::RegisterFunctions(FScriptVM* VM)
{
    if (!VM) return;

    SCRIPT_LOG(TEXT("[UI NATIVE REG] Registering UI functions..."));

    VM->RegisterNativeFunction(TEXT("UI_SwitchState"), SwitchState);
    VM->RegisterNativeFunction(TEXT("UI_ShowLoading"), ShowLoading);
    VM->RegisterNativeFunction(TEXT("UI_UpdateLoading"), UpdateLoading);

    SCRIPT_LOG(TEXT("[UI NATIVE REG] Registered UI functions"));
}

FScriptValue FUINativeReg::SwitchState(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    
    FString StateName = Args[0].ToString();
    EUIState State = EUIState::None;
    
    if (StateName == TEXT("Startup")) State = EUIState::Startup;
    else if (StateName == TEXT("MainMenu")) State = EUIState::MainMenu;
    else if (StateName == TEXT("Loading")) State = EUIState::Loading;
    else if (StateName == TEXT("GameHUD")) State = EUIState::GameHUD;
    else 
    {
        SCRIPT_LOG_WARNING(FString::Printf(TEXT("Unknown UI state: %s"), *StateName));
        return FScriptValue::Bool(false);
    }
    
    UUIManager* UM = GetUIManager();
    if (UM)
    {
        UM->SwitchToState(State);
        return FScriptValue::Bool(true);
    }
    
    return FScriptValue::Bool(false);
}

FScriptValue FUINativeReg::ShowLoading(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Bool(false);
    
    FString Text = Args[0].ToString();
    float Duration = (float)Args[1].AsNumber();
    
    UUIManager* UM = GetUIManager();
    if (UM)
    {
        UM->TriggerLoadingScreen(Text, Duration);
        return FScriptValue::Bool(true);
    }
    
    return FScriptValue::Bool(false);
}

FScriptValue FUINativeReg::UpdateLoading(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Bool(false);
    
    float Percent = (float)Args[0].AsNumber();
    FString Text = Args[1].ToString();
    
    UUIManager* UM = GetUIManager();
    if (UM)
    {
        UM->UpdateLoadingProgress(Percent, Text);
        return FScriptValue::Bool(true);
    }
    
    return FScriptValue::Bool(false);
}

UUIManager* FUINativeReg::GetUIManager()
{
    if (GEngine)
    {
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
            {
                if (UWorld* World = Context.World())
                {
                    if (UGameInstance* GI = World->GetGameInstance())
                    {
                        return GI->GetSubsystem<UUIManager>();
                    }
                }
            }
        }
    }
    return nullptr;
}
