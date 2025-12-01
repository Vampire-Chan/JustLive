// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#include "ScriptCompiler.h"
#include "ScriptLogger.h"
#include "ScriptLexer.h"
#include "ScriptParser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Initialize known native functions
const TSet<FString> FScriptCompiler::NativeFunctions = {
    // Utility
    TEXT("Log"), TEXT("Print"), TEXT("Sleep"),
    
    // Script Management
    TEXT("LoadScript"), TEXT("RunScript"), TEXT("DoesScriptExist"),
    TEXT("IsScriptRunning"), TEXT("CanRunScript"), TEXT("IsMissionScript"),
    
    // Collections - List
    TEXT("List_Create"), TEXT("List_Add"), TEXT("List_Get"), TEXT("List_Set"),
    TEXT("List_RemoveAt"), TEXT("List_Count"), TEXT("List_Clear"), TEXT("List_Contains"),
    
    // Collections - Dictionary
    TEXT("Dict_Create"), TEXT("Dict_Set"), TEXT("Dict_Get"), TEXT("Dict_Remove"),
    TEXT("Dict_HasKey"), TEXT("Dict_Clear"), TEXT("Dict_Count"),
    
    // Math - Basic
    TEXT("Add"), TEXT("Subtract"), TEXT("Multiply"), TEXT("Divide"), TEXT("Mod"), TEXT("Pow"),
    
    // Math - Trig
    TEXT("Sin"), TEXT("Cos"), TEXT("Tan"), TEXT("Asin"), TEXT("Acos"), TEXT("Atan"), TEXT("Atan2"),
    
    // Math - Helpers
    TEXT("Abs"), TEXT("Sqrt"), TEXT("Floor"), TEXT("Ceil"), TEXT("Round"), TEXT("Clamp"),
    TEXT("Min"), TEXT("Max"), TEXT("DegreesToRadians"), TEXT("RadiansToDegrees"),
    TEXT("Log"), TEXT("Exp"),
    
    // Math - Random
    TEXT("RandomFloat"), TEXT("RandomRange"), TEXT("RandomBool"),
    
    // Math - Vector
    TEXT("Vector"), // Constructor
    TEXT("Vector_Add"), TEXT("Vector_Sub"), TEXT("Vector_Mul"), TEXT("Vector_Div"),
    TEXT("Vector_Dot"), TEXT("Vector_Cross"), TEXT("Vector_Dist"), TEXT("Vector_DistSquared"),
    TEXT("Vector_Normalize"), TEXT("Vector_Length"), TEXT("Vector_Lerp"),
    
    // String
    TEXT("String_Len"), TEXT("String_Sub"), TEXT("String_Find"), TEXT("String_Upper"),
    TEXT("String_Lower"), TEXT("String_Replace"), TEXT("String_Trim"), TEXT("String_Split"),
    TEXT("String_Contains"), TEXT("String_FromChar"), TEXT("String_ToChar"),
    
    // UI
    TEXT("UI_SwitchState"), TEXT("UI_ShowLoading"), TEXT("UI_UpdateLoading"),
    
    // Audio
    TEXT("Audio_PlaySound"), TEXT("Audio_PlayMusic"), TEXT("Audio_StopMusic"),
    TEXT("Music_Next"), TEXT("Music_Prev"), TEXT("Music_Pause"), TEXT("Music_Resume"),
    TEXT("Music_SetVolume"), TEXT("Music_SetShuffle"),
    TEXT("SFX_PlayLoop"), TEXT("SFX_StopLoop"),
    
    // Light
    TEXT("Light_SetColor"), TEXT("Light_SetIntensity"), TEXT("Light_Toggle"),
    
    // Decal
    TEXT("Decal_Spawn")
};

FScriptCompiler::FScriptCompiler()
    : ScopeDepth(0)
    , bLastExpressionWasVoidCall(false)
{
}

TSharedPtr<FBytecodeChunk> FScriptCompiler::Compile(TSharedPtr<FScriptProgram> Program)
{
    if (!Program.IsValid() || !Program->IsValid())
    {
        ReportError(TEXT("Invalid program AST"));
        return nullptr;
    }
    
    Chunk = MakeShared<FBytecodeChunk>();
    Errors.Empty();
    Locals.Empty();
    Functions.Empty();
    ImportedFiles.Empty();
    ScopeDepth = 0;
    bLastExpressionWasVoidCall = false;
    
    SCRIPT_LOG(TEXT("=== COMPILER PHASE ==="));
    
    CompileProgram(Program.Get());
    
    if (HasErrors())
    {
        SCRIPT_LOG_ERROR(TEXT("Compilation failed with errors"));
        return nullptr;
    }
    
    // No need for final return - CompileProgram emits HALT for function-only programs
    // and global code already handles its own returns
    
    SCRIPT_LOG(FString::Printf(TEXT("Compilation successful! Generated %d bytes of bytecode"), 
        Chunk->Code.Num()));
    
    return Chunk;
}

void FScriptCompiler::ReportError(const FString& Message)
{
    Errors.Add(Message);
    SCRIPT_LOG_ERROR(FString::Printf(TEXT("Compiler Error: %s"), *Message));
}

//=============================================================================
// Scope Management
//=============================================================================

void FScriptCompiler::BeginScope()
{
    ScopeDepth++;
}

void FScriptCompiler::EndScope()
{
    ScopeDepth--;
    
    // Pop all locals from this scope
    while (Locals.Num() > 0 && Locals.Last().Depth > ScopeDepth)
    {
        EmitByte((uint8)EOpCode::OP_POP);
        Locals.Pop();
    }
}

int32 FScriptCompiler::ResolveLocal(const FString& Name)
{
    for (int32 i = Locals.Num() - 1; i >= 0; --i)
    {
        if (Locals[i].Name == Name)
        {
            if (!Locals[i].bInitialized)
            {
                ReportError(FString::Printf(TEXT("Variable '%s' used before initialization"), *Name));
            }
            return i;
        }
    }
    return -1; // Not found - might be global
}

