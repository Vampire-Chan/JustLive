// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#include "ScriptParser.h"
#include "ScriptLogger.h"

FScriptParser::FScriptParser(const TArray<FScriptToken>& InTokens)
    : Tokens(InTokens), Current(0), bPanicMode(false)
{}

TSharedPtr<FScriptProgram> FScriptParser::Parse()
{
    TArray<TSharedPtr<FFunctionDecl>> Functions;
    TArray<TSharedPtr<FScriptStatement>> Statements;
    
    while (!IsAtEnd())
    {
        TSharedPtr<FScriptASTNode> Decl = ParseDeclaration();
        if (Decl.IsValid())
        {
            // Check if it's a function declaration
            if (Decl->GetNodeType() == TEXT("Function"))
            {
                Functions.Add(StaticCastSharedPtr<FFunctionDecl>(Decl));
            }
            else
            {
                Statements.Add(StaticCastSharedPtr<FScriptStatement>(Decl));
            }
        }
    }
    
    return MakeShared<FScriptProgram>(Functions, Statements);
}

//=============================================================================
// Utility Methods
//=============================================================================

FScriptToken FScriptParser::Peek() const
{
    if (Current < Tokens.Num())
    {
        return Tokens[Current];
    }
    return Tokens.Last(); // Return EOF
}

FScriptToken FScriptParser::Previous() const
{
    if (Current > 0 && Current - 1 < Tokens.Num())
    {
        return Tokens[Current - 1];
    }
    return Tokens.Num() > 0 ? Tokens[0] : FScriptToken();
}

FScriptToken FScriptParser::Advance()
{
    if (!IsAtEnd())
    {
        Current++;
    }
    return Previous();
}

bool FScriptParser::IsAtEnd() const
{
    return Current >= Tokens.Num() || Peek().Type == ETokenType::END_OF_FILE;
}

bool FScriptParser::Check(ETokenType Type) const
{
    if (IsAtEnd()) return false;
    return Peek().Type == Type;
}

bool FScriptParser::Match(ETokenType Type)
{
    if (Check(Type))
    {
        Advance();
        return true;
    }
    return false;
}

TSharedPtr<FScriptExpression> FScriptParser::ParseArrayLiteral()
{
    if (!Check(ETokenType::LEFT_BRACKET))
    {
        return nullptr;
    }
    
    FScriptToken StartBracket = Advance(); // Consume '['
    
    TArray<TSharedPtr<FScriptExpression>> Elements;
    
    // Check for empty array []
    if (Check(ETokenType::RIGHT_BRACKET))
    {
        Advance(); // Consume ']'
        return MakeShared<FArrayLiteralExpr>(Elements);
    }
    
    // Parse first element
    TSharedPtr<FScriptExpression> FirstElement = ParseExpression();
    if (!FirstElement.IsValid())
    {
        ReportError(TEXT("Expected array element"));
        Synchronize();
        return nullptr;
    }
    Elements.Add(FirstElement);
    
    // Parse additional elements
    while (Match(ETokenType::COMMA))
    {
        TSharedPtr<FScriptExpression> Element = ParseExpression();
        if (!Element.IsValid())
        {
            ReportError(TEXT("Expected array element after comma"));
            Synchronize();
            return nullptr;
        }
        Elements.Add(Element);
    }
    
    if (!Consume(ETokenType::RIGHT_BRACKET, TEXT("Expected ']' after array elements")))
    {
        Synchronize();
        return nullptr;
    }
    
    return MakeShared<FArrayLiteralExpr>(Elements);
}

TSharedPtr<FScriptExpression> FScriptParser::ParseArrayAccess()
{
    TSharedPtr<FScriptExpression> Expr = ParsePrimary();
    if (!Expr.IsValid()) return nullptr;
    
    while (true)
    {
        if (Match(ETokenType::LEFT_BRACKET))
        {
            TSharedPtr<FScriptExpression> Index = ParseExpression();
            
            if (!Consume(ETokenType::RIGHT_BRACKET, TEXT("Expected ']' after array index")))
            {
                Synchronize();
                return nullptr;
            }
            
            Expr = MakeShared<FArrayAccessExpr>(Expr, Index);
        }
        else if (Match(ETokenType::DOT))
        {
            if (!Check(ETokenType::IDENTIFIER))
            {
                ReportError(TEXT("Expected property name after '.'"));
                Synchronize();
                return nullptr;
            }
            
            FScriptToken PropertyName = Advance();
            Expr = MakeShared<FStructAccessExpr>(Expr, PropertyName);
        }
        else
        {
            break;
        }
    }
    
    return Expr;
}

bool FScriptParser::Match(const TArray<ETokenType>& Types)
{
    for (ETokenType Type : Types)
    {
        if (Check(Type))
        {
            Advance();
            return true;
        }
    }
    return false;
}

bool FScriptParser::Consume(ETokenType Type, const FString& Message)
{
    if (Check(Type))
    {
        Advance();
        return true;
    }
    
    ReportError(Message);
    return false;
}

void FScriptParser::ReportError(const FString& Message)
{
    if (bPanicMode) return; // Don't spam errors
    
    FScriptToken Token = Peek();
    FString ErrorMsg = FString::Printf(TEXT("[Line %d] Error at '%s': %s"),
        Token.Line, *Token.Lexeme, *Message);
    
    Errors.Add(ErrorMsg);
    SCRIPT_LOG_ERROR(FString::Printf(TEXT("Parser Error: %s"), *ErrorMsg));
    bPanicMode = true;
}

