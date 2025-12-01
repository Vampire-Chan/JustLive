// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#pragma once

#include "CoreMinimal.h"
#include "ScriptBytecode.h"
#include "ScriptAST.h"  // For EScriptType enum

/**
 * VM Execution State
 */
enum class EVMState
{
    Ready,      // Initialized, ready to start
    Running,    // Currently executing
    Paused,     // execution suspended (e.g. Sleep)
    Finished,   // execution completed successfully
    Error       // execution failed
};

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

class FScriptVM;

/**
 * Native function signature
 * Takes VM context and array of arguments, returns a value
 */
typedef TFunction<FScriptValue(FScriptVM* VM, const TArray<FScriptValue>&)> FNativeFunction;

/**
 * Virtual Machine (VM) for Executing SBS/SBSH Bytecode
 * =====================================================
 * 
 * The VM is the FINAL STAGE of the compilation/execution pipeline.
 * It takes BYTECODE (compiled from the AST) and EXECUTES it.
 * 
 * COMPILATION PIPELINE OVERVIEW:
 * -----------------------------
 * 1. Source Code (.sc file)
 * 2. → LEXER → Tokens
 * 3. → PARSER → Abstract Syntax Tree (AST)
 * 4. → COMPILER → Bytecode Instructions
 * 5. → VM (THIS CLASS) → Execution & Results
 * 
 * WHAT THE VM DOES:
 * ----------------
 * Input:  Bytecode chunk (array of instructions)
 * Output: Program execution, side effects (logs, API calls), return values
 * 
 * The VM performs:
 * 1. Instruction-by-instruction execution
 * 2. Stack-based value management
 * 3. Function call management (call frames)
 * 4. Native function integration (API calls to Unreal)
 * 5. Memory and execution safety enforcement
 * 
 * STACK-BASED ARCHITECTURE:
 * -------------------------
 * The VM uses a STACK to manage values during execution.
 * 
 * Example execution of: x = 10 + 20;
 * 
 * Bytecode:           Stack State:        Description:
 * ----------------------------------------
 * PUSH 10            [10]                 Push constant 10
 * PUSH 20            [10, 20]             Push constant 20
 * ADD                [30]                 Pop 20 and 10, push sum 30
 * SET_LOCAL x        []                   Pop 30, store in variable x
 * 
 * INSTRUCTION SET:
 * ---------------
 * 
 * Constants & Literals:
 *   PUSH_CONSTANT <index>  - Push constant from constant pool
 *   PUSH_NIL               - Push nil/null value
 *   PUSH_TRUE              - Push boolean true
 *   PUSH_FALSE             - Push boolean false
 * 
 * Arithmetic Operations:
 *   ADD       - Pop b, pop a, push (a + b)
 *   SUBTRACT  - Pop b, pop a, push (a - b)
 *   MULTIPLY  - Pop b, pop a, push (a * b)
 *   DIVIDE    - Pop b, pop a, push (a / b)
 *   MODULO    - Pop b, pop a, push (a % b)
 *   NEGATE    - Pop a, push (-a)
 * 
 * Comparison Operations:
 *   EQUAL     - Pop b, pop a, push (a == b)
 *   GREATER   - Pop b, pop a, push (a > b)
 *   LESS      - Pop b, pop a, push (a < b)
 * 
 * Logical Operations:
 *   NOT       - Pop a, push (!a)
 *   AND       - Pop b, pop a, push (a && b)
 *   OR        - Pop b, pop a, push (a || b)
 * 
 * Bitwise Operations:
 *   BIT_AND   - Pop b, pop a, push (a & b)
 *   BIT_OR    - Pop b, pop a, push (a | b)
 *   BIT_XOR   - Pop b, pop a, push (a ^ b)
 *   BIT_NOT   - Pop a, push (~a)
 * 
 * Variables:
 *   GET_LOCAL <index>      - Push local variable value
 *   SET_LOCAL <index>      - Pop value, store in local variable
 *   DEFINE_GLOBAL <name>   - Pop value, create global variable
 *   GET_GLOBAL <name>      - Push global variable value
 *   SET_GLOBAL <name>      - Pop value, store in global variable
 * 
 * Control Flow:
 *   JUMP <offset>          - Unconditional jump forward/backward
 *   JUMP_IF_FALSE <offset> - Pop value, jump if false
 *   LOOP <offset>          - Jump backward (for loops)
 * 
 * Functions:
 *   CALL <arg_count>       - Call user-defined function
 *   CALL_NATIVE <name>     - Call native (C++) function
 *   RETURN                 - Return from function
 * 
 * Type Casting:
 *   CAST_INT              - Convert top of stack to int
 *   CAST_FLOAT            - Convert top of stack to float
 *   CAST_STRING           - Convert top of stack to string
 * 
 * Arrays:
 *   ARRAY_CREATE <size>   - Create array with size
 *   ARRAY_GET             - Pop index, pop array, push element
 *   ARRAY_SET             - Pop value, pop index, pop array, set element
 * 
 * CALL FRAMES & FUNCTION EXECUTION:
 * ---------------------------------
 * When a function is called, the VM creates a CALL FRAME containing:
 * - Function start address in bytecode
 * - Return address (where to resume after function completes)
 * - Stack base (for local variables)
 * - Function name (for debugging)
 * 
 * Example function call:
 * 
 *   int Add(int a, int b) {
 *       return a + b;
 *   }
 *   int result = Add(10, 20);
 * 
 * Execution steps:
 * 1. Push arguments: PUSH 10, PUSH 20
 * 2. CALL Add (creates call frame, jumps to Add function)
 * 3. Function body executes: GET_LOCAL a, GET_LOCAL b, ADD
 * 4. RETURN (pops call frame, pushes return value, resumes caller)
 * 5. Result is on stack for assignment to 'result'
 * 
 * NATIVE FUNCTION INTEGRATION:
 * ----------------------------
 * Native functions are C++ functions exposed to scripts.
 * They are registered with RegisterNativeFunction() and called via CALL_NATIVE.
 * 
 * Example - Registering Log function:
 * 
 *   VM->RegisterNativeFunction("Log", [](const TArray<FScriptValue>& Args) -> FScriptValue {
 *       if (Args.Num() > 0) {
 *           UE_LOG(LogTemp, Log, TEXT("%s"), *Args[0].ToString());
 *       }
 *       return FScriptValue(); // void return
 *   });
 * 
 * Script usage:
 *   Log("Hello from script!");
 * 
 * Bytecode:
 *   PUSH_CONSTANT "Hello from script!"
 *   CALL_NATIVE Log 1
 * 
 * EXECUTION SAFETY & LIMITS:
 * --------------------------
 * The VM enforces limits to prevent infinite loops and stack overflows:
 * - MaxInstructionsPerFrame: Maximum bytecode instructions per frame
 * - MaxStackDepth: Maximum stack size
 * - MaxCallDepth: Maximum function call recursion depth
 * - MaxExecutionTimeMs: Maximum execution time in milliseconds
 * 
 * These limits can be configured via SetExecutionLimits().
 * 
 * ERROR HANDLING:
 * --------------
 * Runtime errors are collected in an error list:
 * - Type mismatches (e.g., adding string + int)
 * - Division by zero
 * - Array out of bounds
 * - Stack overflow/underflow
 * - Undefined variables
 * - Call depth exceeded
 * 
 * Errors stop execution and can be retrieved via GetErrors().
 * 
 * MEMORY MANAGEMENT:
 * -----------------
 * - Stack: TArray<FScriptValue> - grows/shrinks as needed
 * - Globals: TMap<FString, FScriptValue> - persistent across calls
 * - Call Frames: TArray<FCallFrame> - tracks function call stack
 * - All memory is managed by Unreal's smart pointers and containers
 * 
 * Stack-based architecture with safety limits
 */
