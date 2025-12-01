#include "DataManager.h"
#include "VFS/VFSManager.h"
#include "ModManager.h"
#include "Misc/CoreDelegates.h"
#include "IPlatformFilePak.h"
#include "Kismet/GameplayStatics.h"

UDataManager* UDataManager::Get(const UObject* WorldContextObject)
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		return GI->GetSubsystem<UDataManager>();
	}
	return nullptr;
}

void UDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Ensure VFS is initialized (dependency)
	Collection.InitializeDependency(UVFSManager::StaticClass());
	UE_LOG(LogTemp, Log, TEXT("DataManager Initialized."));
}

void UDataManager::Boot()
{
	UE_LOG(LogTemp, Log, TEXT("DataManager: Booting System..."));

	// 1. Scan for Mods
	if (UModManager* ModMgr = UModManager::Get(this))
	{
		ModMgr->ScanMods();
	}

	// 2. Load Core Data (e.g., @data/config.xml) - Placeholder
	// ParseXML(TEXT("@data/config.xml"));
}

UVFSManager* UDataManager::GetVFS() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<UVFSManager>();
	}
	return nullptr;
}

TSharedPtr<FXmlFile> UDataManager::ParseXML(const FString& VirtualPath)
{
	UVFSManager* VFS = GetVFS();
	if (!VFS) return nullptr;

	FString PhysicalPath = VFS->Resolve(VirtualPath);
	if (PhysicalPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("DataManager: Failed to resolve XML path '%s'"), *VirtualPath);
		return nullptr;
	}

	if (!VFS->FileExists(VirtualPath))
	{
		UE_LOG(LogTemp, Error, TEXT("DataManager: XML file not found at '%s'"), *PhysicalPath);
		return nullptr;
	}

	TSharedPtr<FXmlFile> XmlFile = MakeShareable(new FXmlFile(PhysicalPath));
	if (!XmlFile->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("DataManager: Failed to parse XML at '%s'"), *PhysicalPath);
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("DataManager: Successfully parsed '%s'"), *VirtualPath);
	return XmlFile;
}

bool UDataManager::MountPak(const FString& VirtualPath)
{
	UVFSManager* VFS = GetVFS();
	if (!VFS) return false;

	FString PhysicalPath = VFS->Resolve(VirtualPath);
	if (PhysicalPath.IsEmpty()) return false;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (FPakPlatformFile* PakPlatformFile = (FPakPlatformFile*)(FPlatformFileManager::Get().FindPlatformFile(TEXT("PakFile"))))
	{
		// Mount the pak at the root (or we could specify a mount point)
		// For now, we assume the pak has correct internal paths
		bool bSuccess = PakPlatformFile->Mount(*PhysicalPath, 0, nullptr);
		if (bSuccess)
		{
			UE_LOG(LogTemp, Log, TEXT("DataManager: Mounted Pak '%s'"), *VirtualPath);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("DataManager: Failed to mount Pak '%s'"), *PhysicalPath);
		}
		return bSuccess;
	}

	UE_LOG(LogTemp, Error, TEXT("DataManager: PakFile platform file not found!"));
	return false;
}
