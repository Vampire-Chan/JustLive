// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#include "ScriptVM.h"
#include "ScriptLogger.h"
#include "Math/UnrealMathUtility.h" // For FMath::RandRange
#include "HAL/PlatformTime.h"

FScriptVM::FScriptVM()
    : State(EVMState::Ready)
    , InstructionPointer(0)
    , InstructionCount(0)
    , ExecutionStartTime(0.0)
{
    Stack.Reserve(256);
    CallFrames.Reserve(64);
}

bool FScriptVM::Execute(TSharedPtr<FBytecodeChunk> Bytecode)
{
    if (!Bytecode.IsValid() || Bytecode->Code.Num() == 0)
    {
        RuntimeError(TEXT("Invalid or empty bytecode"));
        return false;
    }
    
    // SECURITY: Validate bytecode before execution
    FString ValidationReason;
    if (!Bytecode->ValidateSecurity(ValidationReason))
    {
        RuntimeError(FString::Printf(TEXT("Bytecode security validation failed: %s"), *ValidationReason));
        UE_LOG(LogTemp, Error, TEXT("VM: SECURITY VIOLATION - %s"), *ValidationReason);
        UE_LOG(LogTemp, Error, TEXT("VM: Compiler: %s"), *Bytecode->Metadata.CompilerName);
        UE_LOG(LogTemp, Error, TEXT("VM: Source: %s"), *Bytecode->Metadata.SourceFileName);
        return false;
    }
    
    // Log security info
    VM_LOG(FString::Printf(TEXT("=== BYTECODE SECURITY ===")));
    VM_LOG(FString::Printf(TEXT("Compiler: %s %s"), *Bytecode->Metadata.CompilerName, *Bytecode->Metadata.CompilerVersion));
    VM_LOG(FString::Printf(TEXT("Game: %s %s"), *Bytecode->Metadata.GameName, *Bytecode->Metadata.GameVersion));
    VM_LOG(FString::Printf(TEXT("Trusted: %s"), Bytecode->IsTrustedCompiler() ? TEXT("YES") : TEXT("NO")));
    VM_LOG(FString::Printf(TEXT("Security: %s"), *ValidationReason));
    
    Reset();
    CurrentBytecode = Bytecode;
    InstructionPointer = 0;
    InstructionCount = 0;
    ExecutionStartTime = FPlatformTime::Seconds();
    State = EVMState::Ready;
    
    // Load function table from bytecode
    FunctionTable.Empty();
    for (const ::FFunctionInfo& BytecodeFunc : Bytecode->Functions)
    {
        FFunctionInfo VMFunc;
        VMFunc.Name = BytecodeFunc.Name;
        VMFunc.Address = BytecodeFunc.Address;
        VMFunc.Arity = BytecodeFunc.Arity;
        VMFunc.ReturnType = EScriptType::VOID; // Default for now
        FunctionTable.Add(VMFunc);
        
        VM_LOG(FString::Printf(TEXT("Loaded function: %s (address=%d, arity=%d)"),
            *VMFunc.Name, VMFunc.Address, VMFunc.Arity));
    }
    
    VM_LOG(TEXT("=== VM EXECUTION START ==="));
    VM_LOG(FString::Printf(TEXT("Loaded %d functions"), FunctionTable.Num()));
    
    return Resume();
}

bool FScriptVM::Resume()
{
    if (State == EVMState::Finished || State == EVMState::Error)
    {
        return false;
    }

    State = EVMState::Running;

    // Main execution loop
    while (InstructionPointer < CurrentBytecode->Code.Num() && State == EVMState::Running)
    {
        // Safety checks
        if (!CheckInstructionLimit() || !CheckTimeout())
        {
            State = EVMState::Error;
            return false;
        }
        
        // Execute one instruction
        if (!ExecuteInstruction())
        {
            VM_LOG_ERROR(TEXT("VM execution failed"));
            State = EVMState::Error;
            return false;
        }
        
        InstructionCount++;
    }
    
    if (State == EVMState::Paused)
    {
        VM_LOG(TEXT("VM Paused (Latent Action)"));
        return true;
    }

    State = EVMState::Finished;
    
    double ExecutionTime = (FPlatformTime::Seconds() - ExecutionStartTime) * 1000.0;
    VM_LOG(FString::Printf(TEXT("=== VM EXECUTION COMPLETE ===\nExecuted %d instructions in %.2fms"),
        InstructionCount, ExecutionTime));
    
    return true;
}

void FScriptVM::Pause()
{
    State = EVMState::Paused;
}

void FScriptVM::RegisterNativeFunction(const FString& Name, FNativeFunction Function)
{
    NativeFunctions.Add(Name, Function);
    VM_LOG(FString::Printf(TEXT("Registered native function: %s"), *Name));
}

void FScriptVM::Reset()
{
    Stack.Empty();
    CallFrames.Empty();
    FunctionTable.Empty();
    Errors.Empty();
    InstructionPointer = 0;
    InstructionCount = 0;
    ExecutionStartTime = 0.0;
}

//=============================================================================
// Stack Operations
//=============================================================================

