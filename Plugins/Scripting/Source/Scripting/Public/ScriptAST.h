// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h" // Required for UENUM()
#include "ScriptToken.h"

/**
 * Type information for variables and expressions
 */
UENUM()
enum class EScriptType : uint8
{
    VOID,
    INT,
    FLOAT,
    STRING,
    BOOL,
    AUTO,           // Type inference
    
    // Array types
    INT_ARRAY,      // int[]
    FLOAT_ARRAY,    // float[]
    STRING_ARRAY,   // string[]
    BOOL_ARRAY      // bool[]
};

// Forward declarations
class FScriptExpression;

/**
 * Base AST node
 */
class SCRIPTING_API FScriptASTNode
{
public:
    virtual ~FScriptASTNode() = default;
    
    virtual FString ToString() const { return TEXT("ASTNode"); }
    virtual FString GetNodeType() const { return TEXT("FScriptASTNode"); }
    
    // Validation
    virtual bool IsValid() const { return true; }
};

/**
 * Base class for all statement AST nodes
 */
class SCRIPTING_API FScriptStatement : public FScriptASTNode
{
public:
    virtual FString GetNodeType() const override { return TEXT("Statement"); }
    virtual bool IsValid() const override { return true; }
};

/**
 * Expression base class
 */
class SCRIPTING_API FScriptExpression : public FScriptASTNode
{
public:
    EScriptType InferredType = EScriptType::AUTO;
    
    virtual FString GetNodeType() const override { return TEXT("Expression"); }
};

/**
 * Literal expression (number, string, bool, nil)
 */
class SCRIPTING_API FLiteralExpr : public FScriptExpression
{
public:
    FScriptToken Token;
    
    FLiteralExpr(const FScriptToken& InToken) : Token(InToken) {}
    
    // Convenience constructors
    FLiteralExpr(const FScriptToken& InToken, bool BoolValue)
        : Token(InToken)
    {
        Token.Lexeme = BoolValue ? TEXT("true") : TEXT("false");
    }
    
    FLiteralExpr(const FScriptToken& InToken, double NumberValue)
        : Token(InToken)
    {
        Token.Lexeme = FString::SanitizeFloat(NumberValue);
    }
    
    FLiteralExpr(const FScriptToken& InToken, const FString& StringValue)
        : Token(InToken)
    {
        Token.Lexeme = StringValue;
    }
    
    virtual FString ToString() const override
    {
        return FString::Printf(TEXT("Literal(%s)"), *Token.Lexeme);
    }
    
    virtual FString GetNodeType() const override { return TEXT("Literal"); }
};

/**
 * Array literal expression (elements: [1, 2, 3] or [type; size])
 */
class SCRIPTING_API FArrayLiteralExpr : public FScriptExpression
{
public:
    TArray<TSharedPtr<FScriptExpression>> Elements;
    
    FArrayLiteralExpr(const TArray<TSharedPtr<FScriptExpression>>& InElements)
        : Elements(InElements)
    {}
    
    virtual bool IsValid() const override
    {
        for (const auto& Element : Elements)
        {
            if (!Element.IsValid() || !Element->IsValid()) return false;
        }
        return true;
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("ArrayLiteral(INVALID)");
        FString ElementsStr;
        for (int32 i = 0; i < Elements.Num(); ++i)
        {
            ElementsStr += Elements[i]->ToString();
            if (i < Elements.Num() - 1) ElementsStr += TEXT(", ");
        }
        return FString::Printf(TEXT("ArrayLiteral([%s])"), *ElementsStr);
    }
    
    virtual FString GetNodeType() const override { return TEXT("ArrayLiteral"); }
};

/**
 * Array access expression (arr[index])
 */
class SCRIPTING_API FArrayAccessExpr : public FScriptExpression
{
public:
    TSharedPtr<FScriptExpression> Array;
    TSharedPtr<FScriptExpression> Index;
    
    FArrayAccessExpr(TSharedPtr<FScriptExpression> InArray, TSharedPtr<FScriptExpression> InIndex)
        : Array(InArray), Index(InIndex)
    {}
    
