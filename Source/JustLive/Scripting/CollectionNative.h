// CollectionNative.h
// Central Manager for Script Collections (Lists, Dictionaries)
// Stores actual data in C++ memory, Scripts only hold Integer Handles.

#pragma once

#include "CoreMinimal.h"
#include "ScriptVM.h"

/**
 * Central Manager for all Script-created Collections.
 * Allows C++ code to directly access Lists/Dicts created by Scripts.
 */
class JUSTLIVE_API FScriptCollectionManager
{
public:
    // ========================================================================
    // Native API Registration
    // ========================================================================
    static void RegisterFunctions(FScriptVM* VM);
    static void Cleanup(); // Call on shutdown

    // ========================================================================
    // C++ Accessors (Engine Access)
    // ========================================================================
    
    /** Retrieve a pointer to a List by its handle. Returns nullptr if invalid. */
    static TArray<FScriptValue>* GetList(int32 Handle);
    
    /** Retrieve a pointer to a Dictionary by its handle. Returns nullptr if invalid. */
    static TMap<FString, FScriptValue>* GetDictionary(int32 Handle);

    /** Create a new List from C++ and return its handle for script use */
    static int32 CreateList();

    /** Create a new Dictionary from C++ and return its handle for script use */
    static int32 CreateDictionary();

private:
    // ========================================================================
    // Native Functions (Script Callable)
    // ========================================================================
    
    // List Operations
    static FScriptValue List_Create(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue List_Add(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue List_Get(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue List_Set(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue List_RemoveAt(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue List_Count(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue List_Clear(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue List_Contains(FScriptVM* VM, const TArray<FScriptValue>& Args);

    // Dictionary Operations
    static FScriptValue Dict_Create(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Dict_Set(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Dict_Get(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Dict_Remove(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Dict_HasKey(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Dict_Clear(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Dict_Count(FScriptVM* VM, const TArray<FScriptValue>& Args);

    // ========================================================================
    // Internal Storage
    // ========================================================================
    static int32 NextListHandle;
    static TMap<int32, TArray<FScriptValue>> Lists;

    static int32 NextDictHandle;
    static TMap<int32, TMap<FString, FScriptValue>> Dictionaries;
};