void FScriptVM::Push(const FScriptValue& Value)
{
    if (!CheckStackOverflow())
    {
        return;
    }
    Stack.Add(Value);
}

FScriptValue FScriptVM::Pop()
{
    if (Stack.Num() == 0)
    {
        RuntimeError(TEXT("Stack underflow"));
        return FScriptValue::Nil();
    }
    
    FScriptValue Value = Stack.Last();
    Stack.Pop();
    return Value;
}

FScriptValue FScriptVM::Peek(int32 Offset) const
{
    if (Stack.Num() == 0 || Offset >= Stack.Num())
    {
        return FScriptValue::Nil();
    }
    return Stack[Stack.Num() - 1 - Offset];
}

//=============================================================================
// Error Handling
//=============================================================================

bool FScriptVM::CallMainIfExists()
{
    // Look for Main function in function table
    int32 MainFuncIndex = -1;
    for (int32 i = 0; i < FunctionTable.Num(); ++i)
    {
        if (FunctionTable[i].Name == TEXT("Main"))
        {
            MainFuncIndex = i;
            break;
        }
    }
    
    if (MainFuncIndex == -1)
    {
        // No Main function found, this is not an error
        VM_LOG(TEXT("No Main() function found - script completed"));
        return false;
    }
    
    FFunctionInfo& MainFunc = FunctionTable[MainFuncIndex];
    
    // Create a call to Main function
    // Push arguments (none for Main)
    // Call the function
    
    // Verify call depth limit
    if (!CheckCallDepth())
    {
        return false;
    }
    
    // Create new call frame for Main
    FCallFrame Frame;
    Frame.FunctionAddress = MainFunc.Address;
    Frame.ReturnAddress = CurrentBytecode->Code.Num();  // Return to end of bytecode
    Frame.StackBase = Stack.Num();  // No arguments for Main()
    Frame.FunctionName = TEXT("Main");
    
    CallFrames.Add(Frame);
    
    // Jump to Main function
    InstructionPointer = MainFunc.Address;
    
    VM_LOG(TEXT("Calling Main() function..."));
    State = EVMState::Running;
    
    // Now execute until we return from Main
    while (InstructionPointer < CurrentBytecode->Code.Num() && CallFrames.Num() > 0 && State == EVMState::Running)
    {
        // Safety checks
        if (!CheckInstructionLimit() || !CheckTimeout())
        {
            State = EVMState::Error;
            return false;
        }
        
        // Execute one instruction
        if (!ExecuteInstruction())
        {
            VM_LOG_ERROR(TEXT("VM execution failed in Main()"));
            State = EVMState::Error;
            return false;
        }
        
        InstructionCount++;
    }
    
    if (State == EVMState::Paused)
    {
        VM_LOG(TEXT("Main() Paused (Latent Action)"));
        return true;
    }
    
    // Check if we have a return value
    if (Stack.Num() > 0)
    {
        FScriptValue ReturnValue = Pop();
        VM_LOG(FString::Printf(TEXT("Main() returned: %s"), *ReturnValue.ToString()));
    }
    
    State = EVMState::Finished;
    VM_LOG(TEXT("Main() function completed"));
    return true;
}

void FScriptVM::RuntimeError(const FString& Message)
{
    Errors.Add(Message);
    VM_LOG_ERROR(FString::Printf(TEXT("Runtime Error: %s"), *Message));
    VM_LOG_ERROR(FString::Printf(TEXT("  At instruction %d"), InstructionPointer));
    
    // Dump stack for debugging
    if (Stack.Num() > 0)
    {
        VM_LOG_ERROR(TEXT("  Stack trace:"));
        for (int32 i = Stack.Num() - 1; i >= 0 && i >= Stack.Num() - 5; --i)
        {
            VM_LOG_ERROR(FString::Printf(TEXT("    [%d] %s"), i, *Stack[i].ToString()));
        }
    }
}

bool FScriptVM::CheckStackOverflow()
{
    if (Stack.Num() >= Limits.MaxStackDepth)
    {
        RuntimeError(FString::Printf(TEXT("Stack overflow (max depth: %d)"), Limits.MaxStackDepth));
        return false;
    }
    return true;
}

bool FScriptVM::CheckCallDepth()
{
    if (CallFrames.Num() >= Limits.MaxCallDepth)
    {
        RuntimeError(FString::Printf(TEXT("Call stack overflow (max depth: %d)"), Limits.MaxCallDepth));
        return false;
    }
    return true;
}

bool FScriptVM::CheckInstructionLimit()
{
    if (InstructionCount >= Limits.MaxInstructionsPerFrame)
    {
        RuntimeError(FString::Printf(TEXT("Instruction limit exceeded (max: %d)"), Limits.MaxInstructionsPerFrame));
        return false;
    }
    return true;
}

bool FScriptVM::CheckTimeout()
{
    double ElapsedMs = (FPlatformTime::Seconds() - ExecutionStartTime) * 1000.0;
    if (ElapsedMs > Limits.MaxExecutionTimeMs)
    {
        RuntimeError(FString::Printf(TEXT("Execution timeout (max: %.2fms, actual: %.2fms)"), 
            Limits.MaxExecutionTimeMs, ElapsedMs));
        return false;
    }
    return true;
}