int32 FScriptCompiler::AddLocal(const FString& Name, EScriptType Type)
{
    // Check for duplicate in current scope
    for (int32 i = Locals.Num() - 1; i >= 0; --i)
    {
        if (Locals[i].Depth < ScopeDepth)
        {
            break;
        }
        if (Locals[i].Name == Name)
        {
            ReportError(FString::Printf(TEXT("Variable '%s' already declared in this scope"), *Name));
            return -1;
        }
    }
    
    FLocal Local;
    Local.Name = Name;
    Local.Depth = ScopeDepth;
    Local.Type = Type;
    Local.bInitialized = false;
    
    Locals.Add(Local);
    return Locals.Num() - 1;
}

int32 FScriptCompiler::ResolveFunction(const FString& Name)
{
    for (int32 i = 0; i < Functions.Num(); ++i)
    {
        if (Functions[i].Name == Name)
        {
            return i;
        }
    }
    return -1;
}

//=============================================================================
// Program Compilation
//=============================================================================

void FScriptCompiler::CompileProgram(FScriptProgram* Program)
{
    // FIRST: Process all imports to load functions from headers
    for (const auto& Stmt : Program->Statements)
    {
        if (Stmt.IsValid() && Stmt->GetNodeType() == TEXT("Import"))
        {
            CompileImport(static_cast<FImportStmt*>(Stmt.Get()));
        }
    }
    
    // Check for global variable THISISAMISSION = true
    for (const auto& Stmt : Program->Statements)
    {
        if (Stmt.IsValid() && Stmt->GetNodeType() == TEXT("VarDecl"))
        {
            FVarDeclStmt* VarDecl = static_cast<FVarDeclStmt*>(Stmt.Get());
            if (VarDecl->Name.Lexeme == TEXT("THISISAMISSION"))
            {
                SCRIPT_LOG(TEXT("Compiler: Found THISISAMISSION variable."));
                if (VarDecl->Initializer.IsValid())
                {
                    if (VarDecl->Initializer->GetNodeType() == TEXT("Literal"))
                    {
                        FLiteralExpr* Literal = static_cast<FLiteralExpr*>(VarDecl->Initializer.Get());
                        if (Literal && Literal->Token.Type == ETokenType::KW_TRUE)
                        {
                            Chunk->Metadata.bIsMission = true;
                            SCRIPT_LOG(TEXT("Compiler: Detected THISISAMISSION=true. Marking as mission script."));
                        }
                        else
                        {
                            SCRIPT_LOG(TEXT("Compiler: THISISAMISSION is not true."));
                        }
                    }
                    else
                    {
                        SCRIPT_LOG(FString::Printf(TEXT("Compiler: THISISAMISSION initializer is not a literal (Type: %s)"), *VarDecl->Initializer->GetNodeType()));
                    }
                }
            }
        }
    }
    
    // SECOND: Register all functions (including imported ones from headers)
    for (const auto& Func : Program->Functions)
    {
        if (Func.IsValid())
        {
            FFunction FuncInfo;
            FuncInfo.Name = Func->Name.Lexeme;
            // Use TypedParameters if available, otherwise fall back to Parameters
            FuncInfo.Arity = Func->TypedParameters.Num() > 0 ? Func->TypedParameters.Num() : Func->Parameters.Num();
            FuncInfo.Address = -1; // Will be set during compilation
            FuncInfo.ReturnType = Func->ReturnType;  // Use the return type from function declaration
            Functions.Add(FuncInfo);
        }
    }
    
    // Emit a jump to skip over function definitions (will be patched later)
    // Only if we have functions to skip over
    int32 JumpOverFunctions = -1;
    if (Program->Functions.Num() > 0 || Functions.Num() > 0)
    {
        JumpOverFunctions = EmitJump(EOpCode::OP_JUMP);
    }
    
    // THIRD: Compile functions (local + imported, so they're at the beginning of bytecode)
    for (const auto& Func : Program->Functions)
    {
        if (Func.IsValid())
        {
            CompileFunction(Func.Get());
        }
    }
    
    // Patch the jump to point here (after all functions)
    if (JumpOverFunctions >= 0)
    {
        PatchJump(JumpOverFunctions);
    }
    
    // FOURTH: Compile global statements (top-level code), skipping imports (already processed)
    for (const auto& Stmt : Program->Statements)
    {
        if (Stmt.IsValid() && Stmt->GetNodeType() != TEXT("Import"))
        {
            CompileStatement(Stmt.Get());
        }
    }
    
    // If there were no global statements, emit a HALT so the VM doesn't try to execute function bodies
    if (Program->Statements.Num() == 0 && Program->Functions.Num() > 0)
    {
        EmitByte((uint8)EOpCode::OP_HALT);
    }
    
    // Add function table to bytecode for runtime lookup
    for (int32 i = 0; i < Functions.Num(); ++i)
    {
        if (Functions[i].Address != -1) // Only if function was compiled
        {
            // Add to bytecode function table
            FFunctionInfo FuncInfo(Functions[i].Name, Functions[i].Address, Functions[i].Arity);
            Chunk->Functions.Add(FuncInfo);
            
            SCRIPT_LOG(FString::Printf(TEXT("Added function to table: %s (address=%d, arity=%d)"),
                *Functions[i].Name, Functions[i].Address, Functions[i].Arity));
        }
    }
}

void FScriptCompiler::CompileFunction(FFunctionDecl* Function)
{
    // Mark function address
    int32 FuncIndex = ResolveFunction(Function->Name.Lexeme);
    if (FuncIndex >= 0)
    {
        Functions[FuncIndex].Address = Chunk->Code.Num();
    }
    
    SCRIPT_LOG(FString::Printf(TEXT("Compiling function '%s' at address %d"), 
        *Function->Name.Lexeme, Chunk->Code.Num()));
    
    BeginScope();
    
    // Add parameters as locals (prefer typed parameters)
    if (Function->TypedParameters.Num() > 0)
    {
        for (const FParameter& Param : Function->TypedParameters)
        {
            AddLocal(Param.Name.Lexeme, Param.Type);
            if (Locals.Num() > 0)
            {
                Locals.Last().bInitialized = true; // Parameters are initialized
            }
        }
    }
    else
    {
        // Fallback to untyped parameters
        for (const FScriptToken& Param : Function->Parameters)
        {
            AddLocal(Param.Lexeme, EScriptType::AUTO);
            if (Locals.Num() > 0)
            {
                Locals.Last().bInitialized = true; // Parameters are initialized
            }
        }
    }
    
    // Compile function body
    if (Function->Body.IsValid())
    {
        CompileStatement(Function->Body.Get());
    }
    
    // Implicit return only for void functions or if no explicit return was found
    // Note: We always add implicit return for void functions to ensure proper cleanup
    // For non-void functions, we trust they have explicit returns (checked by parser)
    if (Functions[FuncIndex].ReturnType == EScriptType::VOID)
    {
        EmitByte((uint8)EOpCode::OP_NIL);
        EmitReturn();
    }
    // For non-void functions, the explicit return in the function body handles it
    // If function is missing a return, that's a semantic error that should be caught earlier
    
    // NOTE: We don't call EndScope() here because OP_RETURN already handles cleanup
    // Calling EndScope() would emit dead POPs after the RETURN instruction
    // Instead, we manually clean up the locals tracking without emitting bytecode
    while (Locals.Num() > 0 && Locals.Last().Depth >= ScopeDepth)
    {
        Locals.Pop();
    }
    ScopeDepth--;
}