void FScriptParser::Synchronize()
{
    bPanicMode = false;
    Advance();
    
    while (!IsAtEnd())
    {
        if (Previous().Type == ETokenType::SEMICOLON) return;
        
        switch (Peek().Type)
        {
            case ETokenType::FUNCTION:
            case ETokenType::VAR:
            case ETokenType::IF:
            case ETokenType::WHILE:
            case ETokenType::FOR:
            case ETokenType::RETURN:
            case ETokenType::INT:
            case ETokenType::FLOAT:
            case ETokenType::VOID:
            case ETokenType::IMPORT:
                return;
            default:
                break;
        }
        
        Advance();
    }
}

//=============================================================================
// Declaration Parsing
//=============================================================================

TSharedPtr<FScriptASTNode> FScriptParser::ParseDeclaration()
{
    if (Match(ETokenType::FUNCTION))
    {
        return ParseFunction();
    }
    
    if (Match(ETokenType::IMPORT))
    {
        // import "header.sc";
        if (!Check(ETokenType::STRING))
        {
            ReportError(TEXT("Expected string literal after 'import'"));
            Synchronize();
            return nullptr;
        }
        
        FScriptToken Path = Advance();
        Consume(ETokenType::SEMICOLON, TEXT("Expected ';' after import statement"));
        
        return MakeShared<FImportStmt>(Path);
    }
    
    // Type declarations: int x = 10; float y; OR int Add(int a, int b) {}
    TArray<ETokenType> TypeTokens = {
        ETokenType::INT, ETokenType::FLOAT, ETokenType::STRING_TYPE,
        ETokenType::VOID, ETokenType::VAR
    };
    
    if (Match(TypeTokens))
    {
        EScriptType ReturnType = GetTypeFromToken(Previous());
        
        // Look ahead to determine if this is a function or variable declaration
        // We need to check for [] and identifier and (
        // Function: type[] identifier ( ... OR type identifier ( ...
        // Variable: type[] identifier = ... OR type identifier = ...
        
        int SavedCurrent = Current;
        bool bHasArrayBrackets = false;
        
        // Check for optional []
        if (Check(ETokenType::LEFT_BRACKET))
        {
            Advance(); // [
            if (Check(ETokenType::RIGHT_BRACKET))
            {
                Advance(); // ]
                bHasArrayBrackets = true;
                
                // Convert to array type for function return type checking
                switch (ReturnType)
                {
                    case EScriptType::INT: ReturnType = EScriptType::INT_ARRAY; break;
                    case EScriptType::FLOAT: ReturnType = EScriptType::FLOAT_ARRAY; break;
                    case EScriptType::STRING: ReturnType = EScriptType::STRING_ARRAY; break;
                    case EScriptType::BOOL: ReturnType = EScriptType::BOOL_ARRAY; break;
                    default: break;
                }
            }
        }
        
        // Check if identifier followed by (
        bool bIsFunction = false;
        if (Check(ETokenType::IDENTIFIER))
        {
            Advance(); // identifier
            bIsFunction = Check(ETokenType::LEFT_PAREN);
        }
        
        // Restore position to right after the type token (before [] if present)
        Current = SavedCurrent;
        
        if (bIsFunction)
        {
            // For functions, we need to parse [] again in ParseFunctionWithReturnType
            // So restore to after type token and let function parser handle []
            return ParseFunctionWithReturnType(GetTypeFromToken(Previous()));
        }
        
        // It's a variable declaration - let ParseVarDeclaration handle []
        return ParseVarDeclaration();
    }
    
    return ParseStatement();
}

TSharedPtr<FFunctionDecl> FScriptParser::ParseFunction()
{
    // function name(params) { body }
    if (!Consume(ETokenType::IDENTIFIER, TEXT("Expected function name")))
    {
        Synchronize();
        return nullptr;
    }
    
    FScriptToken Name = Previous();
    
    if (!Consume(ETokenType::LEFT_PAREN, TEXT("Expected '(' after function name")))
    {
        Synchronize();
        return nullptr;
    }
    
    TArray<FScriptToken> Parameters;
    if (!Check(ETokenType::RIGHT_PAREN))
    {
        do
        {
            if (!Consume(ETokenType::IDENTIFIER, TEXT("Expected parameter name")))
            {
                Synchronize();
                return nullptr;
            }
            Parameters.Add(Previous());
        } while (Match(ETokenType::COMMA));
    }
    
    if (!Consume(ETokenType::RIGHT_PAREN, TEXT("Expected ')' after parameters")))
    {
        Synchronize();
        return nullptr;
    }
    
    if (!Consume(ETokenType::LEFT_BRACE, TEXT("Expected '{' before function body")))
    {
        Synchronize();
        return nullptr;
    }
    
    TSharedPtr<FBlockStmt> Body = ParseBlock();
    if (!Body.IsValid())
    {
        return nullptr;
    }
    
    return MakeShared<FFunctionDecl>(Name, Parameters, Body);
}

