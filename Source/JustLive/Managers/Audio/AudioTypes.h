#pragma once

#include "CoreMinimal.h"
#include "AudioTypes.generated.h"

USTRUCT(BlueprintType)
struct FSoundDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Id;

	UPROPERTY(BlueprintReadOnly)
	FString FilePath; // Virtual path or Package path

	UPROPERTY(BlueprintReadOnly)
	float Volume = 1.0f;

	UPROPERTY(BlueprintReadOnly)
	float Pitch = 0.0f; // 0 is normalized/untouched

	UPROPERTY(BlueprintReadOnly)
	float Pan = 0.0f; // -1 (Left) to 1 (Right)

	UPROPERTY(BlueprintReadOnly)
	float StartTime = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float PlayBackTime = 0.0f; // 0 = Infinite/Full Length

	UPROPERTY(BlueprintReadOnly)
	bool bSpatial = true;

	UPROPERTY(BlueprintReadOnly)
	float AttenuationRadius = 400.0f;

	UPROPERTY(BlueprintReadOnly)
	float FalloffDistance = 3600.0f;
};
