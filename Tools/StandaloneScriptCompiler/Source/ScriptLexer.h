// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#pragma once

#include "Platform.h"
#include "ScriptToken.h"

/**
 * Lexer/Scanner for the scripting language
 * Converts source code into tokens
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

