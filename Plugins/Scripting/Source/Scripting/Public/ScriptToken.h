// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h" // Required for UENUM()

/**
 * Token types for the scripting language
 */
UENUM()
enum class ETokenType : uint8
{
    // Single-character tokens
    LEFT_PAREN,     // (
    RIGHT_PAREN,    // )
    LEFT_BRACE,     // {
    RIGHT_BRACE,    // }
    LEFT_BRACKET,   // [
    RIGHT_BRACKET,  // ]
    COMMA,          // ,
    DOT,            // .
    COLON,          // :
    MINUS,          // -
    PLUS,           // +
    SEMICOLON,      // ;
    SLASH,          // /
    STAR,           // *
    PERCENT,        // %
    CARET,          // ^  (XOR)
    TILDE,          // ~  (bitwise NOT)
    
    // One or two character tokens
    BANG,           // !
    BANG_EQUAL,     // !=
    EQUAL,          // =
    EQUAL_EQUAL,    // ==
    GREATER,        // >
    GREATER_EQUAL,  // >=
    LESS,           // <
    LESS_EQUAL,     // <=
    AMPERSAND,      // &  (bitwise AND)
    AMPERSAND_AMPERSAND, // && (logical AND)
    PIPE,           // |  (bitwise OR)
    PIPE_PIPE,      // || (logical OR)
    
    // Literals
    IDENTIFIER,
    STRING,
    NUMBER,
    
    // Keywords
    AND,            // and
    BREAK,          // break
    CASE,           // case
    CLASS,          // class
    CONST,          // const
    CONTINUE,       // continue
    DEFAULT,        // default
    DO,             // do
    ELSE,           // else
    ENUM,           // enum
    KW_FALSE,       // false (can't use FALSE - UHT restriction)
    FLOAT,          // float
    FOR,            // for
    FUNCTION,       // function
    IF,             // if
    IMPORT,         // import (for includes)
    INT,            // int
    NIL,            // nil/null
    OR,             // or
    PRINT,          // print
    RETURN,         // return
    STRING_TYPE,    // string (STRING is taken by literal)
    STRUCT,         // struct
    SUPER,          // super
    SWITCH,         // switch
    THIS,           // this
    KW_TRUE,        // true (can't use TRUE - UHT restriction)
    TYPEDEF,        // typedef
    VAR,            // var
    VOID,           // void
    WHILE,          // while
    
    // Special
    ERROR,
    END_OF_FILE,
    
    // Additional tokens for struct/array support
    ARROW,            // -> (for later use)
    PUBLIC,           // public (in structs)
    PRIVATE           // private (in structs)
};

/**
 * Represents a single token in the source code
 */
struct FScriptToken
{
    ETokenType Type;
    FString Lexeme;
    int32 Line;
    int32 Column;
    
    // For number literals
    double NumberValue;
    
    FScriptToken()
        : Type(ETokenType::ERROR)
        , Line(0)
        , Column(0)
        , NumberValue(0.0)
    {}
    
    FScriptToken(ETokenType InType, const FString& InLexeme, int32 InLine, int32 InColumn)
        : Type(InType)
        , Lexeme(InLexeme)
        , Line(InLine)
        , Column(InColumn)
        , NumberValue(0.0)
    {}
    
    FString ToString() const
    {
        return FString::Printf(TEXT("[%d:%d] %s '%s'"), 
            Line, Column, *GetTokenTypeName(), *Lexeme);
    }
    
