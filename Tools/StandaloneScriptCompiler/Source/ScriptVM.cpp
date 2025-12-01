// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#include "ScriptVM.h"
// Removed UE dependency
// Platform time not needed

FScriptVM::FScriptVM()
    : InstructionPointer(0)
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
        RuntimeError("Invalid or empty bytecode");
        return false;
    }
    
    Reset();
    CurrentBytecode = Bytecode;
    InstructionPointer = 0;
    InstructionCount = 0;
    ExecutionStartTime = FPlatformTime::Seconds();
    
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
        
        SCRIPT_LOG(FString_Utils::Printf("Loaded function: %s (address=%d, arity=%d)",
            *VMFunc.Name, VMFunc.Address, VMFunc.Arity));
    }
    
    SCRIPT_LOG("=== VM EXECUTION START ===");
    SCRIPT_LOG(FString_Utils::Printf("Loaded %d functions", FunctionTable.Num()));
    
    // Main execution loop
    while (InstructionPointer < CurrentBytecode->Code.Num())
    {
        // Safety checks
        if (!CheckInstructionLimit() || !CheckTimeout())
        {
            return false;
        }
        
        // Execute one instruction
        if (!ExecuteInstruction())
        {
            SCRIPT_LOG_ERROR("VM execution failed");
            return false;
        }
        
        InstructionCount++;
    }
    
    double ExecutionTime = (FPlatformTime::Seconds() - ExecutionStartTime) * 1000.0;
    SCRIPT_LOG(FString_Utils::Printf("=== VM EXECUTION COMPLETE ===\nExecuted %d instructions in %.2fms",
        InstructionCount, ExecutionTime));
    
    return true;
}

void FScriptVM::RegisterNativeFunction(const FString& Name, FNativeFunction Function)
{
    NativeFunctions.Add(Name, Function);
    SCRIPT_LOG(FString_Utils::Printf("Registered native function: %s", *Name));
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
        RuntimeError("Stack underflow");
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
        if (FunctionTable[i].Name == "Main")
        {
            MainFuncIndex = i;
            break;
        }
    }
    
    if (MainFuncIndex == -1)
    {
        // No Main function found, this is not an error
        SCRIPT_LOG("No Main() function found - script completed");
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
    Frame.FunctionName = "Main";
    
    CallFrames.Add(Frame);
    
    // Jump to Main function
    InstructionPointer = MainFunc.Address;
    
    SCRIPT_LOG("Calling Main() function...");
    
    // Now execute until we return from Main
    while (InstructionPointer < CurrentBytecode->Code.Num() && CallFrames.Num() > 0)
    {
        // Safety checks
        if (!CheckInstructionLimit() || !CheckTimeout())
        {
            return false;
        }
        
        // Execute one instruction
        if (!ExecuteInstruction())
        {
            SCRIPT_LOG_ERROR("VM execution failed in Main()");
            return false;
        }
        
        InstructionCount++;
    }
    
    // Check if we have a return value
    if (Stack.Num() > 0)
    {
        FScriptValue ReturnValue = Pop();
        SCRIPT_LOG(FString_Utils::Printf("Main() returned: %s", *ReturnValue.ToString()));
    }
    
    SCRIPT_LOG("Main() function completed");
    return true;
}

void FScriptVM::RuntimeError(const FString& Message)
{
    Errors.Add(Message);
    SCRIPT_LOG_ERROR(FString_Utils::Printf("Runtime Error: %s", *Message));
    SCRIPT_LOG_ERROR(FString_Utils::Printf("  At instruction %d", InstructionPointer));
    
    // Dump stack for debugging
    if (Stack.Num() > 0)
    {
        SCRIPT_LOG_ERROR("  Stack trace:");
        for (int32 i = Stack.Num() - 1; i >= 0 && i >= Stack.Num() - 5; --i)
        {
            SCRIPT_LOG_ERROR(FString_Utils::Printf("    [%d] %s", i, *Stack[i].ToString()));
        }
    }
}

bool FScriptVM::CheckStackOverflow()
{
    if (Stack.Num() >= Limits.MaxStackDepth)
    {
        RuntimeError(FString_Utils::Printf("Stack overflow (max depth: %d)", Limits.MaxStackDepth));
        return false;
    }
    return true;
}

bool FScriptVM::CheckCallDepth()
{
    if (CallFrames.Num() >= Limits.MaxCallDepth)
    {
        RuntimeError(FString_Utils::Printf("Call stack overflow (max depth: %d)", Limits.MaxCallDepth));
        return false;
    }
    return true;
}

