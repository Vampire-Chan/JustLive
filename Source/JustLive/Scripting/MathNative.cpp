// MathNative.cpp
// Math native function registration for scripting system

#include "MathNative.h"
#include "ScriptLogger.h"
#include "Math/UnrealMathUtility.h"

void FMathNativeReg::RegisterFunctions(FScriptVM* VM)
{
    if (!VM) return;

    SCRIPT_LOG(TEXT("[MATH NATIVE REG] Registering math functions..."));

    // Arithmetic
    VM->RegisterNativeFunction(TEXT("Add"), Add);
    VM->RegisterNativeFunction(TEXT("Subtract"), Subtract);
    VM->RegisterNativeFunction(TEXT("Multiply"), Multiply);
    VM->RegisterNativeFunction(TEXT("Divide"), Divide);
    VM->RegisterNativeFunction(TEXT("Mod"), Mod);
    VM->RegisterNativeFunction(TEXT("Pow"), Pow);

    // Trig
    VM->RegisterNativeFunction(TEXT("Sin"), Sin);
    VM->RegisterNativeFunction(TEXT("Cos"), Cos);
    VM->RegisterNativeFunction(TEXT("Tan"), Tan);
    VM->RegisterNativeFunction(TEXT("Asin"), Asin);
    VM->RegisterNativeFunction(TEXT("Acos"), Acos);
    VM->RegisterNativeFunction(TEXT("Atan"), Atan);
    VM->RegisterNativeFunction(TEXT("Atan2"), Atan2);

    // Helpers
    VM->RegisterNativeFunction(TEXT("Abs"), Abs);
    VM->RegisterNativeFunction(TEXT("Sqrt"), Sqrt);
    VM->RegisterNativeFunction(TEXT("Floor"), Floor);
    VM->RegisterNativeFunction(TEXT("Ceil"), Ceil);
    VM->RegisterNativeFunction(TEXT("Round"), Round);
    VM->RegisterNativeFunction(TEXT("Clamp"), Clamp);
    VM->RegisterNativeFunction(TEXT("Min"), Min);
    VM->RegisterNativeFunction(TEXT("Max"), Max);
    VM->RegisterNativeFunction(TEXT("DegreesToRadians"), DegreesToRadians);
    VM->RegisterNativeFunction(TEXT("RadiansToDegrees"), RadiansToDegrees);
    VM->RegisterNativeFunction(TEXT("Log"), Log);
    VM->RegisterNativeFunction(TEXT("Exp"), Exp);

    // Random
    VM->RegisterNativeFunction(TEXT("RandomFloat"), Random_Float);
    VM->RegisterNativeFunction(TEXT("RandomRange"), Random_Range);
    VM->RegisterNativeFunction(TEXT("RandomBool"), Random_Bool);

    // Vector
    VM->RegisterNativeFunction(TEXT("Vector"), Vector);
    VM->RegisterNativeFunction(TEXT("Vector_Add"), Vector_Add);
    VM->RegisterNativeFunction(TEXT("Vector_Sub"), Vector_Sub);
    VM->RegisterNativeFunction(TEXT("Vector_Mul"), Vector_Mul);
    VM->RegisterNativeFunction(TEXT("Vector_Div"), Vector_Div);
    VM->RegisterNativeFunction(TEXT("Vector_Dot"), Vector_Dot);
    VM->RegisterNativeFunction(TEXT("Vector_Cross"), Vector_Cross);
    VM->RegisterNativeFunction(TEXT("Vector_Dist"), Vector_Dist);
    VM->RegisterNativeFunction(TEXT("Vector_DistSquared"), Vector_DistSquared);
    VM->RegisterNativeFunction(TEXT("Vector_Normalize"), Vector_Normalize);
    VM->RegisterNativeFunction(TEXT("Vector_Length"), Vector_Length);
    VM->RegisterNativeFunction(TEXT("Vector_Lerp"), Vector_Lerp);

    SCRIPT_LOG(TEXT("[MATH NATIVE REG] Registered math functions"));
}

//=============================================================================
// Basic arithmetic
//=============================================================================

FScriptValue FMathNativeReg::Add(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2 || !Args[0].IsNumber() || !Args[1].IsNumber())
    {
        VM->RuntimeError(TEXT("Add requires 2 numeric arguments"));
        return FScriptValue::Number(0);
    }
    return FScriptValue::Number(Args[0].AsNumber() + Args[1].AsNumber());
}

FScriptValue FMathNativeReg::Subtract(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2 || !Args[0].IsNumber() || !Args[1].IsNumber())
    {
        VM->RuntimeError(TEXT("Subtract requires 2 numeric arguments"));
        return FScriptValue::Number(0);
    }
    return FScriptValue::Number(Args[0].AsNumber() - Args[1].AsNumber());
}