TSharedPtr<FFunctionDecl> FScriptParser::ParseFunctionWithReturnType(EScriptType ReturnType)
{
    // type name(params) { body } OR type[] name(params) { body }
    // Return type already consumed, now expect: [] (optional), name ( params ) { body }
    
    // Check for array return type: int[], float[], string[]
    if (Match(ETokenType::LEFT_BRACKET))
    {
        if (!Consume(ETokenType::RIGHT_BRACKET, TEXT("Expected ']' after '[' for array return type")))
        {
            Synchronize();
            return nullptr;
        }
        
        // Convert to array type
        switch (ReturnType)
        {
            case EScriptType::INT: ReturnType = EScriptType::INT_ARRAY; break;
            case EScriptType::FLOAT: ReturnType = EScriptType::FLOAT_ARRAY; break;
            case EScriptType::STRING: ReturnType = EScriptType::STRING_ARRAY; break;
            case EScriptType::BOOL: ReturnType = EScriptType::BOOL_ARRAY; break;
            default:
                ReportError(TEXT("Cannot create array return type for this type"));
                Synchronize();
                return nullptr;
        }
    }
    
    if (!Consume(ETokenType::IDENTIFIER, TEXT("Expected function name")))
    {
        Synchronize();
        return nullptr;
    }
    
    FScriptToken Name = Previous();
    
    if (!Consume(ETokenType::LEFT_PAREN, TEXT("Expected '(' after function name")))
    {
        Synchronize();
        return nullptr;
    }
    
    // Parse typed parameters: int x, float y, string name
    TArray<FParameter> TypedParameters;
    
    if (!Check(ETokenType::RIGHT_PAREN))
    {
        do
        {
            // Parse parameter type
            TArray<ETokenType> TypeTokens = {
                ETokenType::INT, ETokenType::FLOAT, ETokenType::STRING_TYPE,
                ETokenType::VOID, ETokenType::VAR
            };
            
            if (!Match(TypeTokens))
            {
                ReportError(TEXT("Expected parameter type"));
                Synchronize();
                return nullptr;
            }
            
            EScriptType ParamType = GetTypeFromToken(Previous());
            
            // Check for array parameter: int[], float[], string[]
            if (Match(ETokenType::LEFT_BRACKET))
            {
                if (!Consume(ETokenType::RIGHT_BRACKET, TEXT("Expected ']' after '[' for array parameter")))
                {
                    Synchronize();
                    return nullptr;
                }
                
                // Convert to array type
                switch (ParamType)
                {
                    case EScriptType::INT: ParamType = EScriptType::INT_ARRAY; break;
                    case EScriptType::FLOAT: ParamType = EScriptType::FLOAT_ARRAY; break;
                    case EScriptType::STRING: ParamType = EScriptType::STRING_ARRAY; break;
                    case EScriptType::BOOL: ParamType = EScriptType::BOOL_ARRAY; break;
                    default: break;
                }
            }
            
            // Parse parameter name
            if (!Consume(ETokenType::IDENTIFIER, TEXT("Expected parameter name")))
            {
                Synchronize();
                return nullptr;
            }
            
            FScriptToken ParamName = Previous();
            
            FParameter Param;
            Param.Type = ParamType;
            Param.Name = ParamName;
            TypedParameters.Add(Param);
            
        } while (Match(ETokenType::COMMA));
    }
    
    if (!Consume(ETokenType::RIGHT_PAREN, TEXT("Expected ')' after parameters")))
    {
        Synchronize();
        return nullptr;
    }
    
    if (!Consume(ETokenType::LEFT_BRACE, TEXT("Expected '{' before function body")))
    {
        Synchronize();
        return nullptr;
    }
    
    TSharedPtr<FBlockStmt> Body = ParseBlock();
    if (!Body.IsValid())
    {
        return nullptr;
    }
    
    // Create FFunctionDecl with return type
    TSharedPtr<FFunctionDecl> Func = MakeShared<FFunctionDecl>(Name, TArray<FScriptToken>(), Body);
    Func->ReturnType = ReturnType;
    Func->TypedParameters = TypedParameters;
    
    return Func;
}

EScriptType FScriptParser::GetTypeFromToken(const FScriptToken& Token)
{
    switch (Token.Type)
    {
        case ETokenType::INT: return EScriptType::INT;
        case ETokenType::FLOAT: return EScriptType::FLOAT;
        case ETokenType::STRING_TYPE: return EScriptType::STRING;
        case ETokenType::VOID: return EScriptType::VOID;
        case ETokenType::VAR: return EScriptType::AUTO;
        default: return EScriptType::AUTO;
    }
}

