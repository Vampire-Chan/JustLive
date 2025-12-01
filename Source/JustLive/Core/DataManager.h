#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "XmlFile.h"
#include "DataManager.generated.h"

/**
 * Data Manager
 * Central hub for data operations. Handles XML parsing and Pak mounting.
 */
UCLASS()
class JUSTLIVE_API UDataManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UDataManager* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Starts the game data boot sequence.
	 * Scans mods, loads initial configs, etc.
	 */
	UFUNCTION(BlueprintCallable, Category = "Data")
	void Boot();

	/**
	 * Parses an XML file from a virtual path.
	 * @param VirtualPath - e.g., "@data/config.xml"
	 * @return Shared pointer to FXmlFile, or nullptr if failed.
	 */
	TSharedPtr<FXmlFile> ParseXML(const FString& VirtualPath);

	/**
	 * Mounts a Pak file from a virtual path.
	 * @param VirtualPath - e.g., "@mods/MyMod/content/assets.pak"
	 * @return true if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Data")
	bool MountPak(const FString& VirtualPath);

	// Helper to get VFS
	class UVFSManager* GetVFS() const;

private:
};
