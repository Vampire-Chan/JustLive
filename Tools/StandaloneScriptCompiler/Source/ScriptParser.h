// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#pragma once

#include "Platform.h"
#include "ScriptToken.h"
#include "ScriptLexer.h"
#include "ScriptAST.h"

/**
 * Recursive descent parser with robust error recovery
 * NEVER returns invalid pointers - returns nullptr on error
 */
class SCRIPTING_API FScriptParser
{
public:
    FScriptParser(const TArray<FScriptToken>& InTokens);
    
    /** Parse the entire program */
    TSharedPtr<FScriptProgram> Parse();
    
    /** Get all error messages */
    const TArray<FString>& GetErrors() const { return Errors; }
    
    /** Check if there were any errors */
    bool HasErrors() const { return Errors.Num() > 0; }

private:
    TArray<FScriptToken> Tokens;
    int32 Current;
    TArray<FString> Errors;
    bool bPanicMode; // For error recovery
    
    // Utility methods
    FScriptToken Peek() const;
    FScriptToken Previous() const;
    FScriptToken Advance();
    bool IsAtEnd() const;
    bool Check(ETokenType Type) const;
    bool Match(ETokenType Type);
    bool Match(const TArray<ETokenType>& Types);
    
    // Consume with error reporting
    bool Consume(ETokenType Type, const FString& Message);
    
    // Error handling
    void ReportError(const FString& Message);
    void Synchronize(); // Recover from error
    
    // Parsing methods (all return nullptr on error)
    TSharedPtr<FScriptASTNode> ParseDeclaration();
    TSharedPtr<FFunctionDecl> ParseFunction();
    TSharedPtr<FFunctionDecl> ParseFunctionWithReturnType(EScriptType ReturnType);
    TSharedPtr<FScriptStatement> ParseStatement();
    TSharedPtr<FVarDeclStmt> ParseVarDeclaration();
    TSharedPtr<FScriptStatement> ParseExpressionStatement();
    TSharedPtr<FBlockStmt> ParseBlock();
    TSharedPtr<FIfStmt> ParseIfStatement();
    TSharedPtr<FWhileStmt> ParseWhileStatement();
    TSharedPtr<FScriptStatement> ParseSwitchStatement();
    TSharedPtr<FScriptStatement> ParseForStatement();
    TSharedPtr<FReturnStmt> ParseReturnStatement();
    TSharedPtr<FBreakStmt> ParseBreakStatement();
    TSharedPtr<FContinueStmt> ParseContinueStatement();
    
    // Expression parsing (precedence climbing)
    TSharedPtr<FScriptExpression> ParseExpression();
    TSharedPtr<FScriptExpression> ParseAssignment();
    TSharedPtr<FScriptExpression> ParseLogicalOr();
    TSharedPtr<FScriptExpression> ParseLogicalAnd();
    TSharedPtr<FScriptExpression> ParseBitwiseOr();
    TSharedPtr<FScriptExpression> ParseBitwiseXor();
    TSharedPtr<FScriptExpression> ParseBitwiseAnd();
    TSharedPtr<FScriptExpression> ParseEquality();
    TSharedPtr<FScriptExpression> ParseComparison();
    TSharedPtr<FScriptExpression> ParseTerm();
    TSharedPtr<FScriptExpression> ParseFactor();
    TSharedPtr<FScriptExpression> ParseUnary();
    TSharedPtr<FScriptExpression> ParseCall();
    TSharedPtr<FScriptExpression> ParsePrimary();
    TSharedPtr<FScriptExpression> ParseArrayLiteral();
    TSharedPtr<FScriptExpression> ParseArrayAccess();
    
    // Helper for function calls
    TSharedPtr<FScriptExpression> FinishCall(TSharedPtr<FScriptExpression> Callee);
    
    // Helper to convert token type to script type
    EScriptType GetTypeFromToken(const FScriptToken& Token);
};

