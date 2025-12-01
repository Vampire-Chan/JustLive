#pragma once

#include "ScriptVM.h"

namespace CoreAPI
{
    FScriptValue Print(const TArray<FScriptValue>& Args);
    FScriptValue LogWarning(const TArray<FScriptValue>& Args);
    FScriptValue LogError(const TArray<FScriptValue>& Args);
    FScriptValue RandInt(const TArray<FScriptValue>& Args);
    FScriptValue RandFloat(const TArray<FScriptValue>& Args);
}