    virtual bool IsValid() const override
    {
        return Array.IsValid() && Array->IsValid() && 
               Index.IsValid() && Index->IsValid();
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("ArrayAccess(INVALID)");
        return FString::Printf(TEXT("ArrayAccess(%s[%s])"), 
            *Array->ToString(), *Index->ToString());
    }
    
    virtual FString GetNodeType() const override { return TEXT("ArrayAccess"); }
};

/**
 * Array assignment expression (arr[index] = value)
 */
class SCRIPTING_API FArrayAssignExpr : public FScriptExpression
{
public:
    TSharedPtr<FScriptExpression> Array;
    TSharedPtr<FScriptExpression> Index;
    TSharedPtr<FScriptExpression> Value;
    
    FArrayAssignExpr(TSharedPtr<FScriptExpression> InArray, 
                     TSharedPtr<FScriptExpression> InIndex, 
                     TSharedPtr<FScriptExpression> InValue)
        : Array(InArray), Index(InIndex), Value(InValue)
    {}
    
    virtual bool IsValid() const override
    {
        return Array.IsValid() && Array->IsValid() && 
               Index.IsValid() && Index->IsValid() &&
               Value.IsValid() && Value->IsValid();
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("ArrayAssign(INVALID)");
        return FString::Printf(TEXT("ArrayAssign(%s[%s] = %s)"), 
            *Array->ToString(), *Index->ToString(), *Value->ToString());
    }
    
    virtual FString GetNodeType() const override { return TEXT("ArrayAssign"); }
};

/**
 * Struct literal expression (struct { field = value, ... })
 */
class SCRIPTING_API FStructLiteralExpr : public FScriptExpression
{
public:
    FString StructName;
    TMap<FString, TSharedPtr<FScriptExpression>> Fields;
    
    FStructLiteralExpr(const FString& InStructName, 
                      const TMap<FString, TSharedPtr<FScriptExpression>>& InFields)
        : StructName(InStructName), Fields(InFields)
    {}
    
    virtual bool IsValid() const override
    {
        for (const auto& Field : Fields)
        {
            if (!Field.Value.IsValid() || !Field.Value->IsValid()) return false;
        }
        return true;
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("StructLiteral(INVALID)");
        FString FieldsStr;
        
        // Convert map to array to get specific elements by index
        TArray<TPair<FString, TSharedPtr<FScriptExpression>>> FieldArray;
        for (const auto& Field : Fields)
        {
            FieldArray.Add(Field);
        }
        
        for (int32 i = 0; i < FieldArray.Num(); ++i)
        {
            const auto& Field = FieldArray[i];
            FieldsStr += FString::Printf(TEXT("%s=%s"), *Field.Key, *Field.Value->ToString());
            if (i < FieldArray.Num() - 1) FieldsStr += TEXT(", ");
        }
        
        return FString::Printf(TEXT("StructLiteral(%s{%s})"), *StructName, *FieldsStr);
    }
    
    virtual FString GetNodeType() const override { return TEXT("StructLiteral"); }
};

/**
 * Struct access expression (obj.field)
 */
class SCRIPTING_API FStructAccessExpr : public FScriptExpression
{
public:
    TSharedPtr<FScriptExpression> Object;
    FScriptToken Field;
    
    FStructAccessExpr(TSharedPtr<FScriptExpression> InObject, const FScriptToken& InField)
        : Object(InObject), Field(InField)
    {}
    
    virtual bool IsValid() const override
    {
        return Object.IsValid() && Object->IsValid();
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("StructAccess(INVALID)");
        return FString::Printf(TEXT("StructAccess(%s.%s)"), 
            *Object->ToString(), *Field.Lexeme);
    }
    
    virtual FString GetNodeType() const override { return TEXT("StructAccess"); }
};

/**
 * Struct assignment expression (obj.field = value)
 */
class SCRIPTING_API FStructAssignExpr : public FScriptExpression
{
public:
    TSharedPtr<FScriptExpression> Object;
    FScriptToken Field;
    TSharedPtr<FScriptExpression> Value;
    