    FString GetTokenTypeName() const
    {
        switch (Type)
        {
            case ETokenType::LEFT_PAREN: return TEXT("LEFT_PAREN");
            case ETokenType::RIGHT_PAREN: return TEXT("RIGHT_PAREN");
            case ETokenType::LEFT_BRACE: return TEXT("LEFT_BRACE");
            case ETokenType::RIGHT_BRACE: return TEXT("RIGHT_BRACE");
            case ETokenType::LEFT_BRACKET: return TEXT("LEFT_BRACKET");
            case ETokenType::RIGHT_BRACKET: return TEXT("RIGHT_BRACKET");
            case ETokenType::COMMA: return TEXT("COMMA");
            case ETokenType::DOT: return TEXT("DOT");
            case ETokenType::COLON: return TEXT("COLON");
            case ETokenType::MINUS: return TEXT("MINUS");
            case ETokenType::PLUS: return TEXT("PLUS");
            case ETokenType::SEMICOLON: return TEXT("SEMICOLON");
            case ETokenType::SLASH: return TEXT("SLASH");
            case ETokenType::STAR: return TEXT("STAR");
            case ETokenType::BANG: return TEXT("BANG");
            case ETokenType::BANG_EQUAL: return TEXT("BANG_EQUAL");
            case ETokenType::EQUAL: return TEXT("EQUAL");
            case ETokenType::EQUAL_EQUAL: return TEXT("EQUAL_EQUAL");
            case ETokenType::GREATER: return TEXT("GREATER");
            case ETokenType::GREATER_EQUAL: return TEXT("GREATER_EQUAL");
            case ETokenType::LESS: return TEXT("LESS");
            case ETokenType::LESS_EQUAL: return TEXT("LESS_EQUAL");
            case ETokenType::IDENTIFIER: return TEXT("IDENTIFIER");
            case ETokenType::STRING: return TEXT("STRING");
            case ETokenType::NUMBER: return TEXT("NUMBER");
            case ETokenType::AND: return TEXT("AND");
            case ETokenType::BREAK: return TEXT("BREAK");
            case ETokenType::CASE: return TEXT("CASE");
            case ETokenType::CLASS: return TEXT("CLASS");
            case ETokenType::CONST: return TEXT("CONST");
            case ETokenType::CONTINUE: return TEXT("CONTINUE");
            case ETokenType::DEFAULT: return TEXT("DEFAULT");
            case ETokenType::DO: return TEXT("DO");
            case ETokenType::ELSE: return TEXT("ELSE");
            case ETokenType::ENUM: return TEXT("ENUM");
            case ETokenType::KW_FALSE: return TEXT("FALSE");
            case ETokenType::FLOAT: return TEXT("FLOAT");
            case ETokenType::FOR: return TEXT("FOR");
            case ETokenType::FUNCTION: return TEXT("FUNCTION");
            case ETokenType::IF: return TEXT("IF");
            case ETokenType::IMPORT: return TEXT("IMPORT");
            case ETokenType::INT: return TEXT("INT");
            case ETokenType::NIL: return TEXT("NIL");
            case ETokenType::OR: return TEXT("OR");
            case ETokenType::PRINT: return TEXT("PRINT");
            case ETokenType::RETURN: return TEXT("RETURN");
            case ETokenType::STRING_TYPE: return TEXT("STRING_TYPE");
            case ETokenType::STRUCT: return TEXT("STRUCT");
            case ETokenType::SUPER: return TEXT("SUPER");
            case ETokenType::SWITCH: return TEXT("SWITCH");
            case ETokenType::THIS: return TEXT("THIS");
            case ETokenType::KW_TRUE: return TEXT("TRUE");
            case ETokenType::TYPEDEF: return TEXT("TYPEDEF");
            case ETokenType::VAR: return TEXT("VAR");
            case ETokenType::VOID: return TEXT("VOID");
            case ETokenType::WHILE: return TEXT("WHILE");
            case ETokenType::ERROR: return TEXT("ERROR");
            case ETokenType::END_OF_FILE: return TEXT("END_OF_FILE");
            case ETokenType::ARROW: return TEXT("ARROW");
            case ETokenType::PUBLIC: return TEXT("PUBLIC");
            case ETokenType::PRIVATE: return TEXT("PRIVATE");
            default: return TEXT("UNKNOWN");
        }
    }
};

