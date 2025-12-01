// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#include "ScriptLexer.h"
#include "ScriptLogger.h"

FScriptLexer::FScriptLexer(const FString& InSource)
    : Source(InSource)
    , Start(0)
    , Current(0)
    , Line(1)
    , Column(1)
{
}

TArray<FScriptToken> FScriptLexer::ScanTokens()
{
    Tokens.Empty();
    Errors.Empty();
    
    while (!IsAtEnd())
    {
        Start = Current;
        ScanToken();
    }
    
    // Add EOF token
    Tokens.Add(FScriptToken(ETokenType::END_OF_FILE, TEXT(""), Line, Column));
    
    return Tokens;
}

bool FScriptLexer::IsAtEnd() const
{
    return Current >= Source.Len();
}

char FScriptLexer::Advance()
{
    if (IsAtEnd()) return '\0';
    
    char C = Source[Current++];
    Column++;
    
    if (C == '\n')
    {
        Line++;
        Column = 1;
    }
    
    return C;
}

char FScriptLexer::Peek() const
{
    if (IsAtEnd()) return '\0';
    return Source[Current];
}

char FScriptLexer::PeekNext() const
{
    if (Current + 1 >= Source.Len()) return '\0';
    return Source[Current + 1];
}

bool FScriptLexer::Match(char Expected)
{
    if (IsAtEnd()) return false;
    if (Source[Current] != Expected) return false;
    
    Current++;
    Column++;
    return true;
}

void FScriptLexer::ScanToken()
{
    char C = Advance();
    
    switch (C)
    {
        // Single character tokens
        case '(': AddToken(ETokenType::LEFT_PAREN); break;
        case ')': AddToken(ETokenType::RIGHT_PAREN); break;
        case '{': AddToken(ETokenType::LEFT_BRACE); break;
        case '}': AddToken(ETokenType::RIGHT_BRACE); break;
        case '[': AddToken(ETokenType::LEFT_BRACKET); break;
        case ']': AddToken(ETokenType::RIGHT_BRACKET); break;
        case ',': AddToken(ETokenType::COMMA); break;
        case '.': AddToken(ETokenType::DOT); break;
        case ':': AddToken(ETokenType::COLON); break;
        case '-': AddToken(ETokenType::MINUS); break;
        case '+': AddToken(ETokenType::PLUS); break;
        case ';': AddToken(ETokenType::SEMICOLON); break;
        case '*': AddToken(ETokenType::STAR); break;
        case '%': AddToken(ETokenType::PERCENT); break;
        case '^': AddToken(ETokenType::CARET); break;
        case '~': AddToken(ETokenType::TILDE); break;
        
        // Two character tokens
        case '!': AddToken(Match('=') ? ETokenType::BANG_EQUAL : ETokenType::BANG); break;
        case '=': AddToken(Match('=') ? ETokenType::EQUAL_EQUAL : ETokenType::EQUAL); break;
        case '<': AddToken(Match('=') ? ETokenType::LESS_EQUAL : ETokenType::LESS); break;
        case '>': AddToken(Match('=') ? ETokenType::GREATER_EQUAL : ETokenType::GREATER); break;
        case '&': AddToken(Match('&') ? ETokenType::AMPERSAND_AMPERSAND : ETokenType::AMPERSAND); break;
        case '|': AddToken(Match('|') ? ETokenType::PIPE_PIPE : ETokenType::PIPE); break;
        
        // Slash or comment
        case '/':
            if (Match('/'))
            {
                // Comment until end of line
                while (Peek() != '\n' && !IsAtEnd()) Advance();
            }
            else if (Match('*'))
            {
                // Multi-line comment
                while (!IsAtEnd())
                {
                    if (Peek() == '*' && PeekNext() == '/')
                    {
                        Advance(); // *
                        Advance(); // /
                        break;
                    }
                    Advance();
                }
            }
            else
            {
                AddToken(ETokenType::SLASH);
            }
            break;
        
        // Whitespace
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            // Ignore whitespace
            break;
        
        // String literals
        case '"':
            ScanString();
            break;
        
        default:
            if (IsDigit(C))
            {
                ScanNumber();
            }
            else if (IsAlpha(C))
            {
                ScanIdentifier();
            }
            else
            {
                ReportError(FString::Printf(TEXT("Unexpected character: '%c'"), C));
            }
            break;
    }
}

void FScriptLexer::AddToken(ETokenType Type)
{
    FString Text = Source.Mid(Start, Current - Start);
    AddToken(Type, Text);
}