    FStructAssignExpr(TSharedPtr<FScriptExpression> InObject, 
                     const FScriptToken& InField,
                     TSharedPtr<FScriptExpression> InValue)
        : Object(InObject), Field(InField), Value(InValue)
    {}
    
    virtual bool IsValid() const override
    {
        return Object.IsValid() && Object->IsValid() && 
               Value.IsValid() && Value->IsValid();
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("StructAssign(INVALID)");
        return FString::Printf(TEXT("StructAssign(%s.%s = %s)"), 
            *Object->ToString(), *Field.Lexeme, *Value->ToString());
    }
    
    virtual FString GetNodeType() const override { return TEXT("StructAssign"); }
};

/**
 * Switch statement
 */
class SCRIPTING_API FSwitchStmt : public FScriptStatement
{
public:
    TSharedPtr<FScriptExpression> Expression;
    TArray<TPair<TSharedPtr<FScriptExpression>, TSharedPtr<FScriptStatement>>> Cases;
    TSharedPtr<FScriptStatement> DefaultCase;
    
    FSwitchStmt(TSharedPtr<FScriptExpression> InExpr,
                const TArray<TPair<TSharedPtr<FScriptExpression>, TSharedPtr<FScriptStatement>>>& InCases,
                TSharedPtr<FScriptStatement> InDefault = nullptr)
        : Expression(InExpr), Cases(InCases), DefaultCase(InDefault)
    {}
    
    virtual bool IsValid() const override
    {
        if (!Expression.IsValid() || !Expression->IsValid()) return false;
        
        for (const auto& Case : Cases)
        {
            if (!Case.Key.IsValid() || !Case.Key->IsValid() ||
                !Case.Value.IsValid() || !Case.Value->IsValid()) return false;
        }
        
        if (DefaultCase.IsValid() && !DefaultCase->IsValid()) return false;
        
        return true;
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("Switch(INVALID)");
        FString Result = FString::Printf(TEXT("Switch(%s) { "), *Expression->ToString());
        
        for (const auto& Case : Cases)
        {
            Result += FString::Printf(TEXT("Case %s: %s; "), 
                *Case.Key->ToString(), *Case.Value->ToString());
        }
        
        if (DefaultCase.IsValid())
        {
            Result += FString::Printf(TEXT("Default: %s; "), *DefaultCase->ToString());
        }
        
        Result += TEXT("}");
        return Result;
    }
    
    virtual FString GetNodeType() const override { return TEXT("Switch"); }
};

/**
 * Identifier expression (variable reference)
 */
class SCRIPTING_API FIdentifierExpr : public FScriptExpression
{
public:
    FScriptToken Name;
    
    FIdentifierExpr(const FScriptToken& InName) : Name(InName) {}
    
    virtual FString ToString() const override
    {
        return FString::Printf(TEXT("Identifier(%s)"), *Name.Lexeme);
    }
    
    virtual FString GetNodeType() const override { return TEXT("Identifier"); }
};

/**
 * Binary expression (a + b, a * b, etc.)
 */
class SCRIPTING_API FBinaryExpr : public FScriptExpression
{
public:
    TSharedPtr<FScriptExpression> Left;
    FScriptToken Operator;
    TSharedPtr<FScriptExpression> Right;
    
    FBinaryExpr(TSharedPtr<FScriptExpression> InLeft, const FScriptToken& InOp, TSharedPtr<FScriptExpression> InRight)
        : Left(InLeft), Operator(InOp), Right(InRight)
    {}
    
    virtual bool IsValid() const override
    {
        return Left.IsValid() && Right.IsValid() && Left->IsValid() && Right->IsValid();
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("Binary(INVALID)");
        return FString::Printf(TEXT("Binary(%s %s %s)"), 
            *Left->ToString(), *Operator.Lexeme, *Right->ToString());
    }
    
    virtual FString GetNodeType() const override { return TEXT("Binary"); }
};

/**
 * Unary expression (!x, -x)
 */
class SCRIPTING_API FUnaryExpr : public FScriptExpression
{
public:
    FScriptToken Operator;
    TSharedPtr<FScriptExpression> Right;
    