//=============================================================================
// Instruction Execution
//=============================================================================

bool FScriptVM::ExecuteInstruction()
{
    if (InstructionPointer >= CurrentBytecode->Code.Num())
    {
        RuntimeError(TEXT("Instruction pointer out of bounds"));
        return false;
    }
    
    EOpCode OpCode = static_cast<EOpCode>(CurrentBytecode->Code[InstructionPointer++]);
    
    switch (OpCode)
    {
        case EOpCode::OP_CONSTANT:      OpConstant(); break;
        case EOpCode::OP_NIL:           OpNil(); break;
        case EOpCode::OP_TRUE:          OpTrue(); break;
        case EOpCode::OP_FALSE:         OpFalse(); break;
        
        case EOpCode::OP_ADD:           OpAdd(); break;
        case EOpCode::OP_SUBTRACT:      OpSubtract(); break;
        case EOpCode::OP_MULTIPLY:      OpMultiply(); break;
        case EOpCode::OP_DIVIDE:        OpDivide(); break;
        case EOpCode::OP_MODULO:        OpModulo(); break;
        case EOpCode::OP_NEGATE:        OpNegate(); break;
        
        case EOpCode::OP_EQUAL:         OpEqual(); break;
        case EOpCode::OP_GREATER:       OpGreater(); break;
        case EOpCode::OP_LESS:          OpLess(); break;
        case EOpCode::OP_NOT:           OpNot(); break;
        case EOpCode::OP_AND:           OpAnd(); break;
        case EOpCode::OP_OR:            OpOr(); break;
        
        case EOpCode::OP_BIT_AND:       OpBitAnd(); break;
        case EOpCode::OP_BIT_OR:        OpBitOr(); break;
        case EOpCode::OP_BIT_XOR:       OpBitXor(); break;
        case EOpCode::OP_BIT_NOT:       OpBitNot(); break;
        
        case EOpCode::OP_GET_LOCAL:     OpGetLocal(); break;
        case EOpCode::OP_SET_LOCAL:     OpSetLocal(); break;
        case EOpCode::OP_DEFINE_GLOBAL: OpDefineGlobal(); break;
        case EOpCode::OP_GET_GLOBAL:    OpGetGlobal(); break;
        case EOpCode::OP_SET_GLOBAL:    OpSetGlobal(); break;
        
        case EOpCode::OP_JUMP:          OpJump(); break;
        case EOpCode::OP_JUMP_IF_FALSE: OpJumpIfFalse(); break;
        case EOpCode::OP_LOOP:          OpLoop(); break;
        
        case EOpCode::OP_CALL:          OpCall(); break;
        case EOpCode::OP_CALL_NATIVE:   OpCallNative(); break;
        case EOpCode::OP_RETURN:        OpReturn(); break;
        
        case EOpCode::OP_CAST_INT:      OpCastInt(); break;
        case EOpCode::OP_CAST_FLOAT:    OpCastFloat(); break;
        case EOpCode::OP_CAST_STRING:   OpCastString(); break;
        
        case EOpCode::OP_POP:           OpPop(); break;
        case EOpCode::OP_PRINT:         OpPrint(); break;
        
        // Missing opcodes
        case EOpCode::OP_NOT_EQUAL:     OpNotEqual(); break;
        case EOpCode::OP_GREATER_EQUAL: OpGreaterEqual(); break;
        case EOpCode::OP_LESS_EQUAL:    OpLessEqual(); break;
        case EOpCode::OP_CREATE_ARRAY:  OpCreateArray(); break;
        case EOpCode::OP_GET_ELEMENT:   OpGetElement(); break;
        case EOpCode::OP_SET_ELEMENT:   OpSetElement(); break;
        case EOpCode::OP_DUPLICATE:     OpDuplicate(); break;
        
        // Field access opcodes
        case EOpCode::OP_GET_FIELD:     OpGetField(); break;
        case EOpCode::OP_SET_FIELD:     OpSetField(); break;
        
        case EOpCode::OP_HALT:
            VM_LOG(TEXT("VM halted (normal completion)"));
            return true; // HALT is a normal exit, not an error
        
        default:
            RuntimeError(FString::Printf(TEXT("Unknown opcode: %d"), static_cast<int32>(OpCode)));
            return false;
    }
    
    return !HasErrors();
}

//=============================================================================
// Opcode Implementations
//=============================================================================

void FScriptVM::OpConstant()
{
    Push(ReadConstant());
}

void FScriptVM::OpNil()
{
    Push(FScriptValue::Nil());
}

void FScriptVM::OpTrue()
{
    Push(FScriptValue::Bool(true));
}

void FScriptVM::OpFalse()
{
    Push(FScriptValue::Bool(false));
}