//=============================================================================
// Statement Compilation
//=============================================================================

void FScriptCompiler::CompileStatement(FScriptStatement* Statement)
{
    if (!Statement || !Statement->IsValid())
    {
        ReportError(TEXT("Invalid statement"));
        return;
    }
    
    FString NodeType = Statement->GetNodeType();
    
    if (NodeType == TEXT("ExprStmt"))
    {
        CompileExprStmt(static_cast<FExprStmt*>(Statement));
    }
    else if (NodeType == TEXT("VarDecl"))
    {
        CompileVarDecl(static_cast<FVarDeclStmt*>(Statement));
    }
    else if (NodeType == TEXT("Block"))
    {
        CompileBlock(static_cast<FBlockStmt*>(Statement));
    }
    else if (NodeType == TEXT("If"))
    {
        CompileIf(static_cast<FIfStmt*>(Statement));
    }
    else if (NodeType == TEXT("While"))
    {
        CompileWhile(static_cast<FWhileStmt*>(Statement));
    }
    else if (NodeType == TEXT("For"))
    {
        CompileFor(static_cast<FForStmt*>(Statement));
    }
    else if (NodeType == TEXT("Break"))
    {
        CompileBreak(static_cast<FBreakStmt*>(Statement));
    }
    else if (NodeType == TEXT("Continue"))
    {
        CompileContinue(static_cast<FContinueStmt*>(Statement));
    }
    else if (NodeType == TEXT("Switch"))
    {
        CompileSwitch(static_cast<FSwitchStmt*>(Statement));
    }
    else if (NodeType == TEXT("Return"))
    {
        CompileReturn(static_cast<FReturnStmt*>(Statement));
    }
    else if (NodeType == TEXT("Import"))
    {
        CompileImport(static_cast<FImportStmt*>(Statement));
    }
    else
    {
        ReportError(FString::Printf(TEXT("Unknown statement type: %s"), *NodeType));
    }
}

void FScriptCompiler::CompileExprStmt(FExprStmt* Stmt)
{
    bLastExpressionWasVoidCall = false; // Reset flag
    CompileExpression(Stmt->Expression.Get());
    
    // Always pop the expression result from the stack
    // Even void functions leave a value (nil) on the stack in our VM
    EmitByte((uint8)EOpCode::OP_POP); // Discard expression result
}

void FScriptCompiler::CompileVarDecl(FVarDeclStmt* Stmt)
{
    // Check if we're at global scope (ScopeDepth == 0)
    bool bIsGlobal = (ScopeDepth == 0);
    
    // Compile initializer (or use nil)
    if (Stmt->Initializer.IsValid())
    {
        CompileExpression(Stmt->Initializer.Get());
        
        // Type conversion if needed
        EScriptType ExprType = InferType(Stmt->Initializer.Get());
        if (Stmt->VarType != EScriptType::AUTO && ExprType != Stmt->VarType)
        {
            EmitTypeConversion(ExprType, Stmt->VarType);
        }
    }
    else
    {
        // Initialize to default value
        EmitByte((uint8)EOpCode::OP_NIL);
    }
    
    if (bIsGlobal)
    {
        // Global variable: emit OP_DEFINE_GLOBAL with variable name
        int32 NameConstant = Chunk->AddConstant(FScriptValue::String(Stmt->Name.Lexeme));
        EmitByte((uint8)EOpCode::OP_DEFINE_GLOBAL);
        EmitByte((uint8)NameConstant);
        
        SCRIPT_LOG(FString::Printf(TEXT("Compiled global variable: %s"), *Stmt->Name.Lexeme));
    }
    else
    {
        // Local variable: add to locals table
        int32 LocalIndex = AddLocal(Stmt->Name.Lexeme, Stmt->VarType);
        
        if (LocalIndex >= 0 && LocalIndex < Locals.Num())
        {
            Locals[LocalIndex].bInitialized = true;
        }
        
        SCRIPT_LOG(FString::Printf(TEXT("Compiled local variable: %s (slot %d)"), *Stmt->Name.Lexeme, LocalIndex));
    }
}

void FScriptCompiler::CompileBlock(FBlockStmt* Stmt)
{
    BeginScope();
    
    for (const auto& Statement : Stmt->Statements)
    {
        if (Statement.IsValid())
        {
            CompileStatement(Statement.Get());
        }
    }
    
    EndScope();
}

void FScriptCompiler::CompileIf(FIfStmt* Stmt)
{
    // Compile condition
    CompileExpression(Stmt->Condition.Get());
    
    // Jump to else branch if condition is false
    int32 ThenJump = EmitJump(EOpCode::OP_JUMP_IF_FALSE);
    EmitByte((uint8)EOpCode::OP_POP); // Pop condition
    
    // Compile then branch
    CompileStatement(Stmt->ThenBranch.Get());
    
    // Jump over else branch
    int32 ElseJump = EmitJump(EOpCode::OP_JUMP);
    
    // Patch then jump to here
    PatchJump(ThenJump);
    EmitByte((uint8)EOpCode::OP_POP); // Pop condition
    
    // Compile else branch if present
    if (Stmt->ElseBranch.IsValid())
    {
        CompileStatement(Stmt->ElseBranch.Get());
    }
    
    // Patch else jump
    PatchJump(ElseJump);
}