    FUnaryExpr(const FScriptToken& InOp, TSharedPtr<FScriptExpression> InRight)
        : Operator(InOp), Right(InRight)
    {}
    
    virtual bool IsValid() const override
    {
        return Right.IsValid() && Right->IsValid();
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("Unary(INVALID)");
        return FString::Printf(TEXT("Unary(%s%s)"), *Operator.Lexeme, *Right->ToString());
    }
    
    virtual FString GetNodeType() const override { return TEXT("Unary"); }
};

/**
 * Assignment expression (x = 10)
 */
class SCRIPTING_API FAssignExpr : public FScriptExpression
{
public:
    TSharedPtr<FScriptExpression> Target;
    TSharedPtr<FScriptExpression> Value;
    
    FAssignExpr(TSharedPtr<FScriptExpression> InTarget, TSharedPtr<FScriptExpression> InValue)
        : Target(InTarget), Value(InValue)
    {}
    
    virtual bool IsValid() const override
    {
        return Target.IsValid() && Target->IsValid() && Value.IsValid() && Value->IsValid();
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("Assign(INVALID)");
        return FString::Printf(TEXT("Assign(%s = %s)"), *Target->ToString(), *Value->ToString());
    }
    
    virtual FString GetNodeType() const override { return TEXT("Assign"); }
};

/**
 * Call expression (func(arg1, arg2))
 */
class SCRIPTING_API FCallExpr : public FScriptExpression
{
public:
    TSharedPtr<FScriptExpression> Callee;
    TArray<TSharedPtr<FScriptExpression>> Arguments;
    
    FCallExpr(TSharedPtr<FScriptExpression> InCallee, const TArray<TSharedPtr<FScriptExpression>>& InArgs)
        : Callee(InCallee), Arguments(InArgs)
    {}
    
    virtual bool IsValid() const override
    {
        if (!Callee.IsValid() || !Callee->IsValid()) return false;
        for (const auto& Arg : Arguments)
        {
            if (!Arg.IsValid() || !Arg->IsValid()) return false;
        }
        return true;
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("Call(INVALID)");
        FString ArgsStr;
        for (int32 i = 0; i < Arguments.Num(); ++i)
        {
            ArgsStr += Arguments[i]->ToString();
            if (i < Arguments.Num() - 1) ArgsStr += TEXT(", ");
        }
        return FString::Printf(TEXT("Call(%s(%s))"), *Callee->ToString(), *ArgsStr);
    }
    
    virtual FString GetNodeType() const override { return TEXT("Call"); }
};

/**
 * Type cast expression ((int)x, (float)y)
 */
class SCRIPTING_API FTypeCastExpr : public FScriptExpression
{
public:
    EScriptType TargetType;
    TSharedPtr<FScriptExpression> Expression;
    
    FTypeCastExpr(EScriptType InType, TSharedPtr<FScriptExpression> InExpr)
        : TargetType(InType), Expression(InExpr)
    {
        InferredType = TargetType;
    }
    
    virtual bool IsValid() const override
    {
        return Expression.IsValid() && Expression->IsValid();
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("TypeCast(INVALID)");
        return FString::Printf(TEXT("Cast<%s>(%s)"), *GetTypeName(TargetType), *Expression->ToString());
    }
    
    virtual FString GetNodeType() const override { return TEXT("TypeCast"); }
    
    static FString GetTypeName(EScriptType Type)
    {
        switch (Type)
        {
            case EScriptType::VOID: return TEXT("void");
            case EScriptType::INT: return TEXT("int");
            case EScriptType::FLOAT: return TEXT("float");
            case EScriptType::STRING: return TEXT("string");
            case EScriptType::BOOL: return TEXT("bool");
            case EScriptType::AUTO: return TEXT("auto");
            default: return TEXT("unknown");
        }
    }
};



/**
 * Expression statement (expr;)
 */
class SCRIPTING_API FExprStmt : public FScriptStatement
{
public:
    TSharedPtr<FScriptExpression> Expression;
    