TSharedPtr<FVarDeclStmt> FScriptParser::ParseVarDeclaration()
{
    // Previous token was the type (int, float, string, void, var)
    FScriptToken TypeToken = Previous();
    
    EScriptType VarType = EScriptType::AUTO;
    switch (TypeToken.Type)
    {
        case ETokenType::INT: VarType = EScriptType::INT; break;
        case ETokenType::FLOAT: VarType = EScriptType::FLOAT; break;
        case ETokenType::STRING_TYPE: VarType = EScriptType::STRING; break;
        case ETokenType::VOID: VarType = EScriptType::VOID; break;
        case ETokenType::VAR: VarType = EScriptType::AUTO; break;
        default:
            ReportError(TEXT("Invalid type in variable declaration"));
            Synchronize();
            return nullptr;
    }
    
    // Check for array type: type[]
    if (Match(ETokenType::LEFT_BRACKET))
    {
        if (!Consume(ETokenType::RIGHT_BRACKET, TEXT("Expected ']' after '[' in array type")))
        {
            Synchronize();
            return nullptr;
        }
        
        // Convert to array type
        switch (VarType)
        {
            case EScriptType::INT: VarType = EScriptType::INT_ARRAY; break;
            case EScriptType::FLOAT: VarType = EScriptType::FLOAT_ARRAY; break;
            case EScriptType::STRING: VarType = EScriptType::STRING_ARRAY; break;
            case EScriptType::BOOL: VarType = EScriptType::BOOL_ARRAY; break;
            default:
                ReportError(TEXT("Invalid array type in variable declaration"));
                Synchronize();
                return nullptr;
        }
    }
    
    if (!Consume(ETokenType::IDENTIFIER, TEXT("Expected variable name")))
    {
        Synchronize();
        return nullptr;
    }
    
    FScriptToken Name = Previous();
    
    TSharedPtr<FScriptExpression> Initializer = nullptr;
    if (Match(ETokenType::EQUAL))
    {
        // Special handling for array initializers with {...}
        if (Check(ETokenType::LEFT_BRACE) && (VarType == EScriptType::INT_ARRAY || 
                                               VarType == EScriptType::FLOAT_ARRAY || 
                                               VarType == EScriptType::STRING_ARRAY || 
                                               VarType == EScriptType::BOOL_ARRAY))
        {
            Advance(); // Consume '{'
            
            TArray<TSharedPtr<FScriptExpression>> Elements;
            
            // Empty array: {}
            if (!Check(ETokenType::RIGHT_BRACE))
            {
                do
                {
                    TSharedPtr<FScriptExpression> Element = ParseExpression();
                    if (!Element.IsValid())
                    {
                        ReportError(TEXT("Expected expression in array initializer"));
                        Synchronize();
                        return nullptr;
                    }
                    Elements.Add(Element);
                }
                while (Match(ETokenType::COMMA));
            }
            
            if (!Consume(ETokenType::RIGHT_BRACE, TEXT("Expected '}' after array elements")))
            {
                Synchronize();
                return nullptr;
            }
            
            Initializer = MakeShared<FArrayLiteralExpr>(Elements);
        }
        else
        {
            Initializer = ParseExpression();
            if (!Initializer.IsValid())
            {
                Synchronize();
                return nullptr;
            }
        }
    }
    
    if (!Consume(ETokenType::SEMICOLON, TEXT("Expected ';' after variable declaration")))
    {
        Synchronize();
        return nullptr;
    }
    
    return MakeShared<FVarDeclStmt>(VarType, Name, Initializer);
}

//=============================================================================
// Statement Parsing
//=============================================================================

TSharedPtr<FScriptStatement> FScriptParser::ParseStatement()
{
    if (Match(ETokenType::IF))
    {
        return ParseIfStatement();
    }
    
    if (Match(ETokenType::WHILE))
    {
        return ParseWhileStatement();
    }
    
    if (Match(ETokenType::FOR))
    {
        return ParseForStatement();
    }
    
    if (Match(ETokenType::BREAK))
    {
        return ParseBreakStatement();
    }
    
    if (Match(ETokenType::CONTINUE))
    {
        return ParseContinueStatement();
    }
    
    if (Match(ETokenType::SWITCH))
    {
        return ParseSwitchStatement();
    }
    
    if (Match(ETokenType::RETURN))
    {
        return ParseReturnStatement();
    }
    
    if (Match(ETokenType::LEFT_BRACE))
    {
        return ParseBlock();
    }
    
    return ParseExpressionStatement();
}

TSharedPtr<FScriptStatement> FScriptParser::ParseExpressionStatement()
{
    TSharedPtr<FScriptExpression> Expr = ParseExpression();
    if (!Expr.IsValid())
    {
        Synchronize();
        return nullptr;
    }
    
    if (!Consume(ETokenType::SEMICOLON, TEXT("Expected ';' after expression")))
    {
        Synchronize();
        return nullptr;
    }
    
    return MakeShared<FExprStmt>(Expr);
}

TSharedPtr<FBlockStmt> FScriptParser::ParseBlock()
{
    TArray<TSharedPtr<FScriptStatement>> Statements;
    
    while (!Check(ETokenType::RIGHT_BRACE) && !IsAtEnd())
    {
        TSharedPtr<FScriptASTNode> Decl = ParseDeclaration();
        if (Decl.IsValid())
        {
            Statements.Add(StaticCastSharedPtr<FScriptStatement>(Decl));
        }
    }
    
    if (!Consume(ETokenType::RIGHT_BRACE, TEXT("Expected '}' after block")))
    {
        Synchronize();
        return nullptr;
    }
    
    return MakeShared<FBlockStmt>(Statements);
}

TSharedPtr<FIfStmt> FScriptParser::ParseIfStatement()
{
    if (!Consume(ETokenType::LEFT_PAREN, TEXT("Expected '(' after 'if'")))
    {
        Synchronize();
        return nullptr;
    }
    
    TSharedPtr<FScriptExpression> Condition = ParseExpression();
    if (!Condition.IsValid())
    {
        Synchronize();
        return nullptr;
    }
    
    if (!Consume(ETokenType::RIGHT_PAREN, TEXT("Expected ')' after if condition")))
    {
        Synchronize();
        return nullptr;
    }
    
    TSharedPtr<FScriptStatement> ThenBranch = ParseStatement();
    if (!ThenBranch.IsValid())
    {
        return nullptr;
    }
    
    TSharedPtr<FScriptStatement> ElseBranch = nullptr;
    if (Match(ETokenType::ELSE))
    {
        ElseBranch = ParseStatement();
        if (!ElseBranch.IsValid())
        {
            return nullptr;
        }
    }
    
    return MakeShared<FIfStmt>(Condition, ThenBranch, ElseBranch);
}

