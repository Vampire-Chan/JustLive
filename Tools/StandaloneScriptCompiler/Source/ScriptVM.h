// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#pragma once

#include "Platform.h"
#include "ScriptBytecode.h"
#include "ScriptAST.h"  // For EScriptType enum

/**
 * Call frame for function execution
 */
struct SCRIPTING_API FCallFrame
{
    int32 FunctionAddress;      // Where the function starts in bytecode
    int32 ReturnAddress;        // Where to return after function completes
    int32 StackBase;            // Base of this frame's stack
    FString FunctionName;       // For debugging
    
    FCallFrame()
        : FunctionAddress(0)
        , ReturnAddress(0)
        , StackBase(0)
    {}
    
    FCallFrame(int32 InFuncAddr, int32 InRetAddr, int32 InStackBase, const FString& InName = TEXT(""))
        : FunctionAddress(InFuncAddr)
        , ReturnAddress(InRetAddr)
        , StackBase(InStackBase)
        , FunctionName(InName)
    {}
};

/**
 * Native function signature
 * Takes array of arguments, returns a value
 */
typedef TFunction<FScriptValue(const TArray<FScriptValue>&)> FNativeFunction;

/**
 * Virtual Machine for executing bytecode
 * Stack-based architecture with safety limits
 */
// Native function typedef
using FNativeFunction = std::function<void(TArray<class FScriptValue>&)>;

class SCRIPTING_API FScriptVM
{
public:
    FScriptVM();
    
    /**
     * Execute bytecode chunk
     * Returns true if execution completed successfully
     */
    bool Execute(TSharedPtr<FBytecodeChunk> Bytecode);
    
    /**
     * Register a native function that scripts can call
     */
    void RegisterNativeFunction(const FString& Name, FNativeFunction Function);
    
    /**
     * Call Main() entry point if it exists in the script
     * Returns true if Main() was found and called successfully
     * If Main() doesn't exist, returns false (not an error)
     */
    bool CallMainIfExists();
    
    /**
     * Get execution errors
     */
    const TArray<FString>& GetErrors() const { return Errors; }
    bool HasErrors() const { return Errors.Num() > 0; }
    
    /**
     * Reset VM state (clears stack, call frames, errors)
     */
    void Reset();
    
    /**
     * Get current stack for debugging
     */
    const TArray<FScriptValue>& GetStack() const { return Stack; }
    
    /**
     * Execution limits for security
     */
    struct FExecutionLimits
    {
        int32 MaxInstructionsPerFrame = 1000000;  // 1M instructions per Execute()
        int32 MaxStackDepth = 256;                // Max stack size
        int32 MaxCallDepth = 64;                  // Max function call depth
        double MaxExecutionTimeMs = 50.0;         // Increased for debug builds with logging
        
        FExecutionLimits() {}
    };
    
    void SetExecutionLimits(const FExecutionLimits& InLimits) { Limits = InLimits; }
    const FExecutionLimits& GetExecutionLimits() const { return Limits; }

private:
    // Stack machine state
    TArray<FScriptValue> Stack;
    TArray<FCallFrame> CallFrames;
    TSharedPtr<FBytecodeChunk> CurrentBytecode;
    int32 InstructionPointer;
    
    // Native function registry
    TMap<FString, FNativeFunction> NativeFunctions;
    
    // Global variable storage
    TMap<FString, FScriptValue> Globals;
    
    // Function table for user-defined functions
    struct FFunctionInfo
    {
        FString Name;
        int32 Address;
        int32 Arity; // Number of parameters
        EScriptType ReturnType;
        
        FFunctionInfo() : Address(-1), Arity(0), ReturnType(EScriptType::VOID) {}
        FFunctionInfo(const FString& InName, int32 InAddress, int32 InArity, EScriptType InRetType)
            : Name(InName), Address(InAddress), Arity(InArity), ReturnType(InRetType) {}
    };
    TArray<FFunctionInfo> FunctionTable;
    
    // Execution limits and tracking
    FExecutionLimits Limits;
    int32 InstructionCount;
    double ExecutionStartTime;
    
    // Error tracking
    TArray<FString> Errors;
    
    //=============================================================================
    // Stack Operations
    //=============================================================================
    
    void Push(const FScriptValue& Value);
    FScriptValue Pop();
    FScriptValue Peek(int32 Offset = 0) const;
    
    //=============================================================================
    // Error Handling
    //=============================================================================
    
    void RuntimeError(const FString& Message);
    bool CheckStackOverflow();
    bool CheckCallDepth();
    bool CheckInstructionLimit();
    bool CheckTimeout();
    
    //=============================================================================
    // Instruction Execution
    //=============================================================================
    
    bool ExecuteInstruction();
    
    // Opcode handlers
    void OpConstant();
    void OpNil();
    void OpTrue();
    void OpFalse();
    
    void OpAdd();
    void OpSubtract();
    void OpMultiply();
    void OpDivide();
    void OpNegate();
    
    void OpEqual();
    void OpGreater();
    void OpLess();
    void OpNot();
    void OpAnd();
    void OpOr();
    
    void OpGetLocal();
    void OpSetLocal();
    void OpDefineGlobal();
    void OpGetGlobal();
    void OpSetGlobal();
    
    void OpJump();
    void OpJumpIfFalse();
    void OpLoop();
    
    void OpCall();
    void OpCallNative();
    void OpReturn();
    
    void OpCastInt();
    void OpCastFloat();
    void OpCastString();
    
    void OpPop();
    void OpPrint();
    
    // Missing opcode handlers
    void OpNotEqual();
    void OpGreaterEqual();
    void OpLessEqual();
    void OpCreateArray();
    void OpGetElement();
    void OpSetElement();
    void OpDuplicate();
    
    // Additional structure opcodes that were defined but not implemented
    void OpGetField();
    void OpSetField();
    
    //=============================================================================
    // Helper Methods
    //=============================================================================
    
    uint8 ReadByte();
    uint16 ReadShort();
    FScriptValue ReadConstant();
    
    // Type checking and conversion
    bool IsTruthy(const FScriptValue& Value) const;
    bool AreEqual(const FScriptValue& A, const FScriptValue& B) const;
    
    // Debugging
    void DumpStack() const;
};



