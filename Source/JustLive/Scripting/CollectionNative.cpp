// CollectionNative.cpp
// Central Manager for Script Collections

#include "CollectionNative.h"
#include "ScriptLogger.h"

// Initialize static members
int32 FScriptCollectionManager::NextListHandle = 1;
TMap<int32, TArray<FScriptValue>> FScriptCollectionManager::Lists;

int32 FScriptCollectionManager::NextDictHandle = 1;
TMap<int32, TMap<FString, FScriptValue>> FScriptCollectionManager::Dictionaries;

void FScriptCollectionManager::RegisterFunctions(FScriptVM* VM)
{
    if (!VM) return;

    SCRIPT_LOG(TEXT("[COLLECTION MANAGER] Registering collection functions..."));

    // List API
    VM->RegisterNativeFunction(TEXT("List_Create"), List_Create);
    VM->RegisterNativeFunction(TEXT("List_Add"), List_Add);
    VM->RegisterNativeFunction(TEXT("List_Get"), List_Get);
    VM->RegisterNativeFunction(TEXT("List_Set"), List_Set);
    VM->RegisterNativeFunction(TEXT("List_RemoveAt"), List_RemoveAt);
    VM->RegisterNativeFunction(TEXT("List_Count"), List_Count);
    VM->RegisterNativeFunction(TEXT("List_Clear"), List_Clear);
    VM->RegisterNativeFunction(TEXT("List_Contains"), List_Contains);

    // Dictionary API
    VM->RegisterNativeFunction(TEXT("Dict_Create"), Dict_Create);
    VM->RegisterNativeFunction(TEXT("Dict_Set"), Dict_Set);
    VM->RegisterNativeFunction(TEXT("Dict_Get"), Dict_Get);
    VM->RegisterNativeFunction(TEXT("Dict_Remove"), Dict_Remove);
    VM->RegisterNativeFunction(TEXT("Dict_HasKey"), Dict_HasKey);
    VM->RegisterNativeFunction(TEXT("Dict_Clear"), Dict_Clear);
    VM->RegisterNativeFunction(TEXT("Dict_Count"), Dict_Count);

    SCRIPT_LOG(TEXT("[COLLECTION MANAGER] Registered collection functions"));
}

void FScriptCollectionManager::Cleanup()
{
    Lists.Empty();
    Dictionaries.Empty();
    NextListHandle = 1;
    NextDictHandle = 1;
}

// ============================================================================
// C++ Accessors (Engine Access)
// ============================================================================

TArray<FScriptValue>* FScriptCollectionManager::GetList(int32 Handle)
{
    return Lists.Find(Handle);
}

TMap<FString, FScriptValue>* FScriptCollectionManager::GetDictionary(int32 Handle)
{
    return Dictionaries.Find(Handle);
}

int32 FScriptCollectionManager::CreateList()
{
    int32 Handle = NextListHandle++;
    Lists.Add(Handle, TArray<FScriptValue>());
    return Handle;
}

int32 FScriptCollectionManager::CreateDictionary()
{
    int32 Handle = NextDictHandle++;
    Dictionaries.Add(Handle, TMap<FString, FScriptValue>());
    return Handle;
}

// ============================================================================
// List Operations (Native API)
// ============================================================================

FScriptValue FScriptCollectionManager::List_Create(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    return FScriptValue::Number(CreateList());
}

FScriptValue FScriptCollectionManager::List_Add(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Bool(false);
    int32 Handle = (int32)Args[0].AsNumber();
    
    if (TArray<FScriptValue>* List = GetList(Handle))
    {
        List->Add(Args[1]);
        return FScriptValue::Bool(true);
    }
    return FScriptValue::Bool(false);
}

FScriptValue FScriptCollectionManager::List_Get(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Nil();
    int32 Handle = (int32)Args[0].AsNumber();
    int32 Index = (int32)Args[1].AsNumber();
    
    if (TArray<FScriptValue>* List = GetList(Handle))
    {
        if (List->IsValidIndex(Index))
        {
            return (*List)[Index];
        }
    }
    return FScriptValue::Nil();
}

FScriptValue FScriptCollectionManager::List_Set(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 3) return FScriptValue::Bool(false);
    int32 Handle = (int32)Args[0].AsNumber();
    int32 Index = (int32)Args[1].AsNumber();
    
    if (TArray<FScriptValue>* List = GetList(Handle))
    {
        if (List->IsValidIndex(Index))
        {
            (*List)[Index] = Args[2];
            return FScriptValue::Bool(true);
        }
    }
    return FScriptValue::Bool(false);
}