TSharedPtr<FWhileStmt> FScriptParser::ParseWhileStatement()
{
    if (!Consume(ETokenType::LEFT_PAREN, TEXT("Expected '(' after 'while'")))
    {
        Synchronize();
        return nullptr;
    }
    
    TSharedPtr<FScriptExpression> Condition = ParseExpression();
    if (!Condition.IsValid())
    {
        Synchronize();
        return nullptr;
    }
    
    if (!Consume(ETokenType::RIGHT_PAREN, TEXT("Expected ')' after while condition")))
    {
        Synchronize();
        return nullptr;
    }
    
    TSharedPtr<FScriptStatement> Body = ParseStatement();
    if (!Body.IsValid())
    {
        return nullptr;
    }
    
    return MakeShared<FWhileStmt>(Condition, Body);
}

TSharedPtr<FReturnStmt> FScriptParser::ParseReturnStatement()
{
    TSharedPtr<FScriptExpression> Value = nullptr;
    
    if (!Check(ETokenType::SEMICOLON))
    {
        Value = ParseExpression();
        if (!Value.IsValid())
        {
            Synchronize();
            return nullptr;
        }
    }
    
    if (!Consume(ETokenType::SEMICOLON, TEXT("Expected ';' after return statement")))
    {
        Synchronize();
        return nullptr;
    }
    
    return MakeShared<FReturnStmt>(Value);
}

TSharedPtr<FBreakStmt> FScriptParser::ParseBreakStatement()
{
    // 'break' keyword already consumed
    
    if (!Consume(ETokenType::SEMICOLON, TEXT("Expected ';' after 'break'")))
    {
        Synchronize();
        return nullptr;
    }
    
    return MakeShared<FBreakStmt>();
}

TSharedPtr<FContinueStmt> FScriptParser::ParseContinueStatement()
{
    // 'continue' keyword already consumed
    
    if (!Consume(ETokenType::SEMICOLON, TEXT("Expected ';' after 'continue'")))
    {
        Synchronize();
        return nullptr;
    }
    
    return MakeShared<FContinueStmt>();
}

//=============================================================================
// Expression Parsing (Precedence Climbing)
//=============================================================================

TSharedPtr<FScriptExpression> FScriptParser::ParseExpression()
{
    return ParseAssignment();
}

TSharedPtr<FScriptExpression> FScriptParser::ParseAssignment()
{
    TSharedPtr<FScriptExpression> Expr = ParseLogicalOr();
    if (!Expr.IsValid()) return nullptr;
    
    if (Match(ETokenType::EQUAL))
    {
        FScriptToken Equals = Previous();
        TSharedPtr<FScriptExpression> Value = ParseAssignment();
        
        if (!Value.IsValid())
        {
            ReportError(TEXT("Invalid assignment target"));
            return nullptr;
        }
        
        return MakeShared<FAssignExpr>(Expr, Value);
    }
    
    return Expr;
}

TSharedPtr<FScriptExpression> FScriptParser::ParseLogicalOr()
{
    TSharedPtr<FScriptExpression> Expr = ParseLogicalAnd();
    if (!Expr.IsValid()) return nullptr;
    
    while (Match({ETokenType::OR, ETokenType::PIPE_PIPE}))  // Support both 'or' and '||'
    {
        FScriptToken Op = Previous();
        TSharedPtr<FScriptExpression> Right = ParseLogicalAnd();
        
        if (!Right.IsValid())
        {
            ReportError(TEXT("Expected expression after logical OR"));
            return nullptr;
        }
        
        Expr = MakeShared<FBinaryExpr>(Expr, Op, Right);
    }
    
    return Expr;
}

TSharedPtr<FScriptExpression> FScriptParser::ParseLogicalAnd()
{
    TSharedPtr<FScriptExpression> Expr = ParseBitwiseOr();
    if (!Expr.IsValid()) return nullptr;
    
    while (Match({ETokenType::AND, ETokenType::AMPERSAND_AMPERSAND}))  // Support both 'and' and '&&'
    {
        FScriptToken Op = Previous();
        TSharedPtr<FScriptExpression> Right = ParseBitwiseOr();
        
        if (!Right.IsValid())
        {
            ReportError(TEXT("Expected expression after logical AND"));
            return nullptr;
        }
        
        Expr = MakeShared<FBinaryExpr>(Expr, Op, Right);
    }
    
    return Expr;
}

TSharedPtr<FScriptExpression> FScriptParser::ParseBitwiseOr()
{
    TSharedPtr<FScriptExpression> Expr = ParseBitwiseXor();
    if (!Expr.IsValid()) return nullptr;
    
    while (Match(ETokenType::PIPE))
    {
        FScriptToken Op = Previous();
        TSharedPtr<FScriptExpression> Right = ParseBitwiseXor();
        
        if (!Right.IsValid())
        {
            ReportError(TEXT("Expected expression after bitwise OR"));
            return nullptr;
        }
        
        Expr = MakeShared<FBinaryExpr>(Expr, Op, Right);
    }
    
    return Expr;
}

TSharedPtr<FScriptExpression> FScriptParser::ParseBitwiseXor()
{
    TSharedPtr<FScriptExpression> Expr = ParseBitwiseAnd();
    if (!Expr.IsValid()) return nullptr;
    
    while (Match(ETokenType::CARET))
    {
        FScriptToken Op = Previous();
        TSharedPtr<FScriptExpression> Right = ParseBitwiseAnd();
        
        if (!Right.IsValid())
        {
            ReportError(TEXT("Expected expression after bitwise XOR"));
            return nullptr;
        }
        
        Expr = MakeShared<FBinaryExpr>(Expr, Op, Right);
    }
    
    return Expr;
}