class SCRIPTING_API FScriptVM : public TSharedFromThis<FScriptVM>
{
public:
    FScriptVM();
    
    /**
     * Start execution of bytecode chunk
     * Returns true if execution started successfully
     */
    bool Execute(TSharedPtr<FBytecodeChunk> Bytecode);
    
    /**
     * Resume execution (called by LatentManager)
     * Returns true if execution completed or paused successfully
     * Returns false on error
     */
    bool Resume();

    /**
     * Pause execution (called by Native Functions like Sleep)
     * Execution will stop at the current instruction and return from Resume()
     */
    void Pause();

    /**
     * Get current VM state
     */
    EVMState GetState() const { return State; }
    
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
        int32 MaxInstructionsPerFrame = 100000000;  // 100M instructions - effectively unlimited for testing
        int32 MaxStackDepth = 10000;                // 10K stack depth - very generous
        int32 MaxCallDepth = 1000;                  // 1K call depth - allows deep recursion
        double MaxExecutionTimeMs = 60000.0;        // 60 seconds - effectively unlimited for testing
        
        FExecutionLimits() {}
    };
    
    void SetExecutionLimits(const FExecutionLimits& InLimits) { Limits = InLimits; }
    const FExecutionLimits& GetExecutionLimits() const { return Limits; }

    /**
     * Report a runtime error
     */
    void RuntimeError(const FString& Message);

private:
    // VM State
    EVMState State;

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
    void OpModulo();
    void OpNegate();
    
    void OpEqual();
    void OpGreater();
    void OpLess();
    void OpNot();
    void OpAnd();
    void OpOr();
    
    void OpBitAnd();
    void OpBitOr();
    void OpBitXor();
    void OpBitNot();
    
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

    //=============================================================================
    // Native Function Implementations
    //=============================================================================

    static FScriptValue NativePrint(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue NativeLogWarning(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue NativeLogError(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue NativeRandInt(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue NativeRandFloat(FScriptVM* VM, const TArray<FScriptValue>& Args);
};