void FScriptCompiler::CompileWhile(FWhileStmt* Stmt)
{
    int32 LoopStart = Chunk->Code.Num();
    
    // Push loop context for break/continue
    FLoopContext LoopCtx;
    LoopCtx.Start = LoopStart;
    LoopStack.Add(LoopCtx);
    
    // Compile condition
    CompileExpression(Stmt->Condition.Get());
    
    // Exit loop if condition is false
    int32 ExitJump = EmitJump(EOpCode::OP_JUMP_IF_FALSE);
    EmitByte((uint8)EOpCode::OP_POP); // Pop condition
    
    // Compile body
    CompileStatement(Stmt->Body.Get());
    
    // Loop back
    EmitLoop(LoopStart);
    
    // Patch exit jump
    PatchJump(ExitJump);
    EmitByte((uint8)EOpCode::OP_POP); // Pop condition
    
    // Patch all break jumps to here (after loop)
    FLoopContext& CurrentLoop = LoopStack.Last();
    for (int32 BreakJump : CurrentLoop.BreakJumps)
    {
        PatchJump(BreakJump);
    }
    
    // Pop loop context
    LoopStack.Pop();
}

void FScriptCompiler::CompileFor(FForStmt* Stmt)
{
    // For loops desugar to while loops:
    // for (init; condition; increment) body
    // =>
    // {
    //     init;
    //     while (condition) {
    //         body;
    //         increment;
    //     }
    // }
    
    BeginScope();
    
    // Compile initializer
    if (Stmt->Initializer.IsValid())
    {
        CompileStatement(Stmt->Initializer.Get());
    }
    
    int32 LoopStart = Chunk->Code.Num();
    
    // Push loop context for break/continue
    FLoopContext LoopCtx;
    LoopCtx.Start = LoopStart;
    LoopStack.Add(LoopCtx);
    
    // Compile condition (or default to true)
    int32 ExitJump = -1;
    if (Stmt->Condition.IsValid())
    {
        CompileExpression(Stmt->Condition.Get());
        ExitJump = EmitJump(EOpCode::OP_JUMP_IF_FALSE);
        EmitByte((uint8)EOpCode::OP_POP);
    }
    
    // Compile body
    if (Stmt->Body.IsValid())
    {
        CompileStatement(Stmt->Body.Get());
    }
    
    // Continue target: compile increment before looping
    int32 ContinueTarget = Chunk->Code.Num();
    if (Stmt->Increment.IsValid())
    {
        CompileExpression(Stmt->Increment.Get());
        EmitByte((uint8)EOpCode::OP_POP); // Discard increment result
    }
    
    // Loop back to condition
    EmitLoop(LoopStart);
    
    // Patch exit jump if we had a condition
    if (ExitJump != -1)
    {
        PatchJump(ExitJump);
        EmitByte((uint8)EOpCode::OP_POP);
    }
    
    // Patch all break jumps to here (after loop)
    FLoopContext& CurrentLoop = LoopStack.Last();
    for (int32 BreakJump : CurrentLoop.BreakJumps)
    {
        PatchJump(BreakJump);
    }
    
    // Pop loop context
    LoopStack.Pop();
    
    EndScope();
}

void FScriptCompiler::CompileBreak(FBreakStmt* Stmt)
{
    if (LoopStack.Num() == 0)
    {
        ReportError(TEXT("'break' can only be used inside a loop"));
        return;
    }
    
    // Jump to end of loop (will be patched later)
    int32 BreakJump = EmitJump(EOpCode::OP_JUMP);
    LoopStack.Last().BreakJumps.Add(BreakJump);
}

void FScriptCompiler::CompileContinue(FContinueStmt* Stmt)
{
    if (LoopStack.Num() == 0)
    {
        ReportError(TEXT("'continue' can only be used inside a loop"));
        return;
    }
    
    // Jump back to loop start
    EmitLoop(LoopStack.Last().Start);
}

void FScriptCompiler::CompileReturn(FReturnStmt* Stmt)
{
    if (Stmt->Value.IsValid())
    {
        CompileExpression(Stmt->Value.Get());
    }
    else
    {
        EmitByte((uint8)EOpCode::OP_NIL);
    }
    
    EmitByte((uint8)EOpCode::OP_RETURN);
}

void FScriptCompiler::CompileImport(FImportStmt* Stmt)
{
    // Extract the import path (lexer already strips quotes from string tokens)
    FString ImportPath = Stmt->Path.Lexeme;
    
    SCRIPT_LOG(FString::Printf(TEXT("  Processing import: %s"), *ImportPath));
    
    // Resolve full path - imports are relative to Scripts/ folder
    FString ProjectDir = FPaths::ProjectDir();
    FString ScriptsPath = FPaths::Combine(ProjectDir, TEXT("Scripts"));
    FString FullImportPath = FPaths::Combine(ScriptsPath, ImportPath);
    
    // Normalize the path
    FPaths::NormalizeFilename(FullImportPath);
    
    // Check for circular imports
    if (ImportedFiles.Contains(FullImportPath))
    {
        SCRIPT_LOG(FString::Printf(TEXT("  Already imported: %s (skipping)"), *ImportPath));
        return;
    }
    
    // Check if file exists
    if (!FPaths::FileExists(FullImportPath))
    {
        ReportError(FString::Printf(TEXT("Import file not found: %s"), *FullImportPath));
        return;
    }
    
    // Mark as imported
    ImportedFiles.Add(FullImportPath);
    
    // Load the header file
    FString HeaderSource;
    if (!FFileHelper::LoadFileToString(HeaderSource, *FullImportPath))
    {
        ReportError(FString::Printf(TEXT("Failed to read import file: %s"), *FullImportPath));
        return;
    }
    
    SCRIPT_LOG(FString::Printf(TEXT("  Loaded header: %s (%d bytes)"), *ImportPath, HeaderSource.Len()));
    
    // Parse the header file
    FScriptLexer HeaderLexer(HeaderSource);
    TArray<FScriptToken> HeaderTokens = HeaderLexer.ScanTokens();
    
    FScriptParser HeaderParser(HeaderTokens);
    TSharedPtr<FScriptProgram> HeaderProgram = HeaderParser.Parse();
    
    // Check for parse errors
    if (HeaderParser.HasErrors())
    {
        ReportError(FString::Printf(TEXT("Parse errors in import file: %s"), *ImportPath));
        for (const FString& Error : HeaderParser.GetErrors())
        {
            ReportError(FString::Printf(TEXT("  %s"), *Error));
        }
        return;
    }
    
    // Compile the header's functions
    if (HeaderProgram.IsValid())
    {
        // First, handle any imports in the header (recursive)
        for (const TSharedPtr<FScriptASTNode>& Statement : HeaderProgram->Statements)
        {
            if (!Statement.IsValid()) continue;
            
            FString NodeType = Statement->GetNodeType();
            
            if (NodeType == TEXT("Import"))
            {
                CompileImport(static_cast<FImportStmt*>(Statement.Get()));
            }
        }
        
        // Register all functions from the imported header
        for (const auto& Func : HeaderProgram->Functions)
        {
            if (Func.IsValid())
            {
                FFunction FuncInfo;
                FuncInfo.Name = Func->Name.Lexeme;
                FuncInfo.Arity = Func->TypedParameters.Num() > 0 ? Func->TypedParameters.Num() : Func->Parameters.Num();
                FuncInfo.Address = -1; // Will be set during compilation
                FuncInfo.ReturnType = Func->ReturnType;
                Functions.Add(FuncInfo);
            }
        }
        
        // Then compile all function declarations from the header
        for (const auto& Func : HeaderProgram->Functions)
        {
            if (Func.IsValid())
            {
                CompileFunction(Func.Get());
            }
        }
        
        SCRIPT_LOG(FString::Printf(TEXT("  Import compiled: %s"), *ImportPath));
    }
}

