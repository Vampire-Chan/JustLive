// LightNative.cpp
// Light native function registration

#include "LightNative.h"
#include "ScriptLogger.h"
#include "Engine/Light.h"
#include "Components/LightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void FLightNativeReg::RegisterFunctions(FScriptVM* VM)
{
    if (!VM) return;

    SCRIPT_LOG(TEXT("[LIGHT NATIVE REG] Registering light functions..."));

    VM->RegisterNativeFunction(TEXT("Light_SetColor"), SetLightColor);
    VM->RegisterNativeFunction(TEXT("Light_SetIntensity"), SetLightIntensity);
    VM->RegisterNativeFunction(TEXT("Light_Toggle"), ToggleLight);

    SCRIPT_LOG(TEXT("[LIGHT NATIVE REG] Registered light functions"));
}

FScriptValue FLightNativeReg::SetLightColor(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 4) return FScriptValue::Bool(false);
    
    FString Name = Args[0].ToString();
    float R = (float)Args[1].AsNumber();
    float G = (float)Args[2].AsNumber();
    float B = (float)Args[3].AsNumber();
    
    ALight* Light = FindLight(Name);
    if (Light && Light->GetLightComponent())
    {
        Light->GetLightComponent()->SetLightColor(FLinearColor(R, G, B));
        return FScriptValue::Bool(true);
    }
    
    return FScriptValue::Bool(false);
}

FScriptValue FLightNativeReg::SetLightIntensity(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Bool(false);
    
    FString Name = Args[0].ToString();
    float Intensity = (float)Args[1].AsNumber();
    
    ALight* Light = FindLight(Name);
    if (Light && Light->GetLightComponent())
    {
        Light->GetLightComponent()->SetIntensity(Intensity);
        return FScriptValue::Bool(true);
    }
    
    return FScriptValue::Bool(false);
}

FScriptValue FLightNativeReg::ToggleLight(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Bool(false);
    
    FString Name = Args[0].ToString();
    bool bVisible = Args[1].AsBool();
    
    ALight* Light = FindLight(Name);
    if (Light && Light->GetLightComponent())
    {
        Light->GetLightComponent()->SetVisibility(bVisible);
        return FScriptValue::Bool(true);
    }
    
    return FScriptValue::Bool(false);
}

ALight* FLightNativeReg::FindLight(const FString& Name)
{
    if (!GEngine) return nullptr;
    
    UWorld* World = nullptr;
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
        {
            World = Context.World();
            break;
        }
    }
    
    if (!World) return nullptr;
    
    // Slow search, but functional for now
    TArray<AActor*> Lights;
    UGameplayStatics::GetAllActorsOfClass(World, ALight::StaticClass(), Lights);
    
    for (AActor* Actor : Lights)
    {
        if (Actor->GetName().Contains(Name) || Actor->ActorHasTag(FName(*Name)))
        {
            return Cast<ALight>(Actor);
        }
    }
    
    return nullptr;
}
