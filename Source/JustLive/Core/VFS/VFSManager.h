// VFSManager.h
// Runtime Manager for the Virtual File System
// Handles mounting/unmounting of Archives and file lookups.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VFSDefinitions.h"
#include "VFSManager.generated.h"

/**
 * Handle to an open archive
 */
struct FMountedArchive
{
    FString FilePath;
    IFileHandle* FileHandle = nullptr;
    FArchiveHeader Header;
    TMap<uint64, FArchiveEntry> Index; // Hash -> Entry
    int32 Priority = 0;
};

/**
 * Virtual File System Manager
 * Allows loading assets from .jdm, .jim, .pak, etc.
 */
UCLASS()
class JUSTLIVE_API UVFSManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    static UVFSManager* Get(const UObject* WorldContextObject);

    // Subsystem Interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Mounts an archive (.jdm, .jtc, etc.)
     * @param Path - Absolute path to the file
     * @param Priority - Higher priority archives override lower ones (Modding support)
     */
    bool MountArchive(const FString& Path, int32 Priority = 0);

    /**
     * Unmounts an archive
     */
    void UnmountArchive(const FString& Path);

    /**
     * Check if a file exists in any mounted archive
     * @param VirtualPath - e.g. "Game/Weapons/AK47.uasset"
     */
    bool FileExists(const FString& VirtualPath);

    /**
     * Open a file for reading from an archive.
     * Returns a valid IFileHandle if found, or nullptr.
     * The caller owns the handle.
     */
    class IFileHandle* OpenRead(const FString& VirtualPath);

    /**
     * Resolve a virtual path to a physical path (if possible)
     * Used for loose files.
     */
    FString Resolve(const FString& VirtualPath);

    /**
     * Mount a virtual directory to a physical path
     */
    void Mount(const FString& VirtualPath, const FString& PhysicalPath);

private:
    // List of mounted archives
    TArray<TSharedPtr<FMountedArchive>> MountedArchives;
    
    // Virtual Mount Points (e.g. @data -> D:/Game/Data)
    TMap<FString, FString> MountPoints;

    // Quick lookup for file existence (Hash -> Archive Index)
    TMap<uint64, TSharedPtr<FMountedArchive>> FileLookup;

    void RebuildLookup();
    uint64 HashPath(const FString& Path) const;
};