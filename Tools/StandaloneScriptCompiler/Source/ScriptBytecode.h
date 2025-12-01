// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#pragma once

#include "Platform.h"

/**
 * Bytecode operation codes for the VM
 */
UENUM()
enum class EOpCode : uint8
{
    // Constants
    OP_CONSTANT,        // Push constant from pool
    OP_NIL,            // Push nil
    OP_TRUE,           // Push true
    OP_FALSE,          // Push false
    
    // Arithmetic
    OP_ADD,            // +
    OP_SUBTRACT,       // -
    OP_MULTIPLY,       // *
    OP_DIVIDE,         // /
    OP_MODULO,         // %
    OP_NEGATE,         // Unary -
    
    // Comparison
    OP_EQUAL,          // ==
    OP_NOT_EQUAL,      // !=
    OP_GREATER,        // >
    OP_GREATER_EQUAL,  // >=
    OP_LESS,           // <
    OP_LESS_EQUAL,     // <=
    
    // Logical
    OP_NOT,            // ! (logical NOT)
    OP_AND,            // && (logical AND)
    OP_OR,             // || (logical OR)
    
    // Bitwise
    OP_BIT_AND,        // & (bitwise AND)
    OP_BIT_OR,         // | (bitwise OR)
    OP_BIT_XOR,        // ^ (bitwise XOR)
    OP_BIT_NOT,        // ~ (bitwise NOT/complement)
    
    // Variables
    OP_DEFINE_GLOBAL,  // Define global variable
    OP_GET_GLOBAL,     // Get global variable
    OP_SET_GLOBAL,     // Set global variable
    OP_GET_LOCAL,      // Get local variable
    OP_SET_LOCAL,      // Set local variable
    
    // Control flow
    OP_JUMP,           // Unconditional jump
    OP_JUMP_IF_FALSE,  // Jump if false
    OP_LOOP,           // Jump backwards
    OP_BREAK,          // Break from loop
    OP_CONTINUE,       // Continue loop
    
    // Functions
    OP_CALL,           // Call function
    OP_CALL_NATIVE,    // Call native function
    OP_RETURN,         // Return from function
    
    // Type conversions
    OP_CAST_INT,       // Cast to int
    OP_CAST_FLOAT,     // Cast to float
    OP_CAST_STRING,    // Cast to string
    
    // Stack
    OP_POP,            // Pop top of stack
    OP_PRINT,          // Print value (debugging)
    
    // Arrays
    OP_CREATE_ARRAY,   // Create array from elements on stack
    OP_GET_ELEMENT,    // Get array element: arr[index]
    OP_SET_ELEMENT,    // Set array element: arr[index] = value
    OP_DUPLICATE,      // Duplicate top of stack
    
    // Structs
    OP_GET_FIELD,      // Get struct field: obj.field
    OP_SET_FIELD,      // Set struct field: obj.field = value
    
    // End
    OP_HALT            // Stop execution
};

/**
 * Compiler types for bytecode verification
 */
UENUM()
enum class ECompilerType : uint8
{
    // Official Unreal Engine integrated compiler (TRUSTED)
    UnrealEngineCompiler = 0,
    
    // Official standalone compiler tool (TRUSTED if signed)
    StandaloneCompiler = 1,
    
    // External/third-party compiler (UNTRUSTED - requires validation)
    ExternalCompiler = 2,
    
    // Unknown/legacy compiler
    Unknown = 255
};

/**
 * Compiler flags for bytecode security
 */
UENUM()
enum class EScriptCompilerFlags : uint32
{
    None = 0,
    
    // Set by UE compiler - indicates official compilation
    OfficialBuild = 1 << 0,
    
    // Set when compiled in editor vs runtime
    EditorCompiled = 1 << 1,
    
    // Set for development builds (more debug info)
    DevelopmentBuild = 1 << 2,
    
    // Set for shipping builds (optimized)
    ShippingBuild = 1 << 3,
    
    // Set when source code is included
    IncludesSource = 1 << 4,
    
    // Set when debug symbols are included
    DebugSymbols = 1 << 5,
    
    // Set when bytecode is signed with trusted key
    TrustedSigned = 1 << 6,
    
    // Set when bytecode passed all security checks
    SecurityVerified = 1 << 7
};

ENUM_CLASS_FLAGS(EScriptCompilerFlags);

/**
 * Runtime value types
 */
UENUM()
enum class EValueType : uint8
{
    NIL,
    BOOL,
    NUMBER,
    STRING,
    ARRAY
};

/**
 * Runtime value container
 */
struct SCRIPTING_API FScriptValue
{
    EValueType Type;
    
    union
    {
        bool BoolValue;
        double NumberValue;
    };
    
