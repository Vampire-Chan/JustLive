// VFSDefinitions.h
// Definitions for the Virtual File System (VFS) and Custom Formats

#pragma once

#include "CoreMinimal.h"

// ============================================================================
// Universal Archive Header
// Used by: .jdm, .jim, .jtc, .jan, .jab, .jmv
// ============================================================================

#define ARCHIVE_MAGIC 0x4B434150 // "PACK"
#define ARCHIVE_VERSION 1

enum class EArchiveType : uint32
{
    Generic   = 0,
    MeshDict  = 1, // .jdm
    MeshIndv  = 2, // .jim
    Texture   = 3, // .jtc
    Animation = 4, // .jan
    Audio     = 5, // .jab
    Movie     = 6  // .jmv
};

struct FArchiveHeader
{
    uint32 Magic;           // "PACK"
    uint32 Version;         // 1
    uint32 FileCount;       // Number of files inside
    uint64 IndexOffset;     // Offset to the File Index
    uint32 ArchiveType;     // EArchiveType
    uint32 Reserved[3];     // Padding/Future use
};

/**
 * A single file entry inside the archive.
 * Represents a packed .uasset, .uexp, etc.
 */
struct FArchiveEntry
{
    uint64 PathHash;        // Hash of the internal virtual path (e.g., "Weapons/AK47")
    uint64 Offset;          // Absolute offset in the archive
    uint32 CompressedSize;  // Size on disk
    uint32 UncompressedSize;// Original size
    uint32 Flags;           // Compression, Encryption, etc.
};

enum EEntryFlags
{
    Entry_None = 0,
    Entry_Compressed = 1 << 0, // ZLib compressed
    Entry_Encrypted  = 1 << 1, // Simple XOR
};

// ============================================================================
// Custom Map Format (.jmap)
// Binary definition of the world (Actors, Transforms)
// ============================================================================

#define MAP_MAGIC 0x50414D4A // "JMAP"
#define MAP_VERSION 1

struct FMapHeader
{
    uint32 Magic;       // "JMAP"
    uint32 Version;
    uint32 EntityCount; // Total objects
    
    FVector BoundsMin;  // World bounds
    FVector BoundsMax;
};

struct FMapEntity
{
    uint64 GUID;        // Unique ID
    uint64 ModelHash;   // Asset ID to spawn
    
    FVector Position;
    FRotator Rotation;
    FVector Scale;
    
    uint32 Flags;       // Visibility, etc.
};
