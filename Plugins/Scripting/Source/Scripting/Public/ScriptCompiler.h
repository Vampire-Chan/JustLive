// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#pragma once

#include "CoreMinimal.h"
#include "ScriptAST.h"
#include "ScriptBytecode.h"

/**
 * Compiles AST into bytecode
 * Performs type checking, variable resolution, and code generation
 */
class SCRIPTING_API FScriptCompiler
{
public:
    FScriptCompiler();
    
    /** Compile a program AST into bytecode */
    TSharedPtr<FBytecodeChunk> Compile(TSharedPtr<FScriptProgram> Program);
    
    /** Get compilation errors */
    const TArray<FString>& GetErrors() const { return Errors; }
    bool HasErrors() const { return Errors.Num() > 0; }

private:
    // Symbol table for variable tracking
    struct FLocal
    {
        FString Name;
        int32 Depth;      // Scope depth
        EScriptType Type;  // Variable type
        bool bInitialized; // Has been assigned
    };
    
    struct FFunction
    {
        FString Name;
        int32 Arity;      // Number of parameters
        int32 Address;    // Bytecode address
        EScriptType ReturnType;
    };
    
    struct FLoopContext
    {
        int32 Start;              // Loop start address
        TArray<int32> BreakJumps; // Addresses of break jumps to patch
    };
    
    TArray<FLocal> Locals;
    TArray<FFunction> Functions;
    TArray<FLoopContext> LoopStack;  // Track nested loops for break/continue
    TSet<FString> ImportedFiles;     // Track imported files to prevent circular imports
    int32 ScopeDepth;
    bool bLastExpressionWasVoidCall; // Track if last expression was a void function call
    
    // Known native functions (to suppress warnings)
    static const TSet<FString> NativeFunctions;

    TSharedPtr<FBytecodeChunk> Chunk;
    TArray<FString> Errors;
    
    // Error reporting
    void ReportError(const FString& Message);
    
    // Scope management
    void BeginScope();
    void EndScope();
    
    // Variable management
    int32 ResolveLocal(const FString& Name);
    int32 AddLocal(const FString& Name, EScriptType Type);
    int32 ResolveFunction(const FString& Name);
    
    // Compilation methods
    void CompileProgram(FScriptProgram* Program);
    void CompileFunction(FFunctionDecl* Function);
    void CompileStatement(FScriptStatement* Statement);
    void CompileExpression(FScriptExpression* Expression);
    
    // Statement compilation
    void CompileExprStmt(FExprStmt* Stmt);
    void CompileVarDecl(FVarDeclStmt* Stmt);
    void CompileBlock(FBlockStmt* Stmt);
    void CompileIf(FIfStmt* Stmt);
    void CompileWhile(FWhileStmt* Stmt);
    void CompileFor(FForStmt* Stmt);
    void CompileBreak(FBreakStmt* Stmt);
    void CompileContinue(FContinueStmt* Stmt);
    void CompileReturn(FReturnStmt* Stmt);
    void CompileImport(FImportStmt* Stmt);
    
    // Expression compilation
    void CompileLiteral(FLiteralExpr* Expr);
    void CompileBinary(FBinaryExpr* Expr);
    void CompileUnary(FUnaryExpr* Expr);
    void CompileIdentifier(FIdentifierExpr* Expr);
    void CompileAssign(FAssignExpr* Expr);
    void CompileCall(FCallExpr* Expr);
    void CompileArrayLiteral(FArrayLiteralExpr* Expr);
    void CompileArrayAccess(FArrayAccessExpr* Expr);
    void CompileArrayAssign(FArrayAssignExpr* Expr);
    void CompileStructAccess(FStructAccessExpr* Expr);
    void CompileStructAssign(FStructAssignExpr* Expr);
    void CompileSwitch(FSwitchStmt* Stmt);
    void CompileTypeCast(FTypeCastExpr* Expr);
    
    // Helper methods
    void EmitByte(uint8 Byte);
    void EmitBytes(uint8 Byte1, uint8 Byte2);
    void EmitReturn();
    void EmitConstant(const FScriptValue& Value);
    int32 EmitJump(EOpCode JumpOp);
    void PatchJump(int32 Offset);
    int32 EmitLoop(int32 LoopStart);
    
    // Type checking
    EScriptType InferType(FScriptExpression* Expr);
    void EmitTypeConversion(EScriptType From, EScriptType To);
    bool TypesCompatible(EScriptType A, EScriptType B);
};