TSharedPtr<FScriptExpression> FScriptParser::ParseBitwiseAnd()
{
    TSharedPtr<FScriptExpression> Expr = ParseEquality();
    if (!Expr.IsValid()) return nullptr;
    
    while (Match(ETokenType::AMPERSAND))
    {
        FScriptToken Op = Previous();
        TSharedPtr<FScriptExpression> Right = ParseEquality();
        
        if (!Right.IsValid())
        {
            ReportError(TEXT("Expected expression after bitwise AND"));
            return nullptr;
        }
        
        Expr = MakeShared<FBinaryExpr>(Expr, Op, Right);
    }
    
    return Expr;
}

TSharedPtr<FScriptExpression> FScriptParser::ParseEquality()
{
    TSharedPtr<FScriptExpression> Expr = ParseComparison();
    if (!Expr.IsValid()) return nullptr;
    
    while (Match({ETokenType::BANG_EQUAL, ETokenType::EQUAL_EQUAL}))
    {
        FScriptToken Op = Previous();
        TSharedPtr<FScriptExpression> Right = ParseComparison();
        
        if (!Right.IsValid())
        {
            ReportError(TEXT("Expected expression after comparison operator"));
            return nullptr;
        }
        
        Expr = MakeShared<FBinaryExpr>(Expr, Op, Right);
    }
    
    return Expr;
}

TSharedPtr<FScriptExpression> FScriptParser::ParseComparison()
{
    TSharedPtr<FScriptExpression> Expr = ParseTerm();
    if (!Expr.IsValid()) return nullptr;
    
    while (Match({ETokenType::GREATER, ETokenType::GREATER_EQUAL,
                  ETokenType::LESS, ETokenType::LESS_EQUAL}))
    {
        FScriptToken Op = Previous();
        TSharedPtr<FScriptExpression> Right = ParseTerm();
        
        if (!Right.IsValid())
        {
            ReportError(TEXT("Expected expression after comparison operator"));
            return nullptr;
        }
        
        Expr = MakeShared<FBinaryExpr>(Expr, Op, Right);
    }
    
    return Expr;
}

TSharedPtr<FScriptExpression> FScriptParser::ParseTerm()
{
    TSharedPtr<FScriptExpression> Expr = ParseFactor();
    if (!Expr.IsValid()) return nullptr;
    
    while (Match({ETokenType::MINUS, ETokenType::PLUS}))
    {
        FScriptToken Op = Previous();
        TSharedPtr<FScriptExpression> Right = ParseFactor();
        
        if (!Right.IsValid())
        {
            ReportError(TEXT("Expected expression after '+' or '-'"));
            return nullptr;
        }
        
        Expr = MakeShared<FBinaryExpr>(Expr, Op, Right);
    }
    
    return Expr;
}

TSharedPtr<FScriptExpression> FScriptParser::ParseFactor()
{
    TSharedPtr<FScriptExpression> Expr = ParseUnary(); // Call unary which will call call which will call arrayaccess
    if (!Expr.IsValid()) return nullptr;
    
    while (Match({ETokenType::SLASH, ETokenType::STAR, ETokenType::PERCENT}))
    {
        FScriptToken Op = Previous();
        TSharedPtr<FScriptExpression> Right = ParseUnary();
        
        if (!Right.IsValid())
        {
            ReportError(TEXT("Expected expression after '*', '/' or '%'"));
            return nullptr;
        }
        
        Expr = MakeShared<FBinaryExpr>(Expr, Op, Right);
    }
    
    return Expr;
}

TSharedPtr<FScriptExpression> FScriptParser::ParseUnary()
{
    if (Match({ETokenType::BANG, ETokenType::MINUS, ETokenType::TILDE}))
    {
        FScriptToken Op = Previous();
        TSharedPtr<FScriptExpression> Right = ParseUnary();
        
        if (!Right.IsValid())
        {
            ReportError(TEXT("Expected expression after unary operator"));
            return nullptr;
        }
        
        return MakeShared<FUnaryExpr>(Op, Right);
    }
    
    // Type cast: (int)expr or (float)expr
    if (Match(ETokenType::LEFT_PAREN))
    {
        // Peek ahead to check if this is a type cast
        if (Check(ETokenType::INT) || Check(ETokenType::FLOAT) || 
            Check(ETokenType::STRING_TYPE) || Check(ETokenType::VOID))
        {
            FScriptToken TypeToken = Advance();
            
            if (!Consume(ETokenType::RIGHT_PAREN, TEXT("Expected ')' after type in cast")))
            {
                return nullptr;
            }
            
            TSharedPtr<FScriptExpression> Expr = ParseUnary();
            if (!Expr.IsValid())
            {
                ReportError(TEXT("Expected expression after type cast"));
                return nullptr;
            }
            
            EScriptType TargetType = EScriptType::AUTO;
            switch (TypeToken.Type)
            {
                case ETokenType::INT: TargetType = EScriptType::INT; break;
                case ETokenType::FLOAT: TargetType = EScriptType::FLOAT; break;
                case ETokenType::STRING_TYPE: TargetType = EScriptType::STRING; break;
                case ETokenType::VOID: TargetType = EScriptType::VOID; break;
                default: break;
            }
            
            return MakeShared<FTypeCastExpr>(TargetType, Expr);
        }
        else
        {
            // Not a type cast, backtrack and parse as call/primary
            Current--;
        }
    }
    
    return ParseCall();
}