    FExprStmt(TSharedPtr<FScriptExpression> InExpr) : Expression(InExpr) {}
    
    virtual bool IsValid() const override
    {
        return Expression.IsValid() && Expression->IsValid();
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("ExprStmt(INVALID)");
        return FString::Printf(TEXT("ExprStmt(%s)"), *Expression->ToString());
    }
    
    virtual FString GetNodeType() const override { return TEXT("ExprStmt"); }
};

/**
 * Import statement (import "header.sc";)
 * Only Scripting headers are allowed for security.
 */
class SCRIPTING_API FImportStmt : public FScriptStatement
{
public:
    FScriptToken Path; // String token containing the header path
    
    FImportStmt(const FScriptToken& InPath) : Path(InPath) {}
    
    virtual bool IsValid() const override { return Path.Type == ETokenType::STRING; }
    virtual FString ToString() const override
    {
        return FString::Printf(TEXT("Import(%s)"), *Path.Lexeme);
    }
    virtual FString GetNodeType() const override { return TEXT("Import"); }
};

/**
 * Variable declaration (int x = 10; float y;)
 */
class SCRIPTING_API FVarDeclStmt : public FScriptStatement
{
public:
    EScriptType VarType;
    FScriptToken Name;
    TSharedPtr<FScriptExpression> Initializer;
    
    FVarDeclStmt(EScriptType InType, const FScriptToken& InName, TSharedPtr<FScriptExpression> InInit = nullptr)
        : VarType(InType), Name(InName), Initializer(InInit)
    {}
    
    virtual bool IsValid() const override
    {
        return !Initializer.IsValid() || Initializer->IsValid();
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("VarDecl(INVALID)");
        FString TypeStr = FTypeCastExpr::GetTypeName(VarType);
        if (Initializer.IsValid())
        {
            return FString::Printf(TEXT("VarDecl(%s %s = %s)"), *TypeStr, *Name.Lexeme, *Initializer->ToString());
        }
        return FString::Printf(TEXT("VarDecl(%s %s)"), *TypeStr, *Name.Lexeme);
    }
    
    virtual FString GetNodeType() const override { return TEXT("VarDecl"); }
};

/**
 * Block statement ({ stmt1; stmt2; })
 */
class SCRIPTING_API FBlockStmt : public FScriptStatement
{
public:
    TArray<TSharedPtr<FScriptStatement>> Statements;
    
    FBlockStmt(const TArray<TSharedPtr<FScriptStatement>>& InStmts) : Statements(InStmts) {}
    
    virtual bool IsValid() const override
    {
        for (const auto& Stmt : Statements)
        {
            if (!Stmt.IsValid() || !Stmt->IsValid()) return false;
        }
        return true;
    }
    
    virtual FString ToString() const override
    {
        FString Result = TEXT("Block({");
        for (int32 i = 0; i < Statements.Num(); ++i)
        {
            Result += Statements[i]->ToString();
            if (i < Statements.Num() - 1) Result += TEXT("; ");
        }
        Result += TEXT("})");
        return Result;
    }
    
    virtual FString GetNodeType() const override { return TEXT("Block"); }
};

/**
 * If statement
 */
class SCRIPTING_API FIfStmt : public FScriptStatement
{
public:
    TSharedPtr<FScriptExpression> Condition;
    TSharedPtr<FScriptStatement> ThenBranch;
    TSharedPtr<FScriptStatement> ElseBranch;
    
    FIfStmt(TSharedPtr<FScriptExpression> InCond, TSharedPtr<FScriptStatement> InThen, 
            TSharedPtr<FScriptStatement> InElse = nullptr)
        : Condition(InCond), ThenBranch(InThen), ElseBranch(InElse)
    {}
    
    virtual bool IsValid() const override
    {
        if (!Condition.IsValid() || !Condition->IsValid()) return false;
        if (!ThenBranch.IsValid() || !ThenBranch->IsValid()) return false;
        if (ElseBranch.IsValid() && !ElseBranch->IsValid()) return false;
        return true;
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("If(INVALID)");
        FString Result = FString::Printf(TEXT("If(%s) Then(%s)"), 
            *Condition->ToString(), *ThenBranch->ToString());
        if (ElseBranch.IsValid())
        {
            Result += FString::Printf(TEXT(" Else(%s)"), *ElseBranch->ToString());
        }
        return Result;
    }
    