    FString StringValue;
    TArray<FScriptValue> ArrayValue;
    
    FScriptValue()
        : Type(EValueType::NIL)
        , NumberValue(0.0)
    {}
    
    static FScriptValue Nil()
    {
        FScriptValue Val;
        Val.Type = EValueType::NIL;
        return Val;
    }
    
    static FScriptValue Bool(bool Value)
    {
        FScriptValue Val;
        Val.Type = EValueType::BOOL;
        Val.BoolValue = Value;
        return Val;
    }
    
    static FScriptValue Number(double Value)
    {
        FScriptValue Val;
        Val.Type = EValueType::NUMBER;
        Val.NumberValue = Value;
        return Val;
    }
    
    static FScriptValue String(const FString& Value)
    {
        FScriptValue Val;
        Val.Type = EValueType::STRING;
        Val.StringValue = Value;
        return Val;
    }
    
    static FScriptValue Array(const TArray<FScriptValue>& Value)
    {
        FScriptValue Val;
        Val.Type = EValueType::ARRAY;
        Val.ArrayValue = Value;
        return Val;
    }
    
    bool IsTruthy() const
    {
        switch (Type)
        {
            case EValueType::NIL: return false;
            case EValueType::BOOL: return BoolValue;
            case EValueType::NUMBER: return NumberValue != 0.0;
            case EValueType::STRING: return !StringValue.IsEmpty();
            case EValueType::ARRAY: return ArrayValue.Num() > 0;
            default: return false;
        }
    }
    
    FString ToString() const
    {
        switch (Type)
        {
            case EValueType::NIL: return TEXT("nil");
            case EValueType::BOOL: return BoolValue ? TEXT("true") : TEXT("false");
            case EValueType::NUMBER: return FString::SanitizeFloat(NumberValue);
            case EValueType::STRING: return StringValue;
            case EValueType::ARRAY: 
            {
                FString Result = TEXT("[");
                for (int32 i = 0; i < ArrayValue.Num(); ++i)
                {
                    if (i > 0) Result += TEXT(", ");
                    Result += ArrayValue[i].ToString();
                }
                Result += TEXT("]");
                return Result;
            }
            default: return TEXT("<unknown>");
        }
    }
    
    bool IsNumber() const { return Type == EValueType::NUMBER; }
    bool IsString() const { return Type == EValueType::STRING; }
    bool IsBool() const { return Type == EValueType::BOOL; }
    bool IsNil() const { return Type == EValueType::NIL; }
    bool IsArray() const { return Type == EValueType::ARRAY; }
    
    double AsNumber() const { return NumberValue; }
    const FString& AsString() const { return StringValue; }
    bool AsBool() const { return BoolValue; }
    const TArray<FScriptValue>& AsArray() const { return ArrayValue; }
};

/**
 * Debug information for a bytecode instruction
 */
struct FDebugInfo
{
    int32 Line;
    int32 Column;
    FString SourceFile;
    
    FDebugInfo()
        : Line(0), Column(0)
    {}
    
    FDebugInfo(int32 InLine, int32 InColumn, const FString& InFile = TEXT(""))
        : Line(InLine), Column(InColumn), SourceFile(InFile)
    {}
};

/**
 * Function information for bytecode
 */
struct SCRIPTING_API FFunctionInfo
{
    FString Name;
    int32 Address;
    int32 Arity;
    
    FFunctionInfo()
        : Address(-1), Arity(0)
    {}
    
    FFunctionInfo(const FString& InName, int32 InAddress, int32 InArity)
        : Name(InName), Address(InAddress), Arity(InArity)
    {}
};

/**
 * Metadata header for compiled bytecode
 */
struct SCRIPTING_API FBytecodeMetadata
{
    // Compiler information
    ECompilerType CompilerType;
    EScriptCompilerFlags CompilerFlags;
    FString CompilerName;
    FString CompilerVersion;
    
    // Engine information
    FString EngineVersion;
    FString GameName;
    FString GameVersion;
    
    // User & system information
    FString AuthorName;
    FString OperatingSystem;
    FString MachineName;
    FDateTime CompilationTime;
    
    // Source information
    FString SourceFileName;
    uint32 SourceFileSize;
    FString SourceChecksum;  // MD5 hash of source

    // Script properties
    bool bIsMission = false;
    