TSharedPtr<FScriptExpression> FScriptParser::ParseCall()
{
    TSharedPtr<FScriptExpression> Expr = ParsePrimary();
    if (!Expr.IsValid()) return nullptr;
    
    while (true)
    {
        if (Match(ETokenType::LEFT_PAREN))
        {
            // Function call: func()
            Expr = FinishCall(Expr);
            if (!Expr.IsValid()) return nullptr;
        }
        else if (Match(ETokenType::LEFT_BRACKET))
        {
            // Array access: arr[index]
            TSharedPtr<FScriptExpression> Index = ParseExpression();
            if (!Index.IsValid())
            {
                ReportError(TEXT("Expected index expression in array access"));
                return nullptr;
            }
            
            if (!Consume(ETokenType::RIGHT_BRACKET, TEXT("Expected ']' after array index")))
            {
                return nullptr;
            }
            
            Expr = MakeShared<FArrayAccessExpr>(Expr, Index);
        }
        else if (Match(ETokenType::DOT))
        {
            // Struct/object field access: obj.field
            if (!Check(ETokenType::IDENTIFIER))
            {
                ReportError(TEXT("Expected property name after '.'"));
                return nullptr;
            }
            
            FScriptToken PropertyName = Advance();
            Expr = MakeShared<FStructAccessExpr>(Expr, PropertyName);
        }
        else
        {
            break;
        }
    }
    
    return Expr;
}

TSharedPtr<FScriptExpression> FScriptParser::ParsePrimary()
{
    // Array literals [element, element, ...]
    if (Check(ETokenType::LEFT_BRACKET))
    {
        return ParseArrayLiteral();
    }
    
    // Literals
    if (Match(ETokenType::KW_TRUE))
    {
        return MakeShared<FLiteralExpr>(Previous(), true);
    }
    
    if (Match(ETokenType::KW_FALSE))
    {
        return MakeShared<FLiteralExpr>(Previous(), false);
    }
    
    if (Match(ETokenType::NIL))
    {
        return MakeShared<FLiteralExpr>(Previous(), 0.0);
    }
    
    if (Match(ETokenType::NUMBER))
    {
        FScriptToken Token = Previous();
        double Value = FCString::Atod(*Token.Lexeme);
        return MakeShared<FLiteralExpr>(Token, Value);
    }
    
    if (Match(ETokenType::STRING))
    {
        FScriptToken Token = Previous();
        return MakeShared<FLiteralExpr>(Token, Token.Lexeme);
    }
    
    if (Match(ETokenType::IDENTIFIER))
    {
        return MakeShared<FIdentifierExpr>(Previous());
    }
    
    // Grouping
    if (Match(ETokenType::LEFT_PAREN))
    {
        TSharedPtr<FScriptExpression> Expr = ParseExpression();
        if (!Expr.IsValid())
        {
            ReportError(TEXT("Expected expression in grouping"));
            return nullptr;
        }
        
        if (!Consume(ETokenType::RIGHT_PAREN, TEXT("Expected ')' after expression")))
        {
            return nullptr;
        }
        
        return Expr;
    }
    
    ReportError(TEXT("Expected expression"));
    return nullptr;
}

TSharedPtr<FScriptExpression> FScriptParser::FinishCall(TSharedPtr<FScriptExpression> Callee)
{
    TArray<TSharedPtr<FScriptExpression>> Arguments;
    
    if (!Check(ETokenType::RIGHT_PAREN))
    {
        do
        {
            TSharedPtr<FScriptExpression> Arg = ParseExpression();
            if (!Arg.IsValid())
            {
                ReportError(TEXT("Invalid argument in function call"));
                return nullptr;
            }
            Arguments.Add(Arg);
        } while (Match(ETokenType::COMMA));
    }
    
    if (!Consume(ETokenType::RIGHT_PAREN, TEXT("Expected ')' after arguments")))
    {
        return nullptr;
    }
    
    return MakeShared<FCallExpr>(Callee, Arguments);
}