void FScriptVM::OpAdd()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    
    if (A.IsNumber() && B.IsNumber())
    {
        Push(FScriptValue::Number(A.AsNumber() + B.AsNumber()));
    }
    else if (A.IsString() || B.IsString())
    {
        // String concatenation
        Push(FScriptValue::String(A.ToString() + B.ToString()));
    }
    else
    {
        RuntimeError(TEXT("Operands must be numbers or strings"));
    }
}

void FScriptVM::OpSubtract()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    
    if (!A.IsNumber() || !B.IsNumber())
    {
        RuntimeError(TEXT("Operands must be numbers"));
        return;
    }
    
    Push(FScriptValue::Number(A.AsNumber() - B.AsNumber()));
}

void FScriptVM::OpMultiply()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    
    if (!A.IsNumber() || !B.IsNumber())
    {
        RuntimeError(TEXT("Operands must be numbers"));
        return;
    }
    
    Push(FScriptValue::Number(A.AsNumber() * B.AsNumber()));
}

void FScriptVM::OpDivide()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    
    if (!A.IsNumber() || !B.IsNumber())
    {
        RuntimeError(TEXT("Operands must be numbers"));
        return;
    }
    
    if (B.AsNumber() == 0.0)
    {
        RuntimeError(TEXT("Division by zero"));
        return;
    }
    
    double AVal = A.AsNumber();
    double BVal = B.AsNumber();
    
    // C-style integer division: if both operands are whole numbers, truncate result
    bool AIsInt = FMath::IsNearlyEqual(AVal, FMath::RoundToDouble(AVal));
    bool BIsInt = FMath::IsNearlyEqual(BVal, FMath::RoundToDouble(BVal));
    
    if (AIsInt && BIsInt)
    {
        // Integer division - truncate towards zero (C behavior)
        int64 IntA = static_cast<int64>(AVal);
        int64 IntB = static_cast<int64>(BVal);
        Push(FScriptValue::Number(static_cast<double>(IntA / IntB)));
    }
    else
    {
        // Float division
        Push(FScriptValue::Number(AVal / BVal));
    }
}

void FScriptVM::OpModulo()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    
    if (!A.IsNumber() || !B.IsNumber())
    {
        RuntimeError(TEXT("Operands must be numbers"));
        return;
    }
    
    if (B.AsNumber() == 0.0)
    {
        RuntimeError(TEXT("Modulo by zero"));
        return;
    }
    
    // Use FMath::Fmod for floating point modulo
    Push(FScriptValue::Number(FMath::Fmod(A.AsNumber(), B.AsNumber())));
}

void FScriptVM::OpNegate()
{
    FScriptValue Value = Pop();
    
    if (!Value.IsNumber())
    {
        RuntimeError(TEXT("Operand must be a number"));
        return;
    }
    
    Push(FScriptValue::Number(-Value.AsNumber()));
}

void FScriptVM::OpEqual()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    Push(FScriptValue::Bool(AreEqual(A, B)));
}

void FScriptVM::OpGreater()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    
    if (!A.IsNumber() || !B.IsNumber())
    {
        RuntimeError(TEXT("Operands must be numbers"));
        return;
    }
    
    Push(FScriptValue::Bool(A.AsNumber() > B.AsNumber()));
}

void FScriptVM::OpLess()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    
    if (!A.IsNumber() || !B.IsNumber())
    {
        RuntimeError(TEXT("Operands must be numbers"));
        return;
    }
    
    Push(FScriptValue::Bool(A.AsNumber() < B.AsNumber()));
}

void FScriptVM::OpNot()
{
    FScriptValue Value = Pop();
    Push(FScriptValue::Bool(!IsTruthy(Value)));
}

void FScriptVM::OpAnd()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    Push(FScriptValue::Bool(IsTruthy(A) && IsTruthy(B)));
}

void FScriptVM::OpOr()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    Push(FScriptValue::Bool(IsTruthy(A) || IsTruthy(B)));
}

void FScriptVM::OpBitAnd()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    
    if (!A.IsNumber() || !B.IsNumber())
    {
        RuntimeError(TEXT("Bitwise AND operands must be numbers"));
        return;
    }
    
    int32 IntA = static_cast<int32>(A.AsNumber());
    int32 IntB = static_cast<int32>(B.AsNumber());
    Push(FScriptValue::Number(static_cast<double>(IntA & IntB)));
}

void FScriptVM::OpBitOr()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    
    if (!A.IsNumber() || !B.IsNumber())
    {
        RuntimeError(TEXT("Bitwise OR operands must be numbers"));
        return;
    }
    
    int32 IntA = static_cast<int32>(A.AsNumber());
    int32 IntB = static_cast<int32>(B.AsNumber());
    Push(FScriptValue::Number(static_cast<double>(IntA | IntB)));
}

void FScriptVM::OpBitXor()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    
    if (!A.IsNumber() || !B.IsNumber())
    {
        RuntimeError(TEXT("Bitwise XOR operands must be numbers"));
        return;
    }
    
    int32 IntA = static_cast<int32>(A.AsNumber());
    int32 IntB = static_cast<int32>(B.AsNumber());
    Push(FScriptValue::Number(static_cast<double>(IntA ^ IntB)));
}

