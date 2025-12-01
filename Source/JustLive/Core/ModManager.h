#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ModManager.generated.h"

UENUM(BlueprintType)
enum class EModType : uint8
{
	Addon,
	Standalone
};

USTRUCT(BlueprintType)
struct FModInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Name;

	UPROPERTY(BlueprintReadOnly)
	FString Version;

	UPROPERTY(BlueprintReadOnly)
	FString Description;

	UPROPERTY(BlueprintReadOnly)
	EModType Type = EModType::Addon;

	UPROPERTY(BlueprintReadOnly)
	bool bIsActive = false;

	FString RootPath; // e.g., "D:/Project/Mods/MyMod/"
};

/**
 * Mod Manager
 * Handles discovery, loading, and unloading of mods.
 */
UCLASS()
class JUSTLIVE_API UModManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UModManager* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Scans the Mods directory for valid mods.
	 */
	UFUNCTION(BlueprintCallable, Category = "Modding")
	void ScanMods();

	/**
	 * Loads a specific mod.
	 */
	UFUNCTION(BlueprintCallable, Category = "Modding")
	bool LoadMod(const FString& ModName);

	/**
	 * Unloads a specific mod.
	 */
	UFUNCTION(BlueprintCallable, Category = "Modding")
	void UnloadMod(const FString& ModName);

	UFUNCTION(BlueprintCallable, Category = "Modding")
	TArray<FModInfo> GetAvailableMods() const;

private:
	TMap<FString, FModInfo> KnownMods;

	class UDataManager* GetDataManager() const;
};
