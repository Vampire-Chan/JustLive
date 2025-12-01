// Copyright Vampire Game Project. All Rights Reserved.
// Advanced type system for scripting language (arrays, structs, switch/case)

#pragma once

#include "CoreMinimal.h"
#include "ScriptToken.h"
#include "ScriptAST.h"  // Contains the base EScriptType enum
#include "ScriptBytecode.h"

// Extending EScriptType with additional values
// ARRAY and STRUCT are handled as special cases in the type system
// since we can't extend UENUM at runtime, we'll handle these specially in code

/**
 * Type information for arrays
 */
struct FArrayType
{
    EScriptType ElementType;
    int32 Size; // -1 for dynamic arrays, positive for fixed size
    
    FArrayType() : ElementType(EScriptType::AUTO), Size(-1) {}
    FArrayType(EScriptType InElementType, int32 InSize = -1) : ElementType(InElementType), Size(InSize) {}
};

/**
 * Type information for structs
 */
struct FStructType
{
    FString Name; // Name of the struct
    TArray<FString> FieldNames; // Field names in the struct
    TArray<EScriptType> FieldTypes; // Types of each field

    FStructType() {}
    FStructType(const FString& InName) : Name(InName) {}
};

// Extended type information to handle complex types
struct FExtendedType
{
    EScriptType BaseType;
    bool bIsArray;
    bool bIsStruct;
    FArrayType ArrayInfo;
    FStructType StructInfo;
    
    FExtendedType() : BaseType(EScriptType::AUTO), bIsArray(false), bIsStruct(false) {}
    
    static FExtendedType Simple(EScriptType Type)
    {
        FExtendedType Result;
        Result.BaseType = Type;
        Result.bIsArray = false;
        Result.bIsStruct = false;
        return Result;
    }
    
    static FExtendedType Array(EScriptType ElementType, int32 Size = -1)
    {
        FExtendedType Result;
        Result.BaseType = EScriptType::AUTO;  // We'll use bIsArray flag to indicate it's an array
        Result.bIsArray = true;
        Result.bIsStruct = false;
        Result.ArrayInfo = FArrayType(ElementType, Size);
        return Result;
    }
    
    static FExtendedType Struct(const FString& Name)
    {
        FExtendedType Result;
        Result.BaseType = EScriptType::AUTO;  // We'll use bIsStruct flag to indicate it's a struct
        Result.bIsArray = false;
        Result.bIsStruct = true;
        Result.StructInfo = FStructType(Name);
        return Result;
    }
};