bool FScriptVM::CheckInstructionLimit()
{
    if (InstructionCount >= Limits.MaxInstructionsPerFrame)
    {
        RuntimeError(FString_Utils::Printf("Instruction limit exceeded (max: %d)", Limits.MaxInstructionsPerFrame));
        return false;
    }
    return true;
}

bool FScriptVM::CheckTimeout()
{
    double ElapsedMs = (FPlatformTime::Seconds() - ExecutionStartTime) * 1000.0;
    if (ElapsedMs > Limits.MaxExecutionTimeMs)
    {
        RuntimeError(FString_Utils::Printf("Execution timeout (max: %.2fms, actual: %.2fms)", 
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
        RuntimeError("Instruction pointer out of bounds");
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
        case EOpCode::OP_NEGATE:        OpNegate(); break;
        
        case EOpCode::OP_EQUAL:         OpEqual(); break;
        case EOpCode::OP_GREATER:       OpGreater(); break;
        case EOpCode::OP_LESS:          OpLess(); break;
        case EOpCode::OP_NOT:           OpNot(); break;
        case EOpCode::OP_AND:           OpAnd(); break;
        case EOpCode::OP_OR:            OpOr(); break;
        
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
            SCRIPT_LOG("VM halted");
            return false;
        
        default:
            RuntimeError(FString_Utils::Printf("Unknown opcode: %d", static_cast<int32>(OpCode)));
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
        RuntimeError("Operands must be numbers or strings");
    }
}

void FScriptVM::OpSubtract()
{
    FScriptValue B = Pop();
    FScriptValue A = Pop();
    
    if (!A.IsNumber() || !B.IsNumber())
    {
        RuntimeError("Operands must be numbers");
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
        RuntimeError("Operands must be numbers");
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
        RuntimeError("Operands must be numbers");
        return;
    }
    
    if (B.AsNumber() == 0.0)
    {
        RuntimeError("Division by zero");
        return;
    }
    
    Push(FScriptValue::Number(A.AsNumber() / B.AsNumber()));
}

void FScriptVM::OpNegate()
{
    FScriptValue Value = Pop();
    
    if (!Value.IsNumber())
    {
        RuntimeError("Operand must be a number");
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
        RuntimeError("Operands must be numbers");
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
        RuntimeError("Operands must be numbers");
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

void FScriptVM::OpGetLocal()
{
    uint8 Slot = ReadByte();
    
    int32 StackIndex = CallFrames.Num() > 0 ? CallFrames.Last().StackBase + Slot : Slot;
    
    if (StackIndex >= Stack.Num())
    {
        RuntimeError(FString_Utils::Printf("Invalid local variable slot: %d", Slot));
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
        RuntimeError(FString_Utils::Printf("Invalid local variable slot: %d", Slot));
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
        RuntimeError("Global variable name must be a string");
        return;
    }
    
    FString VarName = NameValue.AsString();
    FScriptValue Value = Pop(); // Get initialization value from stack
    
    // Store in globals table
    Globals.Add(VarName, Value);
    
    SCRIPT_LOG(FString_Utils::Printf("Defined global variable: %s = %s", *VarName, *Value.ToString()));
}

void FScriptVM::OpGetGlobal()
{
    // Read global variable name from constant pool
    FScriptValue NameValue = ReadConstant();
    if (!NameValue.IsString())
    {
        RuntimeError("Global variable name must be a string");
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
        RuntimeError(FString_Utils::Printf("Undefined global variable: %s", *VarName));
        Push(FScriptValue::Nil());
    }
}

void FScriptVM::OpSetGlobal()
{
    // Read global variable name from constant pool
    FScriptValue NameValue = ReadConstant();
    if (!NameValue.IsString())
    {
        RuntimeError("Global variable name must be a string");
        return;
    }
    
    FString VarName = NameValue.AsString();
    
    // Check if variable exists
    if (!Globals.Contains(VarName))
    {
        RuntimeError(FString_Utils::Printf("Cannot assign to undefined global variable: %s", *VarName));
        return;
    }
    
    // Set value (peek, don't pop - assignment is an expression)
    FScriptValue Value = Peek(0);
    Globals[VarName] = Value;
    
    SCRIPT_LOG(FString_Utils::Printf("Set global variable: %s = %s", *VarName, *Value.ToString()));
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
        RuntimeError(FString_Utils::Printf("Invalid function index: %d", FuncIndex));
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
        RuntimeError(FString_Utils::Printf("Argument count mismatch for function '%s': expected %d, got %d", 
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
        RuntimeError("Invalid native function name index");
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
        FScriptValue Result = (*NativeFunc)(Args);
        Push(Result);
    }
    else
    {
        SCRIPT_LOG_WARNING(FString_Utils::Printf("Native function '%s' not found - pushing nil", *FuncName));
        Push(FScriptValue::Nil());
    }
}

void FScriptVM::OpReturn()
{
    FScriptValue Result = Pop();
    
    if (CallFrames.Num() > 0)
    {
        // Return from function
        FCallFrame Frame = CallFrames.Last();
        CallFrames.Pop();
        
        // Pop arguments and any locals created in this frame
        // Stack at return time: [Frame.StackBase] ... arguments ... [top] locals created during function
        int32 ArgsCount = 0; // We don't store this directly, so we'll determine from bytecode analysis in future
        // For now, simplify the return process:
        // 1. We have the return value on stack top
        // 2. We need to remove all values from this frame (arguments + local variables)
        // 3. Put the return value back in place of the original arguments
        int32 ValuesToPop = Stack.Num() - Frame.StackBase;
        
        // Pop all values created during the function execution (arguments and locals)
        for (int32 i = 0; i < ValuesToPop; ++i)
        {
            if (Stack.Num() > Frame.StackBase)
            {
                Stack.Pop();
            }
        }
        
        // Push the return value in place of the original arguments
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
        RuntimeError("Cannot cast to int");
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
        RuntimeError("Cannot cast to float");
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
    SCRIPT_LOG(FString_Utils::Printf("[PRINT] %s", *Value.ToString()));
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
        RuntimeError("Operands must be numbers");
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
        RuntimeError("Operands must be numbers");
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
        RuntimeError("Subscript operator requires array");
        Push(FScriptValue::Nil()); // Push a default value
        return;
    }
    
    if (!Index.IsNumber())
    {
        RuntimeError("Array index must be a number");
        Push(FScriptValue::Nil()); // Push a default value
        return;
    }
    
    int32 Idx = static_cast<int32>(Index.AsNumber());
    const TArray<FScriptValue>& ArrayElements = Array.AsArray();
    
    if (Idx < 0 || Idx >= ArrayElements.Num())
    {
        RuntimeError("Array index out of bounds");
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
        RuntimeError("Subscript assignment requires array");
        return;
    }
    
    if (!Index.IsNumber())
    {
        RuntimeError("Array index must be a number");
        return;
    }
    
    int32 Idx = static_cast<int32>(Index.AsNumber());
    TArray<FScriptValue> ArrayElements = Array.AsArray(); // Copy the array
    
    if (Idx < 0 || Idx >= ArrayElements.Num())
    {
        RuntimeError("Array index out of bounds");
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
        RuntimeError("Stack underflow - cannot duplicate");
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
        RuntimeError(FString_Utils::Printf("Invalid field name index: %d", NameIndex));
        return;
    }
    
    FString FieldName = CurrentBytecode->Constants[NameIndex].AsString();
    FScriptValue Object = Pop();
    
    // Handle array properties
    if (Object.IsArray())
    {
        if (FieldName == "length")
        {
            Push(FScriptValue::Number(static_cast<double>(Object.AsArray().Num())));
            return;
        }
        // Could add more array properties here in the future
    }
    
    // For now, we'll return nil for any other field access
    // In a full implementation, we would handle struct/object properties here
    SCRIPT_LOG_WARNING(FString_Utils::Printf("Object field '%s' not found, returning nil", *FieldName));
    Push(FScriptValue::Nil());
}

void FScriptVM::OpSetField()
{
    // Field name is encoded as a 16-bit constant index in the bytecode
    uint16 NameIndex = ReadShort();
    
    if (NameIndex >= CurrentBytecode->Constants.Num())
    {
        RuntimeError(FString_Utils::Printf("Invalid field name index: %d", NameIndex));
        return;
    }
    
    FString FieldName = CurrentBytecode->Constants[NameIndex].AsString();
    FScriptValue Value = Pop();  // The value to assign
    FScriptValue Object = Pop(); // The object to modify
    
    // For now, make fields read-only by default
    // In a full implementation, we would handle struct/object field assignment
    SCRIPT_LOG_WARNING(FString_Utils::Printf("Setting object field '%s' not implemented", *FieldName));
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
        RuntimeError("Unexpected end of bytecode");
        return 0;
    }
    return CurrentBytecode->Code[InstructionPointer++];
}

uint16 FScriptVM::ReadShort()
{
    if (InstructionPointer + 1 >= CurrentBytecode->Code.Num())
    {
        RuntimeError("Unexpected end of bytecode");
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
        RuntimeError(FString_Utils::Printf("Invalid constant index: %d", Index));
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
    SCRIPT_LOG("=== Stack Dump ===");
    for (int32 i = 0; i < Stack.Num(); ++i)
    {
        SCRIPT_LOG(FString_Utils::Printf("  [%d] %s", i, *Stack[i].ToString()));
    }
}



