// MathNative.h
// Math native function registration for scripting system

#pragma once

#include "CoreMinimal.h"
#include "ScriptVM.h"

/**
 * Math native function registration
 */
class JUSTLIVE_API FMathNativeReg
{
public:
    static void RegisterFunctions(FScriptVM* VM);

private:
    // Basic arithmetic
    static FScriptValue Add(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Subtract(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Multiply(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Divide(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Mod(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Pow(FScriptVM* VM, const TArray<FScriptValue>& Args);

    // Trig
    static FScriptValue Sin(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Cos(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Tan(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Asin(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Acos(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Atan(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Atan2(FScriptVM* VM, const TArray<FScriptValue>& Args);

    // Helpers
    static FScriptValue Abs(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Sqrt(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Floor(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Ceil(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Round(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Clamp(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Min(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Max(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue DegreesToRadians(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue RadiansToDegrees(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Log(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Exp(FScriptVM* VM, const TArray<FScriptValue>& Args);

    // Random
    static FScriptValue Random_Float(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Random_Range(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Random_Bool(FScriptVM* VM, const TArray<FScriptValue>& Args);

    // Vector Math (Vectors are Arrays [x, y, z])
    static FScriptValue Vector(FScriptVM* VM, const TArray<FScriptValue>& Args); // Constructor: Vector(x, y, z)
    static FScriptValue Vector_Add(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Vector_Sub(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Vector_Mul(FScriptVM* VM, const TArray<FScriptValue>& Args); // Scalar multiply
    static FScriptValue Vector_Div(FScriptVM* VM, const TArray<FScriptValue>& Args); // Scalar divide
    static FScriptValue Vector_Dot(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Vector_Cross(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Vector_Dist(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Vector_DistSquared(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Vector_Normalize(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Vector_Length(FScriptVM* VM, const TArray<FScriptValue>& Args);
    static FScriptValue Vector_Lerp(FScriptVM* VM, const TArray<FScriptValue>& Args);
    
    // Helper: Extract vector from array value
    static FVector GetVectorFromArray(const FScriptValue& Val);
    // Helper: Create array value from vector
    static FScriptValue CreateArrayFromVector(const FVector& Vec);
};