//=============================================================================
// Expression Compilation
//=============================================================================

void FScriptCompiler::CompileExpression(FScriptExpression* Expression)
{
    if (!Expression || !Expression->IsValid())
    {
        ReportError(TEXT("Invalid expression"));
        return;
    }
    
    FString NodeType = Expression->GetNodeType();
    
    if (NodeType == TEXT("Literal"))
    {
        CompileLiteral(static_cast<FLiteralExpr*>(Expression));
    }
    else if (NodeType == TEXT("Binary"))
    {
        CompileBinary(static_cast<FBinaryExpr*>(Expression));
    }
    else if (NodeType == TEXT("Unary"))
    {
        CompileUnary(static_cast<FUnaryExpr*>(Expression));
    }
    else if (NodeType == TEXT("Identifier"))
    {
        CompileIdentifier(static_cast<FIdentifierExpr*>(Expression));
    }
    else if (NodeType == TEXT("Assign"))
    {
        CompileAssign(static_cast<FAssignExpr*>(Expression));
    }
    else if (NodeType == TEXT("Call"))
    {
        CompileCall(static_cast<FCallExpr*>(Expression));
    }
    else if (NodeType == TEXT("ArrayLiteral"))
    {
        CompileArrayLiteral(static_cast<FArrayLiteralExpr*>(Expression));
    }
    else if (NodeType == TEXT("ArrayAccess"))
    {
        CompileArrayAccess(static_cast<FArrayAccessExpr*>(Expression));
    }
    else if (NodeType == TEXT("ArrayAssign"))
    {
        CompileArrayAssign(static_cast<FArrayAssignExpr*>(Expression));
    }
    else if (NodeType == TEXT("StructAccess"))
    {
        CompileStructAccess(static_cast<FStructAccessExpr*>(Expression));
    }
    else if (NodeType == TEXT("StructAssign"))
    {
        CompileStructAssign(static_cast<FStructAssignExpr*>(Expression));
    }
    else if (NodeType == TEXT("TypeCast") || NodeType == TEXT("Cast"))
    {
        CompileTypeCast(static_cast<FTypeCastExpr*>(Expression));
    }
    else
    {
        ReportError(FString::Printf(TEXT("Unknown expression type: %s"), *NodeType));
    }
}

void FScriptCompiler::CompileLiteral(FLiteralExpr* Expr)
{
    // Parse the literal value from token
    FString Lexeme = Expr->Token.Lexeme;
    
    if (Expr->Token.Type == ETokenType::NUMBER)
    {
        double Value = FCString::Atod(*Lexeme);
        EmitConstant(FScriptValue::Number(Value));
    }
    else if (Expr->Token.Type == ETokenType::STRING)
    {
        EmitConstant(FScriptValue::String(Lexeme));
    }
    else if (Expr->Token.Type == ETokenType::KW_TRUE)
    {
        EmitConstant(FScriptValue::Bool(true));
    }
    else if (Expr->Token.Type == ETokenType::KW_FALSE)
    {
        EmitConstant(FScriptValue::Bool(false));
    }
    else if (Expr->Token.Type == ETokenType::NIL)
    {
        EmitByte((uint8)EOpCode::OP_NIL);
    }
    else
    {
        ReportError(FString::Printf(TEXT("Unknown literal type: %s"), *Lexeme));
    }
}

void FScriptCompiler::CompileBinary(FBinaryExpr* Expr)
{
    // Compile operands
    CompileExpression(Expr->Left.Get());
    CompileExpression(Expr->Right.Get());
    
    // Emit operator
    switch (Expr->Operator.Type)
    {
        // Arithmetic
        case ETokenType::PLUS:    EmitByte((uint8)EOpCode::OP_ADD); break;
        case ETokenType::MINUS:   EmitByte((uint8)EOpCode::OP_SUBTRACT); break;
        case ETokenType::STAR:    EmitByte((uint8)EOpCode::OP_MULTIPLY); break;
        case ETokenType::SLASH:   EmitByte((uint8)EOpCode::OP_DIVIDE); break;
        case ETokenType::PERCENT: EmitByte((uint8)EOpCode::OP_MODULO); break;
        
        // Comparison
        case ETokenType::EQUAL_EQUAL:   EmitByte((uint8)EOpCode::OP_EQUAL); break;
        case ETokenType::BANG_EQUAL:    EmitBytes((uint8)EOpCode::OP_EQUAL, (uint8)EOpCode::OP_NOT); break;
        case ETokenType::GREATER:       EmitByte((uint8)EOpCode::OP_GREATER); break;
        case ETokenType::GREATER_EQUAL: EmitBytes((uint8)EOpCode::OP_LESS, (uint8)EOpCode::OP_NOT); break;
        case ETokenType::LESS:          EmitByte((uint8)EOpCode::OP_LESS); break;
        case ETokenType::LESS_EQUAL:    EmitBytes((uint8)EOpCode::OP_GREATER, (uint8)EOpCode::OP_NOT); break;
        
        // Logical
        case ETokenType::AND:              EmitByte((uint8)EOpCode::OP_AND); break;
        case ETokenType::OR:               EmitByte((uint8)EOpCode::OP_OR); break;
        case ETokenType::AMPERSAND_AMPERSAND: EmitByte((uint8)EOpCode::OP_AND); break;  // && same as 'and'
        case ETokenType::PIPE_PIPE:        EmitByte((uint8)EOpCode::OP_OR); break;   // || same as 'or'
        
        // Bitwise
        case ETokenType::AMPERSAND: EmitByte((uint8)EOpCode::OP_BIT_AND); break;
        case ETokenType::PIPE:      EmitByte((uint8)EOpCode::OP_BIT_OR); break;
        case ETokenType::CARET:     EmitByte((uint8)EOpCode::OP_BIT_XOR); break;
        
        default:
            ReportError(TEXT("Unknown binary operator: ") + Expr->Operator.Lexeme);
    }
}

