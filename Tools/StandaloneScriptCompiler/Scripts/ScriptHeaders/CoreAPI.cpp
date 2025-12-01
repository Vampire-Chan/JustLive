#include "CoreAPI.h"
#include "ScriptLogger.h"
#include "Math/UnrealMathUtility.h"

namespace CoreAPI
{
    FScriptValue Print(const TArray<FScriptValue>& Args)
    {
        if (Args.Num() != 1)
        {
            // In a real implementation, we would have a way to report errors from native functions.
            return FScriptValue::Nil();
        }
        VM_LOG(FString::Printf(TEXT("[SCRIPT PRINT] %s"), *Args[0].ToString()));
        return FScriptValue::Nil();
    }

    FScriptValue LogWarning(const TArray<FScriptValue>& Args)
    {
        if (Args.Num() != 1)
        {
            return FScriptValue::Nil();
        }
        VM_LOG_WARNING(FString::Printf(TEXT("[SCRIPT WARNING] %s"), *Args[0].ToString()));
        return FScriptValue::Nil();
    }

    FScriptValue LogError(const TArray<FScriptValue>& Args)
    {
        if (Args.Num() != 1)
        {
            return FScriptValue::Nil();
        }
        VM_LOG_ERROR(FString::Printf(TEXT("[SCRIPT ERROR] %s"), *Args[0].ToString()));
        return FScriptValue::Nil();
    }

    FScriptValue RandInt(const TArray<FScriptValue>& Args)
    {
        if (Args.Num() != 2 || !Args[0].IsNumber() || !Args[1].IsNumber())
        {
            return FScriptValue::Nil();
        }
        int32 Min = static_cast<int32>(Args[0].AsNumber());
        int32 Max = static_cast<int32>(Args[1].AsNumber());
        return FScriptValue::Number(FMath::RandRange(Min, Max));
    }

    FScriptValue RandFloat(const TArray<FScriptValue>& Args)
    {
        if (Args.Num() != 2 || !Args[0].IsNumber() || !Args[1].IsNumber())
        {
            return FScriptValue::Nil();
        }
        float Min = static_cast<float>(Args[0].AsNumber());
        float Max = static_cast<float>(Args[1].AsNumber());
        return FScriptValue::Number(FMath::FRandRange(Min, Max));
    }
}