FScriptValue FMathNativeReg::Multiply(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2 || !Args[0].IsNumber() || !Args[1].IsNumber())
    {
        VM->RuntimeError(TEXT("Multiply requires 2 numeric arguments"));
        return FScriptValue::Number(0);
    }
    return FScriptValue::Number(Args[0].AsNumber() * Args[1].AsNumber());
}

FScriptValue FMathNativeReg::Divide(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2 || !Args[0].IsNumber() || !Args[1].IsNumber())
    {
        VM->RuntimeError(TEXT("Divide requires 2 numeric arguments"));
        return FScriptValue::Number(0);
    }
    if (Args[1].AsNumber() == 0.0) { VM->RuntimeError(TEXT("Divide by zero")); return FScriptValue::Number(0); }
    return FScriptValue::Number(Args[0].AsNumber() / Args[1].AsNumber());
}

FScriptValue FMathNativeReg::Mod(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2 || !Args[0].IsNumber() || !Args[1].IsNumber())
    {
        VM->RuntimeError(TEXT("Mod requires 2 numeric arguments"));
        return FScriptValue::Number(0);
    }
    if (Args[1].AsNumber() == 0.0) { VM->RuntimeError(TEXT("Mod by zero")); return FScriptValue::Number(0); }
    return FScriptValue::Number(FMath::Fmod(Args[0].AsNumber(), Args[1].AsNumber()));
}

FScriptValue FMathNativeReg::Pow(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2 || !Args[0].IsNumber() || !Args[1].IsNumber())
    {
        VM->RuntimeError(TEXT("Pow requires 2 numeric arguments"));
        return FScriptValue::Number(0);
    }
    return FScriptValue::Number(FMath::Pow(Args[0].AsNumber(), Args[1].AsNumber()));
}

//=============================================================================
// Trig
//=============================================================================

FScriptValue FMathNativeReg::Sin(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::Sin(Args[0].AsNumber()));
}

FScriptValue FMathNativeReg::Cos(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::Cos(Args[0].AsNumber()));
}

FScriptValue FMathNativeReg::Tan(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::Tan(Args[0].AsNumber()));
}

FScriptValue FMathNativeReg::Asin(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::Asin(Args[0].AsNumber()));
}

FScriptValue FMathNativeReg::Acos(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::Acos(Args[0].AsNumber()));
}

FScriptValue FMathNativeReg::Atan(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::Atan(Args[0].AsNumber()));
}

FScriptValue FMathNativeReg::Atan2(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2 || !Args[0].IsNumber() || !Args[1].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::Atan2(Args[0].AsNumber(), Args[1].AsNumber()));
}

//=============================================================================
// Helpers
//=============================================================================

FScriptValue FMathNativeReg::Abs(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::Abs(Args[0].AsNumber()));
}

FScriptValue FMathNativeReg::Sqrt(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    double v = Args[0].AsNumber(); 
    if (v < 0.0) { VM->RuntimeError(TEXT("Sqrt negative input")); return FScriptValue::Number(0); }
    return FScriptValue::Number(FMath::Sqrt(v));
}

FScriptValue FMathNativeReg::Floor(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::FloorToDouble(Args[0].AsNumber()));
}

FScriptValue FMathNativeReg::Ceil(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::CeilToDouble(Args[0].AsNumber()));
}

FScriptValue FMathNativeReg::Round(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::RoundToDouble(Args[0].AsNumber()));
}

FScriptValue FMathNativeReg::Clamp(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 3 || !Args[0].IsNumber() || !Args[1].IsNumber() || !Args[2].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::Clamp(Args[0].AsNumber(), Args[1].AsNumber(), Args[2].AsNumber()));
}

FScriptValue FMathNativeReg::Min(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2 || !Args[0].IsNumber() || !Args[1].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::Min(Args[0].AsNumber(), Args[1].AsNumber()));
}

FScriptValue FMathNativeReg::Max(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2 || !Args[0].IsNumber() || !Args[1].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::Max(Args[0].AsNumber(), Args[1].AsNumber()));
}

FScriptValue FMathNativeReg::DegreesToRadians(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::DegreesToRadians(Args[0].AsNumber()));
}

FScriptValue FMathNativeReg::RadiansToDegrees(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::RadiansToDegrees(Args[0].AsNumber()));
}

FScriptValue FMathNativeReg::Log(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::Loge(Args[0].AsNumber()));
}

FScriptValue FMathNativeReg::Exp(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1 || !Args[0].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::Exp(Args[0].AsNumber()));
}

//=============================================================================
// Random
//=============================================================================

FScriptValue FMathNativeReg::Random_Float(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    return FScriptValue::Number(FMath::FRand());
}

