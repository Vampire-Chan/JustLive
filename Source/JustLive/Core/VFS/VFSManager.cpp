// VFSManager.cpp
// Runtime Manager for the Virtual File System

#include "VFSManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Kismet/GameplayStatics.h"
#include "Hash/CityHash.h"

UVFSManager* UVFSManager::Get(const UObject* WorldContextObject)
{
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject))
    {
        return GI->GetSubsystem<UVFSManager>();
    }
    return nullptr;
}

void UVFSManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("VFS Manager Initialized"));
}

void UVFSManager::Deinitialize()
{
    for (auto& Archive : MountedArchives)
    {
        if (Archive->FileHandle) delete Archive->FileHandle;
    }
    MountedArchives.Empty();
    FileLookup.Empty();
    Super::Deinitialize();
}

uint64 UVFSManager::HashPath(const FString& Path) const
{
    FString NormPath = Path.ToLower();
    NormPath.ReplaceInline(TEXT("\\"), TEXT("/"));
    return CityHash64((char*)*NormPath, NormPath.Len() * sizeof(TCHAR));
}

bool UVFSManager::MountArchive(const FString& Path, int32 Priority)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    if (!PlatformFile.FileExists(*Path))
    {
        return false;
    }

    IFileHandle* Handle = PlatformFile.OpenRead(*Path);
    if (!Handle) return false;

    TSharedPtr<FMountedArchive> Archive = MakeShared<FMountedArchive>();
    Archive->FilePath = Path;
    Archive->Priority = Priority;
    Archive->FileHandle = Handle;

    // Read Header
    Handle->Read((uint8*)&Archive->Header, sizeof(FArchiveHeader));

    if (Archive->Header.Magic != ARCHIVE_MAGIC)
    {
        UE_LOG(LogTemp, Error, TEXT("VFS: Invalid Magic in %s"), *Path);
        delete Handle;
        return false;
    }

    // Seek to Index
    Handle->Seek(Archive->Header.IndexOffset);

    // Read Index entries
    for (uint32 i = 0; i < Archive->Header.FileCount; ++i)
    {
        FArchiveEntry Entry;
        Handle->Read((uint8*)&Entry, sizeof(FArchiveEntry));
        Archive->Index.Add(Entry.PathHash, Entry);
    }

    MountedArchives.Add(Archive);
    RebuildLookup();

    UE_LOG(LogTemp, Log, TEXT("VFS: Mounted %s (%d files)"), *Path, Archive->Header.FileCount);
    return true;
}

void UVFSManager::UnmountArchive(const FString& Path)
{
    for (int32 i = 0; i < MountedArchives.Num(); ++i)
    {
        if (MountedArchives[i]->FilePath == Path)
        {
            if (MountedArchives[i]->FileHandle)
            {
                delete MountedArchives[i]->FileHandle;
            }
            MountedArchives.RemoveAt(i);
            RebuildLookup();
            return;
        }
    }
}

void UVFSManager::RebuildLookup()
{
    FileLookup.Empty();
    MountedArchives.Sort([](const TSharedPtr<FMountedArchive>& A, const TSharedPtr<FMountedArchive>& B) {
        return A->Priority < B->Priority;
    });

    for (const auto& Archive : MountedArchives)
    {
        for (const auto& Pair : Archive->Index)
        {
            FileLookup.Add(Pair.Key, Archive);
        }
    }
}

bool UVFSManager::FileExists(const FString& VirtualPath)
{
    // Check physical mount points first
    FString PhysicalPath = Resolve(VirtualPath);
    if (IPlatformFile::GetPlatformPhysical().FileExists(*PhysicalPath))
    {
        return true;
    }

    // Check archives
    uint64 Hash = HashPath(VirtualPath);
    return FileLookup.Contains(Hash);
}

// Simple Mount Point Logic (Restored)
void UVFSManager::Mount(const FString& VirtualPath, const FString& PhysicalPath)
{
    MountPoints.Add(VirtualPath, PhysicalPath);
}

FString UVFSManager::Resolve(const FString& VirtualPath)
{
    FString Path = VirtualPath;
    for (const auto& Pair : MountPoints)
    {
        if (Path.StartsWith(Pair.Key))
        {
            return Path.Replace(*Pair.Key, *Pair.Value);
        }
    }
    return Path;
}

class FArchiveReadStream : public IFileHandle
{
public:
    FArchiveReadStream(IFileHandle* InSource, const FArchiveEntry& InEntry)
        : SourceHandle(InSource), Entry(InEntry), CurrentPos(0)
    {
    }

    virtual int64 Tell() override { return CurrentPos; }
    
    virtual bool Seek(int64 NewPosition) override 
    { 
        if (NewPosition < 0 || NewPosition > Entry.UncompressedSize) return false;
        CurrentPos = NewPosition;
        return true;
    }
    
    virtual bool SeekFromEnd(int64 NewPositionRelativeToEnd) override
    {
        return Seek(Entry.UncompressedSize + NewPositionRelativeToEnd);
    }
    
    virtual bool Read(uint8* Destination, int64 BytesToRead) override
    {
        // Bounds check
        if (CurrentPos + BytesToRead > Entry.UncompressedSize)
        {
            BytesToRead = Entry.UncompressedSize - CurrentPos;
        }
        
        if (BytesToRead <= 0) return false;

        // TODO: Handle Compression! 
        // For now, assume UncompressedSize == CompressedSize (No Compression)
        
        SourceHandle->Seek(Entry.Offset + CurrentPos);
        bool bSuccess = SourceHandle->Read(Destination, BytesToRead);
        
        if (bSuccess) CurrentPos += BytesToRead;
        return bSuccess;
    }

    // Required by IFileHandle
    virtual bool ReadAt(uint8* Destination, int64 BytesToRead, int64 Offset) override
    {
        int64 OldPos = CurrentPos;
        if (!Seek(Offset)) return false;
        bool bSuccess = Read(Destination, BytesToRead);
        CurrentPos = OldPos; // Restore position
        return bSuccess;
    }
    
    virtual bool Write(const uint8* Source, int64 BytesToWrite) override { return false; }
    virtual bool Flush(const bool bFullFlush = false) override { return false; }
    virtual bool Truncate(int64 NewSize) override { return false; }
    virtual int64 Size() override { return Entry.UncompressedSize; }

private:
    IFileHandle* SourceHandle;
    FArchiveEntry Entry;
    int64 CurrentPos;
};

IFileHandle* UVFSManager::OpenRead(const FString& VirtualPath)
{
    // Check physical first
    FString PhysicalPath = Resolve(VirtualPath);
    if (IPlatformFile::GetPlatformPhysical().FileExists(*PhysicalPath))
    {
        return IPlatformFile::GetPlatformPhysical().OpenRead(*PhysicalPath);
    }

    // Check archives
    uint64 Hash = HashPath(VirtualPath);
    if (TSharedPtr<FMountedArchive>* ArchivePtr = FileLookup.Find(Hash))
    {
        FMountedArchive* Archive = ArchivePtr->Get();
        if (const FArchiveEntry* Entry = Archive->Index.Find(Hash))
        {
            return new FArchiveReadStream(Archive->FileHandle, *Entry);
        }
    }
    return nullptr;
}