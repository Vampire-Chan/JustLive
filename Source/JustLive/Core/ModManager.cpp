#include "ModManager.h"
#include "DataManager.h"
#include "VFS/VFSManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "XmlFile.h"
#include "Kismet/GameplayStatics.h"

UModManager* UModManager::Get(const UObject* WorldContextObject)
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		return GI->GetSubsystem<UModManager>();
	}
	return nullptr;
}

void UModManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency(UDataManager::StaticClass());
	UE_LOG(LogTemp, Log, TEXT("ModManager Initialized."));
}

UDataManager* UModManager::GetDataManager() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UDataManager>();
	}
	return nullptr;
}

void UModManager::ScanMods()
{
	KnownMods.Empty();
	
	// We use VFS to get the physical path of @mods
	UVFSManager* VFS = UVFSManager::Get(this);
	if (!VFS) return;

	FString ModsRoot = VFS->Resolve(TEXT("@mods"));
	if (ModsRoot.IsEmpty()) return;

	// Iterate directories
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	class FModVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:
		TMap<FString, FModInfo>& ModsMap;
		UDataManager* DataMgr;
		FString ModsRoot;

		FModVisitor(TMap<FString, FModInfo>& InMods, UDataManager* InDataMgr, FString InRoot) 
			: ModsMap(InMods), DataMgr(InDataMgr), ModsRoot(InRoot) {}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (bIsDirectory)
			{
				FString ModDir = FilenameOrDirectory;
				FString ModName = FPaths::GetBaseFilename(ModDir);
				FString InfoPath = FPaths::Combine(ModDir, TEXT("mod.info"));

				// Check if mod.info exists
				if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*InfoPath))
				{
					FModInfo NewMod;
					NewMod.Name = ModName;
					NewMod.RootPath = ModDir;
					NewMod.bIsActive = false;

					// Parse mod.info (assuming XML for now)
					// We construct a virtual path manually since we know it's under @mods
					FString VirtualInfoPath = FString::Printf(TEXT("@mods/%s/mod.info"), *ModName);
					
					if (DataMgr)
					{
						TSharedPtr<FXmlFile> Xml = DataMgr->ParseXML(VirtualInfoPath);
						if (Xml.IsValid() && Xml->GetRootNode())
						{
							NewMod.Version = Xml->GetRootNode()->GetAttribute(TEXT("version"));
							NewMod.Description = Xml->GetRootNode()->GetContent();
							
							FString TypeStr = Xml->GetRootNode()->GetAttribute(TEXT("type"));
							if (TypeStr.Equals(TEXT("standalone"), ESearchCase::IgnoreCase))
							{
								NewMod.Type = EModType::Standalone;
							}
							else
							{
								NewMod.Type = EModType::Addon;
							}
						}
					}

					ModsMap.Add(ModName, NewMod);
					UE_LOG(LogTemp, Log, TEXT("ModManager: Found mod '%s' (Type: %s)"), *ModName, *UEnum::GetValueAsString(NewMod.Type));
				}
			}
			return true;
		}
	};

	FModVisitor Visitor(KnownMods, GetDataManager(), ModsRoot);
	PlatformFile.IterateDirectory(*ModsRoot, Visitor);
}

bool UModManager::LoadMod(const FString& ModName)
{
	if (!KnownMods.Contains(ModName))
	{
		UE_LOG(LogTemp, Warning, TEXT("ModManager: Cannot load unknown mod '%s'"), *ModName);
		return false;
	}

	FModInfo& Mod = KnownMods[ModName];
	if (Mod.bIsActive) return true; // Already loaded

	UDataManager* DataMgr = GetDataManager();
	if (!DataMgr) return false;

	// 1. Mount Content Pak if it exists
	// Convention: @mods/ModName/content/assets.pak
	FString PakPath = FString::Printf(TEXT("@mods/%s/content/assets.pak"), *ModName);
	if (DataMgr->GetVFS()->FileExists(PakPath))
	{
		DataMgr->MountPak(PakPath);
	}

	// 2. Handle Standalone Overrides
	if (Mod.Type == EModType::Standalone)
	{
		// Override @data to point to this mod's data folder
		// @mods/ModName/data/
		FString ModDataPath = FString::Printf(TEXT("@mods/%s/data/"), *ModName);
		FString PhysicalModDataPath = DataMgr->GetVFS()->Resolve(ModDataPath);
		
		if (!PhysicalModDataPath.IsEmpty())
		{
			// Remount @data to the mod's data folder
			DataMgr->GetVFS()->Mount(TEXT("@data"), PhysicalModDataPath);
			UE_LOG(LogTemp, Warning, TEXT("ModManager: STANDALONE MOD LOADED. @data is now mapped to '%s'"), *PhysicalModDataPath);
		}
	}

	Mod.bIsActive = true;
	UE_LOG(LogTemp, Log, TEXT("ModManager: Loaded mod '%s'"), *ModName);
	return true;
}

void UModManager::UnloadMod(const FString& ModName)
{
	if (KnownMods.Contains(ModName))
	{
		KnownMods[ModName].bIsActive = false;
		// Unmounting paks is harder in runtime, usually requires restart or complex handling.
		// For now, we just mark as inactive.
		UE_LOG(LogTemp, Log, TEXT("ModManager: Unloaded mod '%s'"), *ModName);
	}
}

TArray<FModInfo> UModManager::GetAvailableMods() const
{
	TArray<FModInfo> Mods;
	KnownMods.GenerateValueArray(Mods);
	return Mods;
}
