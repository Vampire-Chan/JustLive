// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#pragma once

#include "CoreMinimal.h"
#include "ScriptToken.h"
#include "ScriptLexer.h"
#include "ScriptAST.h"

/**
 * Recursive Descent Parser for SBS/SBSH Scripting Language
 * ========================================================
 * 
 * The Parser is the SECOND STAGE of the compilation pipeline.
 * It takes the stream of TOKENS from the Lexer and builds an Abstract Syntax Tree (AST).
 * The Parser UNDERSTANDS the GRAMMAR and SYNTAX of the SBS/SBSH language.
 * 
 * WHAT THE PARSER DOES:
 * --------------------
 * Input:  Array of tokens from Lexer (e.g., [INT, IDENTIFIER, EQUAL, NUMBER, ...])
 * Output: Abstract Syntax Tree (AST) - a structured representation of the program
 * 
 * The parser performs:
 * 1. Syntax validation - checks if tokens follow correct grammar rules
 * 2. AST construction - builds tree structure representing program logic
 * 3. Error detection - reports syntax errors with line/column information
 * 4. Error recovery - attempts to continue parsing after errors
 * 
 * SBS/SBSH GRAMMAR RULES:
 * ======================
 * 
 * PROGRAM STRUCTURE:
 * -----------------
 * Program → (FunctionDecl | Statement)*
 * 
 * A program consists of function declarations and global statements.
 * Example:
 *   int globalVar = 10;           // Global statement
 *   
 *   int Add(int a, int b) {       // Function declaration
 *       return a + b;
 *   }
 *   
 *   void Main() {                 // Entry point function
 *       Log("Hello World");
 *   }
 * 
 * FUNCTION DECLARATIONS:
 * ---------------------
 * FunctionDecl → Type Identifier "(" Parameters? ")" Block
 * Parameters   → Type Identifier ("," Type Identifier)*
 * Type         → "int" | "float" | "string" | "void" | Type"[]"
 * 
 * Examples:
 *   void PrintMessage(string msg) { ... }
 *   int Add(int a, int b) { ... }
 *   float[] GetCoordinates() { ... }
 *   int Sum(int[] numbers) { ... }
 * 
 * STATEMENTS:
 * ----------
 * Statement → VarDecl | ExprStmt | IfStmt | WhileStmt | ForStmt | 
 *             SwitchStmt | ReturnStmt | BreakStmt | ContinueStmt | Block
 * 
 * 1. Variable Declaration:
 *    VarDecl → Type Identifier ("=" Expression)? ";"
 *    
 *    Examples:
 *      int x;                    // Declaration only
 *      float y = 3.14;           // Declaration with initialization
 *      string name = "Player";   // String initialization
 *      int[] scores = {10, 20};  // Array initialization
 * 
 * 2. Expression Statement:
 *    ExprStmt → Expression ";"
 *    
 *    Examples:
 *      x = 10;                   // Assignment
 *      Log("Hello");             // Function call
 *      x = x + 1;                // Increment
 * 
 * 3. If Statement:
 *    IfStmt → "if" "(" Expression ")" Statement ("else" Statement)?
 *    
 *    Examples:
 *      if (x > 10) { Log("Big"); }
 *      if (x == 0) { Log("Zero"); } else { Log("Non-zero"); }
 *      if (x > 0) Log("Positive"); else if (x < 0) Log("Negative");
 * 
 * 4. While Loop:
 *    WhileStmt → "while" "(" Expression ")" Statement
 *    
 *    Example:
 *      while (i < 10) {
 *          Log("Count: " + i);
 *          i = i + 1;
 *      }
 * 
 * 5. For Loop:
 *    ForStmt → "for" "(" (VarDecl | ExprStmt | ";") Expression? ";" Expression? ")" Statement
 *    
 *    Example:
 *      for (int i = 0; i < 10; i = i + 1) {
 *          Log("Iteration: " + i);
 *      }
 * 
 * 6. Switch Statement:
 *    SwitchStmt → "switch" "(" Expression ")" "{" CaseClause* DefaultClause? "}"
 *    CaseClause → "case" Expression ":" Statement*
 *    DefaultClause → "default" ":" Statement*
 *    
 *    Example:
 *      switch (day) {
 *          case 1: Log("Monday"); break;
 *          case 2: Log("Tuesday"); break;
 *          default: Log("Other day"); break;
 *      }
 * 
 * 7. Return Statement:
 *    ReturnStmt → "return" Expression? ";"
 *    
 *    Examples:
 *      return;           // Return from void function
 *      return 42;        // Return value
 *      return x + y;     // Return expression
 * 
 * 8. Control Flow:
 *    BreakStmt    → "break" ";"
 *    ContinueStmt → "continue" ";"
 * 
 * 9. Block:
 *    Block → "{" Statement* "}"
 * 
 * EXPRESSIONS (Operator Precedence - highest to lowest):
 * ------------------------------------------------------
 * Expression → Assignment
 * 
 * Assignment     → Identifier "=" Assignment | LogicalOr
 * LogicalOr      → LogicalAnd ("||" LogicalAnd)*
 * LogicalAnd     → BitwiseOr ("&&" BitwiseOr)*
 * BitwiseOr      → BitwiseXor ("|" BitwiseXor)*
 * BitwiseXor     → BitwiseAnd ("^" BitwiseAnd)*
 * BitwiseAnd     → Equality ("&" Equality)*
 * Equality       → Comparison (("==" | "!=") Comparison)*
 * Comparison     → Term ((">" | ">=" | "<" | "<=") Term)*
 * Term           → Factor (("+" | "-") Factor)*
 * Factor         → Unary (("*" | "/" | "%") Unary)*
 * Unary          → ("!" | "-" | "~") Unary | Call
 * Call           → Primary ("(" Arguments? ")")*
 * Primary        → Literal | Identifier | ArrayLiteral | ArrayAccess | "(" Expression ")"
 * 
 * ARRAY OPERATIONS:
 * ----------------
 * ArrayLiteral → "[" (Expression ("," Expression)*)? "]" | "{" (Expression ("," Expression)*)? "}"
 * ArrayAccess  → Identifier "[" Expression "]"
 * 
 * Examples:
 *   int[] arr = {1, 2, 3, 4, 5};      // Array literal with braces
 *   int[] arr2 = [10, 20, 30];        // Array literal with brackets
 *   int x = arr[0];                    // Array access
 *   arr[1] = 100;                      // Array assignment
 * 
 * FUNCTION CALLS:
 * --------------
 * CallExpr → Identifier "(" Arguments? ")"
 * Arguments → Expression ("," Expression)*
 * 
 * Examples:
 *   Log("Hello");                      // Function call with one argument
 *   int sum = Add(10, 20);            // Function call with return value
 *   ProcessArray(scores, 5);          // Passing array as argument
 * 
 * TYPE CASTING:
 * ------------
 * Future syntax (not fully implemented):
 *   int x = (int)3.14;                 // Cast float to int
 *   string s = (string)42;             // Cast int to string
 * 
 * PARSING STRATEGY:
 * ----------------
 * This parser uses RECURSIVE DESCENT parsing technique:
 * - Each grammar rule becomes a parsing method
 * - Methods call each other recursively to build the AST
 * - Operator precedence is encoded in the method call hierarchy
 * - Error recovery uses synchronization points (semicolons, keywords)
 * 
 * NEVER returns invalid pointers - returns nullptr on error
 * Errors are collected in an error list for reporting to the user
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