void FScriptVM::OpBitNot()
{
    FScriptValue Value = Pop();
    
    if (!Value.IsNumber())
    {
        RuntimeError(TEXT("Bitwise NOT operand must be a number"));
        return;
    }
    
    int32 IntValue = static_cast<int32>(Value.AsNumber());
    Push(FScriptValue::Number(static_cast<double>(~IntValue)));
}

void FScriptVM::OpGetLocal()
{
    uint8 Slot = ReadByte();
    
    int32 StackIndex = CallFrames.Num() > 0 ? CallFrames.Last().StackBase + Slot : Slot;
    
    if (StackIndex >= Stack.Num())
    {
        RuntimeError(FString::Printf(TEXT("Invalid local variable slot: %d"), Slot));
        return;
    }
    
    // Copy value BEFORE Push to avoid reallocation invalidating the reference
    FScriptValue Value = Stack[StackIndex];
    Push(Value);
}

void FScriptVM::OpSetLocal()
{
    uint8 Slot = ReadByte();
    
    int32 StackIndex = CallFrames.Num() > 0 ? CallFrames.Last().StackBase + Slot : Slot;
    
    if (StackIndex >= Stack.Num())
    {
        RuntimeError(FString::Printf(TEXT("Invalid local variable slot: %d"), Slot));
        return;
    }
    
    // Copy value from stack top BEFORE any array modification
    FScriptValue Value = Peek(0);
    Stack[StackIndex] = Value; // Don't pop - assignment is an expression
}

void FScriptVM::OpDefineGlobal()
{
    // Read global variable name from constant pool
    FScriptValue NameValue = ReadConstant();
    if (!NameValue.IsString())
    {
        RuntimeError(TEXT("Global variable name must be a string"));
        return;
    }
    
    FString VarName = NameValue.AsString();
    FScriptValue Value = Pop(); // Get initialization value from stack
    
    // Store in globals table
    Globals.Add(VarName, Value);
    
    VM_LOG(FString::Printf(TEXT("Defined global variable: %s = %s"), *VarName, *Value.ToString()));
}

void FScriptVM::OpGetGlobal()
{
    // Read global variable name from constant pool
    FScriptValue NameValue = ReadConstant();
    if (!NameValue.IsString())
    {
        RuntimeError(TEXT("Global variable name must be a string"));
        return;
    }
    
    FString VarName = NameValue.AsString();
    
    // Lookup in globals table
    FScriptValue* ValuePtr = Globals.Find(VarName);
    if (ValuePtr)
    {
        Push(*ValuePtr);
    }
    else
    {
        RuntimeError(FString::Printf(TEXT("Undefined global variable: %s"), *VarName));
        Push(FScriptValue::Nil());
    }
}

void FScriptVM::OpSetGlobal()
{
    // Read global variable name from constant pool
    FScriptValue NameValue = ReadConstant();
    if (!NameValue.IsString())
    {
        RuntimeError(TEXT("Global variable name must be a string"));
        return;
    }
    
    FString VarName = NameValue.AsString();
    
    // Check if variable exists
    if (!Globals.Contains(VarName))
    {
        RuntimeError(FString::Printf(TEXT("Cannot assign to undefined global variable: %s"), *VarName));
        return;
    }
    
    // Set value (peek, don't pop - assignment is an expression)
    FScriptValue Value = Peek(0);
    Globals[VarName] = Value;
    
    VM_LOG(FString::Printf(TEXT("Set global variable: %s = %s"), *VarName, *Value.ToString()));
}

void FScriptVM::OpJump()
{
    uint16 Offset = ReadShort();
    InstructionPointer += Offset;
}

void FScriptVM::OpJumpIfFalse()
{
    uint16 Offset = ReadShort();
    if (!IsTruthy(Peek(0)))
    {
        InstructionPointer += Offset;
    }
}

void FScriptVM::OpLoop()
{
    uint16 Offset = ReadShort();
    InstructionPointer -= Offset;
}

void FScriptVM::OpCall()
{
    uint8 ArgCount = ReadByte();
    uint16 FuncIndex = ReadShort();
    
    // Validate function index
    if (!FunctionTable.IsValidIndex(FuncIndex))
    {
        RuntimeError(FString::Printf(TEXT("Invalid function index: %d"), FuncIndex));
        // Pop arguments to clean up stack
        for (int32 i = 0; i < ArgCount; ++i)
        {
            Pop();
        }
        Push(FScriptValue::Nil());
        return;
    }
    
    FFunctionInfo& FuncInfo = FunctionTable[FuncIndex];
    
    // Check argument count matches function arity
    if (ArgCount != FuncInfo.Arity)
    {
        RuntimeError(FString::Printf(TEXT("Argument count mismatch for function '%s': expected %d, got %d"), 
            *FuncInfo.Name, FuncInfo.Arity, ArgCount));
        // Pop arguments to clean up stack
        for (int32 i = 0; i < ArgCount; ++i)
        {
            Pop();
        }
        Push(FScriptValue::Nil());
        return;
    }
    
    // Set up call frame - the arguments are already on the stack
    // Arguments are on stack in reverse order (last arg on top), so we need to rearrange them
    // Current stack layout: [top] argN, argN-1, ..., arg2, arg1, [bottom]
    
    // Verify call depth limit
    if (!CheckCallDepth())
    {
        // Pop arguments to clean up stack
        for (int32 i = 0; i < ArgCount; ++i)
        {
            Pop();
        }
        Push(FScriptValue::Nil());
        return;
    }
    
    // Create new call frame
    FCallFrame Frame;
    Frame.FunctionAddress = FuncInfo.Address;
    Frame.ReturnAddress = InstructionPointer;  // Return to instruction after call
    Frame.StackBase = Stack.Num() - ArgCount;  // Arguments start at this index
    Frame.FunctionName = FuncInfo.Name;
    
    CallFrames.Add(Frame);
    
    // Jump to function
    InstructionPointer = FuncInfo.Address;
}

