// StringNative.cpp
// String native function registration

#include "StringNative.h"
#include "ScriptLogger.h"
#include "CollectionNative.h" // Needed for Split() to return a List Handle

void FStringNativeReg::RegisterFunctions(FScriptVM* VM)
{
    if (!VM) return;

    SCRIPT_LOG(TEXT("[STRING NATIVE REG] Registering string functions..."));

    VM->RegisterNativeFunction(TEXT("String_Len"), Len);
    VM->RegisterNativeFunction(TEXT("String_Sub"), Substring);
    VM->RegisterNativeFunction(TEXT("String_Find"), Find);
    VM->RegisterNativeFunction(TEXT("String_Upper"), ToUpper);
    VM->RegisterNativeFunction(TEXT("String_Lower"), ToLower);
    VM->RegisterNativeFunction(TEXT("String_Replace"), Replace);
    VM->RegisterNativeFunction(TEXT("String_Trim"), Trim);
    VM->RegisterNativeFunction(TEXT("String_Split"), Split);
    VM->RegisterNativeFunction(TEXT("String_Contains"), Contains);
    VM->RegisterNativeFunction(TEXT("String_FromChar"), FromChar);
    VM->RegisterNativeFunction(TEXT("String_ToChar"), ToChar);

    SCRIPT_LOG(TEXT("[STRING NATIVE REG] Registered string functions"));
}

FScriptValue FStringNativeReg::Len(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Number(0);
    return FScriptValue::Number(Args[0].ToString().Len());
}

FScriptValue FStringNativeReg::Substring(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::String(TEXT(""));
    FString Str = Args[0].ToString();
    int32 Start = (int32)Args[1].AsNumber();
    int32 Count = (Args.Num() > 2) ? (int32)Args[2].AsNumber() : Str.Len();
    
    if (Start < 0) Start = 0;
    if (Start >= Str.Len()) return FScriptValue::String(TEXT(""));
    
    return FScriptValue::String(Str.Mid(Start, Count));
}

FScriptValue FStringNativeReg::Find(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Number(-1);
    FString Str = Args[0].ToString();
    FString Sub = Args[1].ToString();
    return FScriptValue::Number(Str.Find(Sub));
}

FScriptValue FStringNativeReg::ToUpper(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::String(TEXT(""));
    return FScriptValue::String(Args[0].ToString().ToUpper());
}

FScriptValue FStringNativeReg::ToLower(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::String(TEXT(""));
    return FScriptValue::String(Args[0].ToString().ToLower());
}

FScriptValue FStringNativeReg::Replace(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 3) return Args.Num() > 0 ? Args[0] : FScriptValue::String(TEXT(""));
    FString Str = Args[0].ToString();
    FString From = Args[1].ToString();
    FString To = Args[2].ToString();
    return FScriptValue::String(Str.Replace(*From, *To));
}

FScriptValue FStringNativeReg::Trim(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::String(TEXT(""));
    FString Str = Args[0].ToString();
    Str.TrimStartAndEndInline();
    return FScriptValue::String(Str);
}

FScriptValue FStringNativeReg::Split(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Number(-1); // Invalid handle
    FString Str = Args[0].ToString();
    FString Delim = Args[1].ToString();
    
    TArray<FString> Parts;
    Str.ParseIntoArray(Parts, *Delim, true);
    
    // Create a new List via Collection Manager
    int32 ListHandle = FScriptCollectionManager::CreateList();
    TArray<FScriptValue>* List = FScriptCollectionManager::GetList(ListHandle);
    
    if (List)
    {
        for (const FString& Part : Parts)
        {
            List->Add(FScriptValue::String(Part));
        }
    }
    
    return FScriptValue::Number(ListHandle);
}

FScriptValue FStringNativeReg::Contains(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Bool(false);
    return FScriptValue::Bool(Args[0].ToString().Contains(Args[1].ToString()));
}

FScriptValue FStringNativeReg::FromChar(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::String(TEXT(""));
    TCHAR CharCode = (TCHAR)Args[0].AsNumber();
    return FScriptValue::String(FString().AppendChar(CharCode));
}

FScriptValue FStringNativeReg::ToChar(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Number(0);
    FString Str = Args[0].ToString();
    if (Str.Len() > 0)
    {
        return FScriptValue::Number((double)Str[0]);
    }
    return FScriptValue::Number(0);
}