void FScriptCompiler::CompileUnary(FUnaryExpr* Expr)
{
    CompileExpression(Expr->Right.Get());
    
    switch (Expr->Operator.Type)
    {
        case ETokenType::MINUS: EmitByte((uint8)EOpCode::OP_NEGATE); break;
        case ETokenType::BANG:  EmitByte((uint8)EOpCode::OP_NOT); break;
        case ETokenType::TILDE: EmitByte((uint8)EOpCode::OP_BIT_NOT); break;
        default:
            ReportError(TEXT("Unknown unary operator: ") + Expr->Operator.Lexeme);
    }
}

void FScriptCompiler::CompileIdentifier(FIdentifierExpr* Expr)
{
    FString Name = Expr->Name.Lexeme;
    int32 LocalIndex = ResolveLocal(Name);
    
    if (LocalIndex >= 0)
    {
        // Local variable
        EmitBytes((uint8)EOpCode::OP_GET_LOCAL, (uint8)LocalIndex);
    }
    else
    {
        // Global variable - emit OP_GET_GLOBAL with variable name
        int32 NameConstant = Chunk->AddConstant(FScriptValue::String(Name));
        EmitByte((uint8)EOpCode::OP_GET_GLOBAL);
        EmitByte((uint8)NameConstant);
    }
}

void FScriptCompiler::CompileAssign(FAssignExpr* Expr)
{
    // Assignment targets can be:
    // - Identifier (local/global variable)
    // - Array access (arr[index])
    // - Struct/object field access (obj.field)

    FString TargetType = Expr->Target->GetNodeType();

    if (TargetType == TEXT("Identifier"))
    {
        FIdentifierExpr* Target = static_cast<FIdentifierExpr*>(Expr->Target.Get());
        FString Name = Target->Name.Lexeme;
        int32 LocalIndex = ResolveLocal(Name);

        // Compile value
        CompileExpression(Expr->Value.Get());

        if (LocalIndex >= 0)
        {
            // Set local variable
            EmitBytes((uint8)EOpCode::OP_SET_LOCAL, (uint8)LocalIndex);
        }
        else
        {
            // Set global variable - emit OP_SET_GLOBAL with variable name
            int32 NameConstant = Chunk->AddConstant(FScriptValue::String(Name));
            EmitByte((uint8)EOpCode::OP_SET_GLOBAL);
            EmitByte((uint8)NameConstant);
        }
        return;
    }

    if (TargetType == TEXT("ArrayAccess"))
    {
        // arr[index] = value
        FArrayAccessExpr* Arr = static_cast<FArrayAccessExpr*>(Expr->Target.Get());

        // Compile array expression, index, then value
        CompileExpression(Arr->Array.Get());
        CompileExpression(Arr->Index.Get());
        CompileExpression(Expr->Value.Get());

        // Emit array assignment
        EmitByte((uint8)EOpCode::OP_SET_ELEMENT);

        // If the array expression is a simple identifier (variable), we must write
        // the modified array back into that variable (locals or globals).
        if (Arr->Array.IsValid() && Arr->Array->GetNodeType() == TEXT("Identifier"))
        {
            FIdentifierExpr* Id = static_cast<FIdentifierExpr*>(Arr->Array.Get());
            FString Name = Id->Name.Lexeme;
            int32 LocalIndex = ResolveLocal(Name);

            if (LocalIndex >= 0)
            {
                // Sets the local slot to the value on top of the stack (assignment is expression)
                EmitBytes((uint8)EOpCode::OP_SET_LOCAL, (uint8)LocalIndex);
            }
            else
            {
                int32 NameConstant = Chunk->AddConstant(FScriptValue::String(Name));
                EmitByte((uint8)EOpCode::OP_SET_GLOBAL);
                EmitByte((uint8)NameConstant);
            }

            return;
        }

        // Unsupported array assignment target (e.g., expression that's not a variable)
        ReportError(TEXT("Array assignment target must be a variable identifier"));
        return;
    }

    if (TargetType == TEXT("StructAccess"))
    {
        // obj.field = value
        FStructAccessExpr* Field = static_cast<FStructAccessExpr*>(Expr->Target.Get());

        // Compile object, then value
        CompileExpression(Field->Object.Get());

        int32 FieldNameIndex = Chunk->AddConstant(FScriptValue::String(Field->Field.Lexeme));
        CompileExpression(Expr->Value.Get());

        EmitByte((uint8)EOpCode::OP_SET_FIELD);
        EmitByte((uint8)(FieldNameIndex >> 8));
        EmitByte((uint8)(FieldNameIndex & 0xFF));
        return;
    }

    // Unknown/unsupported assignment target
    ReportError(TEXT("Invalid assignment target"));
}