void FScriptVM::OpCallNative()
{
    uint8 ArgCount = ReadByte();
    uint16 NameIndex = ReadShort();
    
    if (NameIndex >= CurrentBytecode->Constants.Num())
    {
        RuntimeError(TEXT("Invalid native function name index"));
        return;
    }
    
    FString FuncName = CurrentBytecode->Constants[NameIndex].AsString();
    
    // Pop arguments
    TArray<FScriptValue> Args;
    Args.Reserve(ArgCount);
    for (int32 i = 0; i < ArgCount; ++i)
    {
        Args.Insert(Pop(), 0); // Insert at front to preserve order
    }
    
    // Call native function
    FNativeFunction* NativeFunc = NativeFunctions.Find(FuncName);
    if (NativeFunc)
    {
        // Pass 'this' (VM pointer) to the native function
        FScriptValue Result = (*NativeFunc)(this, Args);
        
        // Only push result if we are NOT paused
        // If paused, the native function might not have returned a meaningful value yet
        // But actually, natives always return a value. If sleeping, they return Nil.
        // So we push the result (Nil). When we resume, we are at the next instruction.
        // The native function is responsible for calling VM->Pause() if needed.
        Push(Result);
    }
    else
    {
        VM_LOG_WARNING(FString::Printf(TEXT("Native function '%s' not found - pushing nil"), *FuncName));
        Push(FScriptValue::Nil());
    }
}

void FScriptVM::OpReturn()
{
    // At this point, stack has: [Frame.StackBase: args...] [locals...] [return value]
    FScriptValue Result = Pop();
    
    if (CallFrames.Num() > 0)
    {
        // Return from function
        FCallFrame Frame = CallFrames.Last();
        CallFrames.Pop();
        
        VM_LOG(FString::Printf(TEXT("OpReturn: Frame.StackBase=%d, Stack.Num()=%d, Result=%s"),
            Frame.StackBase, Stack.Num(), *Result.ToString()));
        
        // Calculate how many values to remove (everything above Frame.StackBase)
        // This includes all arguments and local variables created during function execution
        int32 ValuesToPop = Stack.Num() - Frame.StackBase;
        
        VM_LOG(FString::Printf(TEXT("OpReturn: Popping %d values from stack"), ValuesToPop));
        
        // Pop all values from this frame (arguments and locals)
        for (int32 i = 0; i < ValuesToPop; ++i)
        {
            if (Stack.Num() > Frame.StackBase)
            {
                FScriptValue Popped = Stack.Pop();
                VM_LOG(FString::Printf(TEXT("OpReturn: Popped value %d: %s"), i, *Popped.ToString()));
            }
        }
        
        VM_LOG(FString::Printf(TEXT("OpReturn: Pushing result %s"), *Result.ToString()));
        
        // Push the return value where the arguments were
        Push(Result);
        
        // Restore instruction pointer to after the CALL instruction
        InstructionPointer = Frame.ReturnAddress;
    }
    else
    {
        // Top-level return - halt execution
        Push(Result);
        InstructionPointer = CurrentBytecode->Code.Num();
    }
}

void FScriptVM::OpCastInt()
{
    FScriptValue Value = Pop();
    
    if (Value.IsNumber())
    {
        Push(FScriptValue::Number(static_cast<int32>(Value.AsNumber())));
    }
    else if (Value.IsString())
    {
        int32 IntValue = FCString::Atoi(*Value.AsString());
        Push(FScriptValue::Number(static_cast<double>(IntValue)));
    }
    else
    {
        RuntimeError(TEXT("Cannot cast to int"));
    }
}

void FScriptVM::OpCastFloat()
{
    FScriptValue Value = Pop();
    
    if (Value.IsNumber())
    {
        Push(Value); // Already a number
    }
    else if (Value.IsString())
    {
        double FloatValue = FCString::Atod(*Value.AsString());
        Push(FScriptValue::Number(FloatValue));
    }
    else
    {
        RuntimeError(TEXT("Cannot cast to float"));
    }
}

void FScriptVM::OpCastString()
{
    FScriptValue Value = Pop();
    Push(FScriptValue::String(Value.ToString()));
}

void FScriptVM::OpPop()
{
    Pop();
}

