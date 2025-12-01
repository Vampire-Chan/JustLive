// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#pragma once

#include "CoreMinimal.h"
#include "ScriptToken.h"

/**
 * Lexer/Scanner for the SBS/SBSH scripting language
 * ==================================================
 * 
 * The Lexer (also called Scanner or Tokenizer) is the FIRST STAGE of the compilation pipeline.
 * It reads raw source code text and breaks it down into meaningful chunks called TOKENS.
 * 
 * WHAT THE LEXER DOES:
 * -------------------
 * Input:  Raw text source code (e.g., "int x = 10 + 20;")
 * Output: Array of tokens (e.g., [INT, IDENTIFIER("x"), EQUAL, NUMBER("10"), PLUS, NUMBER("20"), SEMICOLON])
 * 
 * The lexer performs:
 * 1. Character-by-character scanning of source code
 * 2. Recognition of keywords, identifiers, operators, literals, and punctuation
 * 3. Skipping whitespace and comments
 * 4. Tracking line/column numbers for error reporting
 * 5. Detecting lexical errors (e.g., unterminated strings, invalid characters)
 * 
 * SBS/SBSH LANGUAGE SYNTAX EXPECTATIONS:
 * =====================================
 * 
 * COMMENTS:
 *   // Single-line comment
 *   /* Multi-line comment * /
 * 
 * DATA TYPES:
 *   int      - Integer numbers
 *   float    - Floating point numbers
 *   string   - Text strings in double quotes
 *   bool     - true/false values
 *   void     - No return value (for functions)
 * 
 * ARRAYS:
 *   int[]    - Array of integers
 *   float[]  - Array of floats
 *   string[] - Array of strings
 * 
 * OPERATORS:
 *   Arithmetic: + - * / %
 *   Comparison: == != < > <= >=
 *   Logical:    && || !
 *   Bitwise:    & | ^ ~ 
 *   Assignment: =
 * 
 * PUNCTUATION:
 *   () [] {}  - Parentheses, brackets, braces
 *   ; , .     - Semicolon, comma, dot
 *   :         - Colon (for future use)
 * 
 * LITERALS:
 *   123       - Integer literal
 *   3.14      - Float literal
 *   "hello"   - String literal
 *   true/false- Boolean literals
 * 
 * IDENTIFIERS:
 *   Must start with letter or underscore: myVar, _internal, Player1
 *   Can contain letters, digits, underscores: health_points, maxHP_100
 * 
 * KEYWORDS (Reserved words that cannot be used as identifiers):
 *   Control flow: if, else, while, for, do, switch, case, default, break, continue
 *   Functions:    return, void, function
 *   Variables:    int, float, string, var, const
 *   Future use:   struct, class, enum, typedef, import, public, private
 * 
 * EXAMPLE INPUT/OUTPUT:
 * --------------------
 * 
 * Source Code:
 *   int Add(int a, int b) {
 *       return a + b;
 *   }
 * 
 * Tokens Generated:
 *   INT, IDENTIFIER("Add"), LEFT_PAREN, INT, IDENTIFIER("a"), COMMA, 
 *   INT, IDENTIFIER("b"), RIGHT_PAREN, LEFT_BRACE, RETURN, IDENTIFIER("a"),
 *   PLUS, IDENTIFIER("b"), SEMICOLON, RIGHT_BRACE
 * 
 * ERROR DETECTION EXAMPLES:
 * ------------------------
 * - Unterminated string: "hello
 * - Invalid character: int x @ 10;
 * - Invalid number format: 3.14.15
 */
class SCRIPTING_API FScriptLexer
{
public:
    FScriptLexer(const FString& InSource);
    
    /** Tokenize the entire source code */
    TArray<FScriptToken> ScanTokens();
    
    /** Get all error messages */
    const TArray<FString>& GetErrors() const { return Errors; }
    
    /** Check if there were any errors */
    bool HasErrors() const { return Errors.Num() > 0; }

private:
    FString Source;
    TArray<FScriptToken> Tokens;
    TArray<FString> Errors;
    
    int32 Start;      // Start of current token
    int32 Current;    // Current character being scanned
    int32 Line;       // Current line number
    int32 Column;     // Current column number
    
    // Helper methods
    bool IsAtEnd() const;
    char Advance();
    char Peek() const;
    char PeekNext() const;
    bool Match(char Expected);
    
    void ScanToken();
    void AddToken(ETokenType Type);
    void AddToken(ETokenType Type, const FString& Lexeme);
    void ScanString();
    void ScanNumber();
    void ScanIdentifier();
    
    void ReportError(const FString& Message);
    
    static bool IsDigit(char C);
    static bool IsAlpha(char C);
    static bool IsAlphaNumeric(char C);
    static ETokenType GetKeywordType(const FString& Text);
};