FScriptValue FScriptCollectionManager::List_RemoveAt(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Bool(false);
    int32 Handle = (int32)Args[0].AsNumber();
    int32 Index = (int32)Args[1].AsNumber();
    
    if (TArray<FScriptValue>* List = GetList(Handle))
    {
        if (List->IsValidIndex(Index))
        {
            List->RemoveAt(Index);
            return FScriptValue::Bool(true);
        }
    }
    return FScriptValue::Bool(false);
}

FScriptValue FScriptCollectionManager::List_Count(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Number(0);
    int32 Handle = (int32)Args[0].AsNumber();
    
    if (TArray<FScriptValue>* List = GetList(Handle))
    {
        return FScriptValue::Number(List->Num());
    }
    return FScriptValue::Number(0);
}

FScriptValue FScriptCollectionManager::List_Clear(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    int32 Handle = (int32)Args[0].AsNumber();
    
    if (TArray<FScriptValue>* List = GetList(Handle))
    {
        List->Empty();
        return FScriptValue::Bool(true);
    }
    return FScriptValue::Bool(false);
}

FScriptValue FScriptCollectionManager::List_Contains(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Bool(false);
    int32 Handle = (int32)Args[0].AsNumber();
    
    if (TArray<FScriptValue>* List = GetList(Handle))
    {
        // Manual search since FScriptValue comparison needs care
        for (const FScriptValue& Val : *List)
        {
            // Simple type check first
            if (Val.Type != Args[1].Type) continue;

            // Value check
            if (Val.Type == EValueType::NUMBER && FMath::IsNearlyEqual(Val.NumberValue, Args[1].NumberValue)) return FScriptValue::Bool(true);
            if (Val.Type == EValueType::STRING && Val.StringValue == Args[1].StringValue) return FScriptValue::Bool(true);
            if (Val.Type == EValueType::BOOL && Val.BoolValue == Args[1].BoolValue) return FScriptValue::Bool(true);
        }
    }
    return FScriptValue::Bool(false);
}

// ============================================================================
// Dictionary Operations (Native API)
// ============================================================================

FScriptValue FScriptCollectionManager::Dict_Create(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    return FScriptValue::Number(CreateDictionary());
}

FScriptValue FScriptCollectionManager::Dict_Set(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 3) return FScriptValue::Bool(false);
    int32 Handle = (int32)Args[0].AsNumber();
    FString Key = Args[1].ToString();
    
    if (TMap<FString, FScriptValue>* Dict = GetDictionary(Handle))
    {
        Dict->Add(Key, Args[2]);
        return FScriptValue::Bool(true);
    }
    return FScriptValue::Bool(false);
}

FScriptValue FScriptCollectionManager::Dict_Get(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Nil();
    int32 Handle = (int32)Args[0].AsNumber();
    FString Key = Args[1].ToString();
    
    if (TMap<FString, FScriptValue>* Dict = GetDictionary(Handle))
    {
        if (FScriptValue* Val = Dict->Find(Key))
        {
            return *Val;
        }
    }
    return FScriptValue::Nil();
}

FScriptValue FScriptCollectionManager::Dict_Remove(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Bool(false);
    int32 Handle = (int32)Args[0].AsNumber();
    FString Key = Args[1].ToString();
    
    if (TMap<FString, FScriptValue>* Dict = GetDictionary(Handle))
    {
        return FScriptValue::Bool(Dict->Remove(Key) > 0);
    }
    return FScriptValue::Bool(false);
}

FScriptValue FScriptCollectionManager::Dict_HasKey(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Bool(false);
    int32 Handle = (int32)Args[0].AsNumber();
    FString Key = Args[1].ToString();
    
    if (TMap<FString, FScriptValue>* Dict = GetDictionary(Handle))
    {
        return FScriptValue::Bool(Dict->Contains(Key));
    }
    return FScriptValue::Bool(false);
}

FScriptValue FScriptCollectionManager::Dict_Clear(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Bool(false);
    int32 Handle = (int32)Args[0].AsNumber();
    
    if (TMap<FString, FScriptValue>* Dict = GetDictionary(Handle))
    {
        Dict->Empty();
        return FScriptValue::Bool(true);
    }
    return FScriptValue::Bool(false);
}

FScriptValue FScriptCollectionManager::Dict_Count(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Number(0);
    int32 Handle = (int32)Args[0].AsNumber();
    
    if (TMap<FString, FScriptValue>* Dict = GetDictionary(Handle))
    {
        return FScriptValue::Number(Dict->Num());
    }
    return FScriptValue::Number(0);
}