void FScriptVM::OpPrint()
{
    FScriptValue Value = Pop();
    VM_LOG(FString::Printf(TEXT("[PRINT] %s"), *Value.ToString()));
}

void FScriptVM::OpNotEqual()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    Push(FScriptValue::Bool(!AreEqual(A, B)));
}

void FScriptVM::OpGreaterEqual()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    
    if (!A.IsNumber() || !B.IsNumber())
    {
        RuntimeError(TEXT("Operands must be numbers"));
        return;
    }
    
    Push(FScriptValue::Bool(A.AsNumber() >= B.AsNumber()));
}

void FScriptVM::OpLessEqual()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    
    if (!A.IsNumber() || !B.IsNumber())
    {
        RuntimeError(TEXT("Operands must be numbers"));
        return;
    }
    
    Push(FScriptValue::Bool(A.AsNumber() <= B.AsNumber()));
}

void FScriptVM::OpCreateArray()
{
    // This opcode should be followed by a byte indicating the number of elements to create the array from
    uint8 ElementCount = ReadByte();
    
    TArray<FScriptValue> Elements;
    Elements.Reserve(ElementCount);
    
    // Pop elements in reverse order (they were pushed in order)
    for (int32 i = 0; i < ElementCount; ++i)
    {
        Elements.Insert(Pop(), 0);
    }
    
    Push(FScriptValue::Array(Elements));
}

void FScriptVM::OpGetElement()
{
    // Index is on top of stack, followed by the array
    FScriptValue Index = Pop();
    FScriptValue Array = Pop();
    
    if (!Array.IsArray())
    {
        RuntimeError(TEXT("Subscript operator requires array"));
        Push(FScriptValue::Nil()); // Push a default value
        return;
    }
    
    if (!Index.IsNumber())
    {
        RuntimeError(TEXT("Array index must be a number"));
        Push(FScriptValue::Nil()); // Push a default value
        return;
    }
    
    int32 Idx = static_cast<int32>(Index.AsNumber());
    const TArray<FScriptValue>& ArrayElements = Array.AsArray();
    
    if (Idx < 0 || Idx >= ArrayElements.Num())
    {
        RuntimeError(TEXT("Array index out of bounds"));
        Push(FScriptValue::Nil()); // Push a default value
        return;
    }
    
    Push(ArrayElements[Idx]);
}

void FScriptVM::OpSetElement()
{
    // Value to set is on top, followed by index, then array
    FScriptValue Value = Pop();      // Value to set
    FScriptValue Index = Pop();      // Index
    FScriptValue Array = Pop();      // Array
    
    if (!Array.IsArray())
    {
        RuntimeError(TEXT("Subscript assignment requires array"));
        return;
    }
    
    if (!Index.IsNumber())
    {
        RuntimeError(TEXT("Array index must be a number"));
        return;
    }
    
    int32 Idx = static_cast<int32>(Index.AsNumber());
    TArray<FScriptValue> ArrayElements = Array.AsArray(); // Copy the array
    
    if (Idx < 0 || Idx >= ArrayElements.Num())
    {
        RuntimeError(TEXT("Array index out of bounds"));
        return;
    }
    
    // Modify the copied array
    ArrayElements[Idx] = Value;
    
    // Push back the modified array
    Push(FScriptValue::Array(ArrayElements));
}

void FScriptVM::OpDuplicate()
{
    // Duplicate the top value on the stack
    if (Stack.Num() == 0)
    {
        RuntimeError(TEXT("Stack underflow - cannot duplicate"));
        return;
    }
    
    FScriptValue Value = Stack.Last();
    Push(Value);
}

void FScriptVM::OpGetField()
{
    // Field name is encoded as a 16-bit constant index in the bytecode
    uint16 NameIndex = ReadShort();
    
    if (NameIndex >= CurrentBytecode->Constants.Num())
    {
        RuntimeError(FString::Printf(TEXT("Invalid field name index: %d"), NameIndex));
        return;
    }
    
    FString FieldName = CurrentBytecode->Constants[NameIndex].AsString();
    FScriptValue Object = Pop();
    
    // Handle array properties
    if (Object.IsArray())
    {
        if (FieldName == TEXT("length"))
        {
            Push(FScriptValue::Number(static_cast<double>(Object.AsArray().Num())));
            return;
        }
        // Could add more array properties here in the future
    }
    
    // For now, we'll return nil for any other field access
    // In a full implementation, we would handle struct/object properties here
    VM_LOG_WARNING(FString::Printf(TEXT("Object field '%s' not found, returning nil"), *FieldName));
    Push(FScriptValue::Nil());
}

void FScriptVM::OpSetField()
{
    // Field name is encoded as a 16-bit constant index in the bytecode
    uint16 NameIndex = ReadShort();
    
    if (NameIndex >= CurrentBytecode->Constants.Num())
    {
        RuntimeError(FString::Printf(TEXT("Invalid field name index: %d"), NameIndex));
        return;
    }
    
    FString FieldName = CurrentBytecode->Constants[NameIndex].AsString();
    FScriptValue Value = Pop();  // The value to assign
    FScriptValue Object = Pop(); // The object to modify
    
    // For now, make fields read-only by default
    // In a full implementation, we would handle struct/object field assignment
    VM_LOG_WARNING(FString::Printf(TEXT("Setting object field '%s' not implemented"), *FieldName));
    // Push the object back
    Push(Object);
}