    FBytecodeMetadata()
        : CompilerType(ECompilerType::UnrealEngineCompiler)
        , CompilerFlags(EScriptCompilerFlags::None)
        , CompilerName(TEXT("UnrealEngineCompiler"))
        , CompilerVersion(TEXT("SBS Compiler C 2025 V1.0"))
        , GameName(TEXT("Sandbox Game"))
        , GameVersion(TEXT("V1.0"))
        , SourceFileSize(0)
    {
        // Auto-fill engine version
        EngineVersion = FString::Printf(TEXT("UE %d.%d"), ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION);
        
        // Auto-fill system info
#if PLATFORM_WINDOWS
        OperatingSystem = TEXT("Windows");
#elif PLATFORM_MAC
        OperatingSystem = TEXT("Mac");
#elif PLATFORM_LINUX
        OperatingSystem = TEXT("Linux");
#else
        OperatingSystem = TEXT("Unknown");
#endif
        
        MachineName = FPlatformProcess::ComputerName();
        AuthorName = FPlatformProcess::UserName();
        CompilationTime = FDateTime::Now();
        
        // Set official flags for UE compiler
#if UE_BUILD_DEVELOPMENT
        CompilerFlags = EScriptCompilerFlags::OfficialBuild | EScriptCompilerFlags::EditorCompiled | 
                       EScriptCompilerFlags::DevelopmentBuild | EScriptCompilerFlags::DebugSymbols |
                       EScriptCompilerFlags::TrustedSigned | EScriptCompilerFlags::SecurityVerified;
#elif UE_BUILD_SHIPPING
        CompilerFlags = EScriptCompilerFlags::OfficialBuild | EScriptCompilerFlags::ShippingBuild | 
                       EScriptCompilerFlags::TrustedSigned | EScriptCompilerFlags::SecurityVerified;
#else
        CompilerFlags = EScriptCompilerFlags::OfficialBuild | EScriptCompilerFlags::EditorCompiled | 
                       EScriptCompilerFlags::DebugSymbols | EScriptCompilerFlags::TrustedSigned;
#endif
    }
};

/**
 * Compiled bytecode chunk
 */
struct SCRIPTING_API FBytecodeChunk
{
    // Version for compatibility checking
    int32 Version = 2;
    FBytecodeMetadata Metadata;
    
    // Digital signature for verification
    FString Signature;
    
    // Bytecode instructions
    TArray<uint8> Code;
    
    // Constant pool
    TArray<FScriptValue> Constants;
    
    // Function table
    TArray<FFunctionInfo> Functions;
    
    // Line numbers for debugging
    TArray<int32> LineNumbers;
    
    // Debug information (not serialized in shipping builds)
    TArray<FDebugInfo> DebugInfo;
    
    // Source code hash (for cache validation)
    FString SourceHash;
    
    FBytecodeChunk()
        : Version(1)
    {}
    
    void WriteByte(uint8 Byte, int32 Line = 0)
    {
        Code.Add(Byte);
        
        #if !UE_BUILD_SHIPPING
        DebugInfo.Add(FDebugInfo(Line, 0));
        #endif
    }
    
    void WriteBytes(uint8 Byte1, uint8 Byte2, int32 Line = 0)
    {
        WriteByte(Byte1, Line);
        WriteByte(Byte2, Line);
    }
    
    int32 AddConstant(const FScriptValue& Value)
    {
        // Check if constant already exists (optimization)
        for (int32 i = 0; i < Constants.Num(); ++i)
        {
            const FScriptValue& Existing = Constants[i];
            if (Existing.Type == Value.Type)
            {
                switch (Value.Type)
                {
                    case EValueType::NIL:
                        return i;
                    case EValueType::BOOL:
                        if (Existing.BoolValue == Value.BoolValue) return i;
                        break;
                    case EValueType::NUMBER:
                        if (FMath::IsNearlyEqual(Existing.NumberValue, Value.NumberValue))
                            return i;
                        break;
                    case EValueType::STRING:
                        if (Existing.StringValue == Value.StringValue) return i;
                        break;
                }
            }
        }
        
        Constants.Add(Value);
        return Constants.Num() - 1;
    }
    
    void Clear()
    {
        Code.Empty();
        Constants.Empty();
        DebugInfo.Empty();
    }
    
    // Disassemble for debugging
    FString Disassemble() const;
    
    // Decompile bytecode back to script-like representation
    FString Decompile() const;
    
    // Serialize bytecode to binary array (with compression)
    bool Serialize(TArray<uint8>& OutData, bool bCompress = true) const;
    
    // Deserialize bytecode from binary array (with decompression)
    bool Deserialize(const TArray<uint8>& InData);
    
    // Generate digital signature for the bytecode
    FString GenerateSignature() const;
    
    // Verify digital signature
    bool VerifySignature(const FString& ExpectedSignature) const;
    
    // Validate bytecode security (compiler type, flags, signature)
    bool ValidateSecurity(FString& OutReason) const;
    
    // Check if bytecode is from trusted compiler
    bool IsTrustedCompiler() const;
    
private:
    // Calculate SHA256 hash
    static FString CalculateSHA256(const TArray<uint8>& Data);
};