    virtual FString GetNodeType() const override { return TEXT("If"); }
};

/**
 * While statement
 */
class SCRIPTING_API FWhileStmt : public FScriptStatement
{
public:
    TSharedPtr<FScriptExpression> Condition;
    TSharedPtr<FScriptStatement> Body;
    
    FWhileStmt(TSharedPtr<FScriptExpression> InCond, TSharedPtr<FScriptStatement> InBody)
        : Condition(InCond), Body(InBody)
    {}
    
    virtual bool IsValid() const override
    {
        return Condition.IsValid() && Condition->IsValid() &&
               Body.IsValid() && Body->IsValid();
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("While(INVALID)");
        return FString::Printf(TEXT("While(%s) Do(%s)"), 
            *Condition->ToString(), *Body->ToString());
    }
    
    virtual FString GetNodeType() const override { return TEXT("While"); }
};

/**
 * Return statement
 */
class SCRIPTING_API FReturnStmt : public FScriptStatement
{
public:
    TSharedPtr<FScriptExpression> Value;
    
    FReturnStmt(TSharedPtr<FScriptExpression> InValue = nullptr) : Value(InValue) {}
    
    virtual bool IsValid() const override
    {
        return !Value.IsValid() || Value->IsValid();
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("Return(INVALID)");
        if (Value.IsValid())
        {
            return FString::Printf(TEXT("Return(%s)"), *Value->ToString());
        }
        return TEXT("Return()");
    }
    
    virtual FString GetNodeType() const override { return TEXT("Return"); }
};

/**
 * Break statement
 */
class SCRIPTING_API FBreakStmt : public FScriptStatement
{
public:
    FBreakStmt() {}
    
    virtual FString ToString() const override { return TEXT("Break()"); }
    virtual FString GetNodeType() const override { return TEXT("Break"); }
};

/**
 * Continue statement
 */
class SCRIPTING_API FContinueStmt : public FScriptStatement
{
public:
    FContinueStmt() {}
    
    virtual FString ToString() const override { return TEXT("Continue()"); }
    virtual FString GetNodeType() const override { return TEXT("Continue"); }
};

/**
 * For statement
 */
class SCRIPTING_API FForStmt : public FScriptStatement
{
public:
    TSharedPtr<FScriptStatement> Initializer;
    TSharedPtr<FScriptExpression> Condition;
    TSharedPtr<FScriptExpression> Increment;
    TSharedPtr<FScriptStatement> Body;
    
    FForStmt(TSharedPtr<FScriptStatement> InInit, 
             TSharedPtr<FScriptExpression> InCond,
             TSharedPtr<FScriptExpression> InIncr,
             TSharedPtr<FScriptStatement> InBody)
        : Initializer(InInit), Condition(InCond), Increment(InIncr), Body(InBody)
    {}
    
    virtual bool IsValid() const override
    {
        bool bValid = Body.IsValid() && Body->IsValid();
        if (Initializer.IsValid()) bValid = bValid && Initializer->IsValid();
        if (Condition.IsValid()) bValid = bValid && Condition->IsValid();
        if (Increment.IsValid()) bValid = bValid && Increment->IsValid();
        return bValid;
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("For(INVALID)");
        FString InitStr = Initializer.IsValid() ? Initializer->ToString() : TEXT("none");
        FString CondStr = Condition.IsValid() ? Condition->ToString() : TEXT("true");
        FString IncrStr = Increment.IsValid() ? Increment->ToString() : TEXT("none");
        return FString::Printf(TEXT("For(%s; %s; %s) Do(%s)"),
            *InitStr, *CondStr, *IncrStr, *Body->ToString());
    }
    
    virtual FString GetNodeType() const override { return TEXT("For"); }
};

/**
 * Function parameter with type information
 */
struct FParameter
{
    EScriptType Type;
    FScriptToken Name;
};