void FScriptCompiler::CompileCall(FCallExpr* Expr)
{
    // Get function name
    if (Expr->Callee->GetNodeType() != TEXT("Identifier"))
    {
        ReportError(TEXT("Only direct function calls supported"));
        return;
    }
    
    FIdentifierExpr* Callee = static_cast<FIdentifierExpr*>(Expr->Callee.Get());
    FString FuncName = Callee->Name.Lexeme;
    
    // Check if this is a known native function
    bool bIsKnownNative = NativeFunctions.Contains(FuncName);
    
    // Check if this is a known void native function (for implicit return handling if needed)
    // For now, we just track if it was a void call to clean up stack
    static const TSet<FString> VoidNativeFunctions = {
        TEXT("Log"), TEXT("Print"),
        TEXT("UI_ShowSplashScreen"), TEXT("UI_ShowMenuScreen"), 
        TEXT("UI_ShowLoadingScreen"), TEXT("UI_ShowInGameHUD"),
        TEXT("UI_ShowPauseMenu"), TEXT("UI_HidePauseMenu"),
        TEXT("Audio_Stop"), TEXT("Audio_SetVolume"),
        TEXT("Audio_SetCategoryVolume"), TEXT("Audio_FadeOutAndStop")
    };
    
    bool bIsVoidFunction = VoidNativeFunctions.Contains(FuncName);
    
    // Compile arguments
    for (const auto& Arg : Expr->Arguments)
    {
        if (Arg.IsValid())
        {
            CompileExpression(Arg.Get());
        }
    }
    
    // Emit call instruction
    int32 FuncIndex = ResolveFunction(FuncName);
    if (FuncIndex >= 0)
    {
        // This is a user-defined function
        EmitBytes((uint8)EOpCode::OP_CALL, (uint8)Expr->Arguments.Num());
        EmitBytes((uint8)(FuncIndex >> 8), (uint8)(FuncIndex & 0xFF)); // Function index (2 bytes)
        
        // Check if user-defined function is void
        if (Functions.IsValidIndex(FuncIndex) && Functions[FuncIndex].ReturnType == EScriptType::VOID)
        {
            bIsVoidFunction = true;
        }
    }
    else
    {
        // Might be a native function - will handle in VM
        if (!bIsKnownNative)
        {
            SCRIPT_LOG_WARNING(FString::Printf(TEXT("Unknown function '%s' - assuming native"), *FuncName));
        }
        EmitBytes((uint8)EOpCode::OP_CALL_NATIVE, (uint8)Expr->Arguments.Num());
        int32 NameIndex = Chunk->AddConstant(FScriptValue::String(FuncName));
        EmitBytes((uint8)(NameIndex >> 8), (uint8)(NameIndex & 0xFF));
    }
    
    // Set flag if this is a void function
    bLastExpressionWasVoidCall = bIsVoidFunction;
}

void FScriptCompiler::CompileArrayLiteral(FArrayLiteralExpr* Expr)
{
    // Compile array literal: [elem1, elem2, ...]
    // We'll implement this as pushing elements onto the stack and then creating an array
    
    // Compile each element
    for (const auto& Element : Expr->Elements)
    {
        if (Element.IsValid())
        {
            CompileExpression(Element.Get());
        }
    }
    
    // Create array with the specified number of elements
    int32 ElementCount = Expr->Elements.Num();
    EmitByte((uint8)EOpCode::OP_CREATE_ARRAY);
    EmitByte((uint8)ElementCount);
}

void FScriptCompiler::CompileArrayAccess(FArrayAccessExpr* Expr)
{
    // Compile array access: arr[index] or obj.field
    
    // Compile the array/object expression
    CompileExpression(Expr->Array.Get());
    
    // Compile the index/expression access
    CompileExpression(Expr->Index.Get());
    
    // Emit array access instruction
    EmitByte((uint8)EOpCode::OP_GET_ELEMENT);
}

void FScriptCompiler::CompileArrayAssign(FArrayAssignExpr* Expr)
{
    // Compile array assignment: arr[index] = value
    
    // Compile the array expression
    CompileExpression(Expr->Array.Get());
    
    // Compile the index
    CompileExpression(Expr->Index.Get());
    
    // Compile the value
    CompileExpression(Expr->Value.Get());
    
    // Emit array assignment instruction
    EmitByte((uint8)EOpCode::OP_SET_ELEMENT);
}

void FScriptCompiler::CompileStructAccess(FStructAccessExpr* Expr)
{
    // Compile struct access: object.field
    
    // Compile the object
    CompileExpression(Expr->Object.Get());
    
    // Get field name as constant
    int32 FieldNameIndex = Chunk->AddConstant(FScriptValue::String(Expr->Field.Lexeme));
    
    // Emit struct access instruction
    EmitByte((uint8)EOpCode::OP_GET_FIELD);
    EmitByte((uint8)(FieldNameIndex >> 8));  // High byte
    EmitByte((uint8)(FieldNameIndex & 0xFF)); // Low byte
}

void FScriptCompiler::CompileStructAssign(FStructAssignExpr* Expr)
{
    // Compile struct assignment: object.field = value
    
    // Compile the object
    CompileExpression(Expr->Object.Get());
    
    // Get field name as constant
    int32 FieldNameIndex = Chunk->AddConstant(FScriptValue::String(Expr->Field.Lexeme));
    
    // Compile the value
    CompileExpression(Expr->Value.Get());
    
    // Emit struct assignment instruction
    EmitByte((uint8)EOpCode::OP_SET_FIELD);
    EmitByte((uint8)(FieldNameIndex >> 8));  // High byte
    EmitByte((uint8)(FieldNameIndex & 0xFF)); // Low byte
}

void FScriptCompiler::CompileSwitch(FSwitchStmt* Stmt)
{
    // Compile switch statement
    // For now, we'll translate to a series of if-else comparisons
    // Note: This is a simplified approach - a more efficient implementation would use jump tables
    
    // Store the switch expression value in a temporary local variable
    int32 TempVarIndex = AddLocal(TEXT("$switch_expr"), EScriptType::AUTO);
    
    // Compile the switch expression
    CompileExpression(Stmt->Expression.Get());
    
    // Assign the value to the temporary variable
    EmitByte((uint8)EOpCode::OP_SET_LOCAL);
    EmitByte((uint8)TempVarIndex);
    
    // Track whether any case matched to implement fallthrough behavior
    bool bAnyCaseMatched = false;
    
    // Process all cases
    for (int32 i = 0; i < Stmt->Cases.Num(); ++i)
    {
        auto& Case = Stmt->Cases[i];
        
        // Get the current offset for jump patching
        uint16 JumpIfExprNotEqual = 0;
        
        // Load the temporary variable value
        EmitByte((uint8)EOpCode::OP_GET_LOCAL);
        EmitByte((uint8)TempVarIndex);
        
        // Compile the case value to compare against
        CompileExpression(Case.Key.Get());
        
        // Compare for equality
        EmitByte((uint8)EOpCode::OP_EQUAL);
        
        // Jump to the next case if the values are not equal
        JumpIfExprNotEqual = EmitJump(EOpCode::OP_JUMP_IF_FALSE);
        
        // Pop the comparison result
        EmitByte((uint8)EOpCode::OP_POP);
        
        // Execute the case body
        CompileStatement(Case.Value.Get());
        
        // After a case is executed, we need to decide whether to break or continue
        // For now, we'll implement simple break behavior (no fallthrough)
        // In a full implementation, we would check for the presence of break statements
        uint16 JumpToEnd = EmitJump(EOpCode::OP_JUMP);
        
        // Patch the jump to go to the next case if the expression doesn't match this case
        PatchJump(JumpIfExprNotEqual);
        
        // Pop the comparison result after the jump
        EmitByte((uint8)EOpCode::OP_POP);
    }
    
    // Handle default case if exists
    if (Stmt->DefaultCase.IsValid())
    {
        // Execute default case body
        CompileStatement(Stmt->DefaultCase.Get());
    }
    
    // Patch all the jumps to end of switch
    // For simplicity of this example implementation
}