//=============================================================================
// Helper Methods
//=============================================================================

uint8 FScriptVM::ReadByte()
{
    if (InstructionPointer >= CurrentBytecode->Code.Num())
    {
        RuntimeError(TEXT("Unexpected end of bytecode"));
        return 0;
    }
    return CurrentBytecode->Code[InstructionPointer++];
}

uint16 FScriptVM::ReadShort()
{
    if (InstructionPointer + 1 >= CurrentBytecode->Code.Num())
    {
        RuntimeError(TEXT("Unexpected end of bytecode"));
        return 0;
    }
    uint8 High = CurrentBytecode->Code[InstructionPointer++];
    uint8 Low = CurrentBytecode->Code[InstructionPointer++];
    return (High << 8) | Low;
}

FScriptValue FScriptVM::ReadConstant()
{
    uint8 Index = ReadByte();
    if (Index >= CurrentBytecode->Constants.Num())
    {
        RuntimeError(FString::Printf(TEXT("Invalid constant index: %d"), Index));
        return FScriptValue::Nil();
    }
    return CurrentBytecode->Constants[Index];
}

bool FScriptVM::IsTruthy(const FScriptValue& Value) const
{
    return Value.IsTruthy();
}

bool FScriptVM::AreEqual(const FScriptValue& A, const FScriptValue& B) const
{
    if (A.Type != B.Type)
    {
        return false;
    }
    
    switch (A.Type)
    {
        case EValueType::NIL:
            return true;
        case EValueType::BOOL:
            return A.BoolValue == B.BoolValue;
        case EValueType::NUMBER:
            return FMath::IsNearlyEqual(A.NumberValue, B.NumberValue, 0.0001);
        case EValueType::STRING:
            return A.StringValue.Equals(B.StringValue);
        case EValueType::ARRAY:
        {
            const TArray<FScriptValue>& ArrayA = A.AsArray();
            const TArray<FScriptValue>& ArrayB = B.AsArray();
            
            if (ArrayA.Num() != ArrayB.Num())
            {
                return false;
            }
            
            for (int32 i = 0; i < ArrayA.Num(); ++i)
            {
                if (!AreEqual(ArrayA[i], ArrayB[i]))
                {
                    return false;
                }
            }
            return true;
        }
        default:
            return false;
    }
}

void FScriptVM::DumpStack() const
{
    VM_LOG(TEXT("=== Stack Dump ==="));
    for (int32 i = 0; i < Stack.Num(); ++i)
    {
        VM_LOG(FString::Printf(TEXT("  [%d] %s"), i, *Stack[i].ToString()));
    }
}

//=============================================================================
// Native Function Implementations
//=============================================================================

FScriptValue FScriptVM::NativePrint(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() != 1)
    {
        VM->RuntimeError(TEXT("Print expects 1 argument."));
        return FScriptValue::Nil();
    }
    VM_LOG(FString::Printf(TEXT("[SCRIPT PRINT] %s"), *Args[0].ToString()));
    return FScriptValue::Nil();
}

FScriptValue FScriptVM::NativeLogWarning(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() != 1)
    {
        VM->RuntimeError(TEXT("LogWarning expects 1 argument."));
        return FScriptValue::Nil();
    }
    VM_LOG_WARNING(FString::Printf(TEXT("[SCRIPT WARNING] %s"), *Args[0].ToString()));
    return FScriptValue::Nil();
}

FScriptValue FScriptVM::NativeLogError(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() != 1)
    {
        VM->RuntimeError(TEXT("LogError expects 1 argument."));
        return FScriptValue::Nil();
    }
    VM_LOG_ERROR(FString::Printf(TEXT("[SCRIPT ERROR] %s"), *Args[0].ToString()));
    return FScriptValue::Nil();
}

FScriptValue FScriptVM::NativeRandInt(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() != 2 || !Args[0].IsNumber() || !Args[1].IsNumber())
    {
        VM->RuntimeError(TEXT("RandInt expects 2 number arguments (min, max)."));
        return FScriptValue::Nil();
    }
    int32 Min = static_cast<int32>(Args[0].AsNumber());
    int32 Max = static_cast<int32>(Args[1].AsNumber());
    return FScriptValue::Number(FMath::RandRange(Min, Max));
}

FScriptValue FScriptVM::NativeRandFloat(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() != 2 || !Args[0].IsNumber() || !Args[1].IsNumber())
    {
        VM->RuntimeError(TEXT("RandFloat expects 2 number arguments (min, max)."));
        return FScriptValue::Nil();
    }
    float Min = static_cast<float>(Args[0].AsNumber());
    float Max = static_cast<float>(Args[1].AsNumber());
    return FScriptValue::Number(FMath::FRandRange(Min, Max));
}