/**
 * Function declaration
 */
class SCRIPTING_API FFunctionDecl : public FScriptASTNode
{
public:
    FScriptToken Name;
    TArray<FScriptToken> Parameters;  // Legacy: untyped parameters
    TArray<FParameter> TypedParameters;  // Modern: typed parameters (int x, float y)
    TSharedPtr<FBlockStmt> Body;
    EScriptType ReturnType;
    
    FFunctionDecl(const FScriptToken& InName, const TArray<FScriptToken>& InParams, 
                  TSharedPtr<FBlockStmt> InBody)
        : Name(InName), Parameters(InParams), TypedParameters(), Body(InBody), ReturnType(EScriptType::VOID)
    {}
    
    virtual bool IsValid() const override
    {
        return Body.IsValid() && Body->IsValid();
    }
    
    virtual FString ToString() const override
    {
        if (!IsValid()) return TEXT("Function(INVALID)");
        
        FString ParamsStr;
        if (TypedParameters.Num() > 0)
        {
            // Use typed parameters if available
            for (int32 i = 0; i < TypedParameters.Num(); ++i)
            {
                const FParameter& P = TypedParameters[i];
                switch (P.Type)
                {
                    case EScriptType::INT: ParamsStr += TEXT("int "); break;
                    case EScriptType::FLOAT: ParamsStr += TEXT("float "); break;
                    case EScriptType::STRING: ParamsStr += TEXT("string "); break;
                    case EScriptType::VOID: ParamsStr += TEXT("void "); break;
                    default: ParamsStr += TEXT("auto "); break;
                }
                ParamsStr += P.Name.Lexeme;
                if (i < TypedParameters.Num() - 1) ParamsStr += TEXT(", ");
            }
        }
        else
        {
            // Fallback to untyped parameters
            for (int32 i = 0; i < Parameters.Num(); ++i)
            {
                ParamsStr += Parameters[i].Lexeme;
                if (i < Parameters.Num() - 1) ParamsStr += TEXT(", ");
            }
        }
        
        FString ReturnTypeStr;
        switch (ReturnType)
        {
            case EScriptType::INT: ReturnTypeStr = TEXT("int"); break;
            case EScriptType::FLOAT: ReturnTypeStr = TEXT("float"); break;
            case EScriptType::STRING: ReturnTypeStr = TEXT("string"); break;
            case EScriptType::VOID: ReturnTypeStr = TEXT("void"); break;
            default: ReturnTypeStr = TEXT("auto"); break;
        }
        
        return FString::Printf(TEXT("%s %s(%s) %s"), 
            *ReturnTypeStr, *Name.Lexeme, *ParamsStr, *Body->ToString());
    }
    
    virtual FString GetNodeType() const override { return TEXT("Function"); }
};

/**
 * Program (top-level)
 */
class SCRIPTING_API FScriptProgram : public FScriptASTNode
{
public:
    TArray<TSharedPtr<FFunctionDecl>> Functions;
    TArray<TSharedPtr<FScriptStatement>> Statements;
    
    FScriptProgram(const TArray<TSharedPtr<FFunctionDecl>>& InFuncs, 
                   const TArray<TSharedPtr<FScriptStatement>>& InStmts)
        : Functions(InFuncs), Statements(InStmts)
    {}
    
    virtual bool IsValid() const override
    {
        for (const auto& Func : Functions)
        {
            if (!Func.IsValid() || !Func->IsValid()) return false;
        }
        for (const auto& Stmt : Statements)
        {
            if (!Stmt.IsValid() || !Stmt->IsValid()) return false;
        }
        return true;
    }
    
    virtual FString ToString() const override
    {
        FString Result = TEXT("Program(\n");
        for (const auto& Func : Functions)
        {
            Result += TEXT("  ") + Func->ToString() + TEXT("\n");
        }
        for (const auto& Stmt : Statements)
        {
            Result += TEXT("  ") + Stmt->ToString() + TEXT("\n");
        }
        Result += TEXT(")");
        return Result;
    }
    
    virtual FString GetNodeType() const override { return TEXT("Program"); }
};