void FScriptLexer::AddToken(ETokenType Type, const FString& Lexeme)
{
    FScriptToken Token(Type, Lexeme, Line, Column - Lexeme.Len());
    
    // Store number value for number tokens
    if (Type == ETokenType::NUMBER)
    {
        Token.NumberValue = FCString::Atod(*Lexeme);
    }
    
    Tokens.Add(Token);
}

void FScriptLexer::ScanString()
{
    FString StringValue;
    
    while (Peek() != '"' && !IsAtEnd())
    {
        char C = Advance();
        
        // Handle escape sequences
        if (C == '\\' && !IsAtEnd())
        {
            char Next = Advance();
            switch (Next)
            {
                case 'n': StringValue.AppendChar('\n'); break;
                case 't': StringValue.AppendChar('\t'); break;
                case 'r': StringValue.AppendChar('\r'); break;
                case '\\': StringValue.AppendChar('\\'); break;
                case '"': StringValue.AppendChar('"'); break;
                default:
                    StringValue.AppendChar(C);
                    StringValue.AppendChar(Next);
                    break;
            }
        }
        else
        {
            StringValue.AppendChar(C);
        }
    }
    
    if (IsAtEnd())
    {
        ReportError(TEXT("Unterminated string"));
        return;
    }
    
    // Closing "
    Advance();
    
    AddToken(ETokenType::STRING, StringValue);
}

void FScriptLexer::ScanNumber()
{
    while (IsDigit(Peek())) Advance();
    
    // Look for fractional part
    if (Peek() == '.' && IsDigit(PeekNext()))
    {
        Advance(); // Consume '.'
        while (IsDigit(Peek())) Advance();
    }
    
    AddToken(ETokenType::NUMBER);
}

void FScriptLexer::ScanIdentifier()
{
    while (IsAlphaNumeric(Peek())) Advance();
    
    FString Text = Source.Mid(Start, Current - Start);
    ETokenType Type = GetKeywordType(Text);
    
    AddToken(Type, Text);
}

void FScriptLexer::ReportError(const FString& Message)
{
    FString ErrorMsg = FString::Printf(TEXT("[Line %d, Col %d] Error: %s"), Line, Column, *Message);
    Errors.Add(ErrorMsg);
    SCRIPT_LOG_ERROR(FString::Printf(TEXT("Lexer: %s"), *ErrorMsg));
}

bool FScriptLexer::IsDigit(char C)
{
    return C >= '0' && C <= '9';
}

bool FScriptLexer::IsAlpha(char C)
{
    return (C >= 'a' && C <= 'z') ||
           (C >= 'A' && C <= 'Z') ||
           C == '_';
}

bool FScriptLexer::IsAlphaNumeric(char C)
{
    return IsAlpha(C) || IsDigit(C);
}

ETokenType FScriptLexer::GetKeywordType(const FString& Text)
{
    static TMap<FString, ETokenType> Keywords = {
        { TEXT("and"), ETokenType::AND },
        { TEXT("break"), ETokenType::BREAK },
        { TEXT("case"), ETokenType::CASE },
        { TEXT("class"), ETokenType::CLASS },
        { TEXT("const"), ETokenType::CONST },
        { TEXT("continue"), ETokenType::CONTINUE },
        { TEXT("default"), ETokenType::DEFAULT },
        { TEXT("do"), ETokenType::DO },
        { TEXT("else"), ETokenType::ELSE },
        { TEXT("enum"), ETokenType::ENUM },
        { TEXT("false"), ETokenType::KW_FALSE },
        { TEXT("float"), ETokenType::FLOAT },
        { TEXT("for"), ETokenType::FOR },
        { TEXT("function"), ETokenType::FUNCTION },
        { TEXT("if"), ETokenType::IF },
        { TEXT("import"), ETokenType::IMPORT },
        { TEXT("int"), ETokenType::INT },
        { TEXT("nil"), ETokenType::NIL },
        { TEXT("null"), ETokenType::NIL },
        { TEXT("or"), ETokenType::OR },
        { TEXT("print"), ETokenType::PRINT },
        { TEXT("return"), ETokenType::RETURN },
        { TEXT("string"), ETokenType::STRING_TYPE },
        { TEXT("struct"), ETokenType::STRUCT },
        { TEXT("super"), ETokenType::SUPER },
        { TEXT("switch"), ETokenType::SWITCH },
        { TEXT("this"), ETokenType::THIS },
        { TEXT("true"), ETokenType::KW_TRUE },
        { TEXT("typedef"), ETokenType::TYPEDEF },
        { TEXT("var"), ETokenType::VAR },
        { TEXT("void"), ETokenType::VOID },
        { TEXT("while"), ETokenType::WHILE }
    };
    
    if (const ETokenType* Type = Keywords.Find(Text))
    {
        return *Type;
    }
    
    return ETokenType::IDENTIFIER;
}