void FScriptCompiler::CompileTypeCast(FTypeCastExpr* Expr)
{
    CompileExpression(Expr->Expression.Get());
    
    EScriptType FromType = InferType(Expr->Expression.Get());
    EmitTypeConversion(FromType, Expr->TargetType);
}

//=============================================================================
// Helper Methods
//=============================================================================

void FScriptCompiler::EmitByte(uint8 Byte)
{
    Chunk->WriteByte(Byte, 0); // Line number tracking TODO
}

void FScriptCompiler::EmitBytes(uint8 Byte1, uint8 Byte2)
{
    EmitByte(Byte1);
    EmitByte(Byte2);
}

void FScriptCompiler::EmitReturn()
{
    EmitByte((uint8)EOpCode::OP_RETURN);
}

void FScriptCompiler::EmitConstant(const FScriptValue& Value)
{
    int32 ConstIndex = Chunk->AddConstant(Value);
    EmitBytes((uint8)EOpCode::OP_CONSTANT, (uint8)ConstIndex);
}

int32 FScriptCompiler::EmitJump(EOpCode JumpOp)
{
    EmitByte((uint8)JumpOp);
    EmitByte(0xFF); // Placeholder
    EmitByte(0xFF); // Placeholder
    return Chunk->Code.Num() - 2;
}

void FScriptCompiler::PatchJump(int32 Offset)
{
    int32 Jump = Chunk->Code.Num() - Offset - 2;
    
    if (Jump > 0xFFFF)
    {
        ReportError(TEXT("Jump offset too large"));
        return;
    }
    
    Chunk->Code[Offset] = (Jump >> 8) & 0xFF;
    Chunk->Code[Offset + 1] = Jump & 0xFF;
}

int32 FScriptCompiler::EmitLoop(int32 LoopStart)
{
    EmitByte((uint8)EOpCode::OP_LOOP);
    
    int32 Offset = Chunk->Code.Num() - LoopStart + 2;
    if (Offset > 0xFFFF)
    {
        ReportError(TEXT("Loop body too large"));
    }
    
    EmitByte((Offset >> 8) & 0xFF);
    EmitByte(Offset & 0xFF);
    
    return Chunk->Code.Num();
}

//=============================================================================
// Type System
//=============================================================================

EScriptType FScriptCompiler::InferType(FScriptExpression* Expr)
{
    if (!Expr)
    {
        return EScriptType::VOID;
    }
    
    // Use cached type if available
    if (Expr->InferredType != EScriptType::AUTO)
    {
        return Expr->InferredType;
    }
    
    FString NodeType = Expr->GetNodeType();
    
    if (NodeType == TEXT("Literal"))
    {
        FLiteralExpr* Lit = static_cast<FLiteralExpr*>(Expr);
        if (Lit->Token.Type == ETokenType::NUMBER)
        {
            return EScriptType::FLOAT; // Default to float for numbers
        }
        else if (Lit->Token.Type == ETokenType::STRING)
        {
            return EScriptType::STRING;
        }
        else if (Lit->Token.Type == ETokenType::KW_TRUE || Lit->Token.Type == ETokenType::KW_FALSE)
        {
            return EScriptType::BOOL;
        }
    }
    else if (NodeType == TEXT("Binary"))
    {
        FBinaryExpr* Bin = static_cast<FBinaryExpr*>(Expr);
        EScriptType LeftType = InferType(Bin->Left.Get());
        EScriptType RightType = InferType(Bin->Right.Get());
        
        // For arithmetic, promote to float if either operand is float
        if (LeftType == EScriptType::FLOAT || RightType == EScriptType::FLOAT)
        {
            return EScriptType::FLOAT;
        }
        return EScriptType::INT;
    }
    else if (NodeType == TEXT("Identifier"))
    {
        FIdentifierExpr* Ident = static_cast<FIdentifierExpr*>(Expr);
        int32 LocalIndex = ResolveLocal(Ident->Name.Lexeme);
        if (LocalIndex >= 0)
        {
            return Locals[LocalIndex].Type;
        }
    }
    
    return EScriptType::AUTO;
}

void FScriptCompiler::EmitTypeConversion(EScriptType From, EScriptType To)
{
    if (From == To || To == EScriptType::AUTO)
    {
        return; // No conversion needed
    }
    
    // Emit conversion opcodes based on From->To
    if (From == EScriptType::FLOAT && To == EScriptType::INT)
    {
        EmitByte((uint8)EOpCode::OP_CAST_INT);
    }
    else if (From == EScriptType::INT && To == EScriptType::FLOAT)
    {
        EmitByte((uint8)EOpCode::OP_CAST_FLOAT);
    }
    else if (To == EScriptType::STRING)
    {
        EmitByte((uint8)EOpCode::OP_CAST_STRING);
    }
    // Add more conversions as needed
}

bool FScriptCompiler::TypesCompatible(EScriptType A, EScriptType B)
{
    if (A == B) return true;
    if (A == EScriptType::AUTO || B == EScriptType::AUTO) return true;
    
    // Number types are compatible
    if ((A == EScriptType::INT || A == EScriptType::FLOAT) &&
        (B == EScriptType::INT || B == EScriptType::FLOAT))
    {
        return true;
    }
    
    return false;
}