TSharedPtr<FScriptStatement> FScriptParser::ParseForStatement()
{
    // Parse for (init; condition; increment) body
    if (!Consume(ETokenType::LEFT_PAREN, TEXT("Expected '(' after 'for'")))
    {
        Synchronize();
        return nullptr;
    }
    
    // Parse initialization
    TSharedPtr<FScriptStatement> Init = nullptr;
    if (Check(ETokenType::SEMICOLON))
    {
        // Empty init
        Advance(); // Consume semicolon
    }
    else if (Check(ETokenType::VAR) || Check(ETokenType::INT) || Check(ETokenType::FLOAT) || Check(ETokenType::STRING_TYPE))
    {
        // Consume the type token before calling ParseVarDeclaration
        Advance();
        Init = ParseVarDeclaration();
    }
    else
    {
        Init = ParseExpressionStatement();
    }
    
    // Parse condition
    TSharedPtr<FScriptExpression> Condition = nullptr;
    if (!Check(ETokenType::SEMICOLON))
    {
        Condition = ParseExpression();
    }
    if (!Condition.IsValid())
    {
        Condition = MakeShared<FLiteralExpr>(FScriptToken(ETokenType::KW_TRUE, TEXT("true"), 0, 0), true);
    }
    
    if (!Consume(ETokenType::SEMICOLON, TEXT("Expected ';' after condition in for loop")))
    {
        Synchronize();
        return nullptr;
    }
    
    // Parse increment
    TSharedPtr<FScriptExpression> Increment = nullptr;
    if (!Check(ETokenType::RIGHT_PAREN))
    {
        Increment = ParseExpression();
    }
    
    if (!Consume(ETokenType::RIGHT_PAREN, TEXT("Expected ')' after for clauses")))
    {
        Synchronize();
        return nullptr;
    }
    
    // Parse body
    TSharedPtr<FScriptStatement> Body = ParseStatement();
    if (!Body.IsValid())
    {
        return nullptr;
    }
    
    // Syntactic sugar: convert for loop to while loop
    // for(init; condition; increment) body => { init; while(condition) { body; increment; } }
    
    // Create a block with the initialization and while loop
    TArray<TSharedPtr<FScriptStatement>> BlockStmts;
    
    if (Init.IsValid())
    {
        BlockStmts.Add(Init);
    }
    
    // Create the while loop: while(condition) { body; increment; }
    TArray<TSharedPtr<FScriptStatement>> WhileBodyStmts;
    
    WhileBodyStmts.Add(Body);
    
    // If increment exists, add it as a statement
    if (Increment.IsValid())
    {
        WhileBodyStmts.Add(MakeShared<FExprStmt>(Increment));
    }
    
    TSharedPtr<FBlockStmt> WhileBody = MakeShared<FBlockStmt>(WhileBodyStmts);
    TSharedPtr<FWhileStmt> WhileLoop = MakeShared<FWhileStmt>(Condition, WhileBody);
    
    BlockStmts.Add(WhileLoop);
    
    return MakeShared<FBlockStmt>(BlockStmts);
}

TSharedPtr<FScriptStatement> FScriptParser::ParseSwitchStatement()
{
    if (!Consume(ETokenType::LEFT_PAREN, TEXT("Expected '(' after 'switch'")))
    {
        Synchronize();
        return nullptr;
    }
    
    TSharedPtr<FScriptExpression> Expression = ParseExpression();
    if (!Expression.IsValid())
    {
        ReportError(TEXT("Expected expression in switch statement"));
        Synchronize();
        return nullptr;
    }
    
    if (!Consume(ETokenType::RIGHT_PAREN, TEXT("Expected ')' after switch expression")))
    {
        Synchronize();
        return nullptr;
    }
    
    if (!Consume(ETokenType::LEFT_BRACE, TEXT("Expected '{' after switch statement")))
    {
        Synchronize();
        return nullptr;
    }
    
    TArray<TPair<TSharedPtr<FScriptExpression>, TSharedPtr<FScriptStatement>>> Cases;
    TSharedPtr<FScriptStatement> DefaultCase = nullptr;
    
    // Parse cases (case VALUE :, default :)
    while (!Check(ETokenType::RIGHT_BRACE) && !IsAtEnd())
    {
        if (Match(ETokenType::CASE))
        {
            TSharedPtr<FScriptExpression> Value = ParseExpression();
            if (!Value.IsValid())
            {
                ReportError(TEXT("Expected value in 'case' statement"));
                Synchronize();
                return nullptr;
            }
            
            if (!Consume(ETokenType::COLON, TEXT("Expected ':' after case value")))
            {
                Synchronize();
                return nullptr;
            }
            
            // Parse the statements until we hit another case, default, or closing brace 
            TArray<TSharedPtr<FScriptStatement>> CaseBodyStmts;
            
            while (!Check(ETokenType::CASE) && 
                   !Check(ETokenType::DEFAULT) && 
                   !Check(ETokenType::RIGHT_BRACE) && 
                   !IsAtEnd())
            {
                TSharedPtr<FScriptStatement> Stmt = ParseStatement();
                if (!Stmt.IsValid())
                {
                    break;
                }
                CaseBodyStmts.Add(Stmt);
            }
            
            TSharedPtr<FBlockStmt> CaseBody = MakeShared<FBlockStmt>(CaseBodyStmts);
            Cases.Add(TPair<TSharedPtr<FScriptExpression>, TSharedPtr<FScriptStatement>>(Value, CaseBody));
        }
        else if (Match(ETokenType::DEFAULT))
        {
            if (!Consume(ETokenType::COLON, TEXT("Expected ':' after 'default'")))
            {
                Synchronize();
                return nullptr;
            }
            
            // Parse default case statements
            TArray<TSharedPtr<FScriptStatement>> DefaultBodyStmts;
            
            while (!Check(ETokenType::CASE) && 
                   !Check(ETokenType::RIGHT_BRACE) && 
                   !IsAtEnd())
            {
                TSharedPtr<FScriptStatement> Stmt = ParseStatement();
                if (!Stmt.IsValid())
                {
                    break;
                }
                DefaultBodyStmts.Add(Stmt);
            }
            
            DefaultCase = MakeShared<FBlockStmt>(DefaultBodyStmts);
        }
        else
        {
            ReportError(TEXT("Expected 'case' or 'default' in switch statement"));
            Synchronize();
            break;
        }
    }
    
    if (!Consume(ETokenType::RIGHT_BRACE, TEXT("Expected '}' after switch statement")))
    {
        Synchronize();
        return nullptr;
    }
    
    return MakeShared<FSwitchStmt>(Expression, Cases, DefaultCase);
}