FScriptValue FMathNativeReg::Random_Range(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2 || !Args[0].IsNumber() || !Args[1].IsNumber()) return FScriptValue::Number(0);
    return FScriptValue::Number(FMath::RandRange(Args[0].AsNumber(), Args[1].AsNumber()));
}

FScriptValue FMathNativeReg::Random_Bool(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    return FScriptValue::Bool(FMath::RandBool());
}

//=============================================================================
// Vector Math
//=============================================================================

FScriptValue FMathNativeReg::Vector(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    double X = (Args.Num() > 0) ? Args[0].AsNumber() : 0.0;
    double Y = (Args.Num() > 1) ? Args[1].AsNumber() : 0.0;
    double Z = (Args.Num() > 2) ? Args[2].AsNumber() : 0.0;
    
    TArray<FScriptValue> Vec;
    Vec.Add(FScriptValue::Number(X));
    Vec.Add(FScriptValue::Number(Y));
    Vec.Add(FScriptValue::Number(Z));
    return FScriptValue::Array(Vec);
}

FVector FMathNativeReg::GetVectorFromArray(const FScriptValue& Val)
{
    if (!Val.IsArray() || Val.ArrayValue.Num() < 3) return FVector::ZeroVector;
    return FVector(
        Val.ArrayValue[0].AsNumber(),
        Val.ArrayValue[1].AsNumber(),
        Val.ArrayValue[2].AsNumber()
    );
}

FScriptValue FMathNativeReg::CreateArrayFromVector(const FVector& Vec)
{
    TArray<FScriptValue> Arr;
    Arr.Add(FScriptValue::Number(Vec.X));
    Arr.Add(FScriptValue::Number(Vec.Y));
    Arr.Add(FScriptValue::Number(Vec.Z));
    return FScriptValue::Array(Arr);
}

FScriptValue FMathNativeReg::Vector_Add(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Nil();
    FVector V1 = GetVectorFromArray(Args[0]);
    FVector V2 = GetVectorFromArray(Args[1]);
    return CreateArrayFromVector(V1 + V2);
}

FScriptValue FMathNativeReg::Vector_Sub(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Nil();
    FVector V1 = GetVectorFromArray(Args[0]);
    FVector V2 = GetVectorFromArray(Args[1]);
    return CreateArrayFromVector(V1 - V2);
}

FScriptValue FMathNativeReg::Vector_Mul(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Nil();
    FVector V1 = GetVectorFromArray(Args[0]);
    double Scalar = Args[1].AsNumber();
    return CreateArrayFromVector(V1 * Scalar);
}

FScriptValue FMathNativeReg::Vector_Div(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Nil();
    FVector V1 = GetVectorFromArray(Args[0]);
    double Scalar = Args[1].AsNumber();
    if (Scalar == 0.0) return FScriptValue::Nil();
    return CreateArrayFromVector(V1 / Scalar);
}

FScriptValue FMathNativeReg::Vector_Dot(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Number(0);
    FVector V1 = GetVectorFromArray(Args[0]);
    FVector V2 = GetVectorFromArray(Args[1]);
    return FScriptValue::Number(FVector::DotProduct(V1, V2));
}

FScriptValue FMathNativeReg::Vector_Cross(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Nil();
    FVector V1 = GetVectorFromArray(Args[0]);
    FVector V2 = GetVectorFromArray(Args[1]);
    return CreateArrayFromVector(FVector::CrossProduct(V1, V2));
}

FScriptValue FMathNativeReg::Vector_Dist(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Number(0);
    FVector V1 = GetVectorFromArray(Args[0]);
    FVector V2 = GetVectorFromArray(Args[1]);
    return FScriptValue::Number(FVector::Dist(V1, V2));
}

FScriptValue FMathNativeReg::Vector_DistSquared(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 2) return FScriptValue::Number(0);
    FVector V1 = GetVectorFromArray(Args[0]);
    FVector V2 = GetVectorFromArray(Args[1]);
    return FScriptValue::Number(FVector::DistSquared(V1, V2));
}

FScriptValue FMathNativeReg::Vector_Normalize(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Nil();
    FVector V1 = GetVectorFromArray(Args[0]);
    V1.Normalize();
    return CreateArrayFromVector(V1);
}

FScriptValue FMathNativeReg::Vector_Length(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 1) return FScriptValue::Number(0);
    FVector V1 = GetVectorFromArray(Args[0]);
    return FScriptValue::Number(V1.Size());
}

FScriptValue FMathNativeReg::Vector_Lerp(FScriptVM* VM, const TArray<FScriptValue>& Args)
{
    if (Args.Num() < 3) return FScriptValue::Nil();
    FVector V1 = GetVectorFromArray(Args[0]);
    FVector V2 = GetVectorFromArray(Args[1]);
    float Alpha = (float)Args[2].AsNumber();
    return CreateArrayFromVector(FMath::Lerp(V1, V2, Alpha));
}