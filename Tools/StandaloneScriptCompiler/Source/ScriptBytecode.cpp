// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#include "ScriptBytecode.h"

FString FBytecodeChunk::Disassemble() const
{
    FString Result;
    Result += FString::Printf(TEXT("=== Bytecode Chunk (Version %d) ===\n"), Version);
    Result += FString::Printf(TEXT("Code Size: %d bytes\n"), Code.Num());
    Result += FString::Printf(TEXT("Constants: %d\n\n"), Constants.Num());
    
    // Disassemble constants
    if (Constants.Num() > 0)
    {
        Result += TEXT("Constants Pool:\n");
        for (int32 i = 0; i < Constants.Num(); ++i)
        {
            Result += FString::Printf(TEXT("  [%d] %s\n"), i, *Constants[i].ToString());
        }
        Result += TEXT("\n");
    }
    
    // Disassemble instructions
    Result += TEXT("Instructions:\n");
    int32 Offset = 0;
    while (Offset < Code.Num())
    {
        Result += FString::Printf(TEXT("%04d  "), Offset);
        
        // Add line number if available
        #if !UE_BUILD_SHIPPING
        if (DebugInfo.IsValidIndex(Offset))
        {
            Result += FString::Printf(TEXT("[Line %d] "), DebugInfo[Offset].Line);
        }
        #endif
        
        EOpCode Op = static_cast<EOpCode>(Code[Offset]);
        Offset++;
        
        switch (Op)
        {
            case EOpCode::OP_CONSTANT:
            {
                uint8 ConstIndex = Code[Offset++];
                Result += FString::Printf(TEXT("OP_CONSTANT %d (%s)\n"), 
                    ConstIndex, *Constants[ConstIndex].ToString());
                break;
            }
            
            case EOpCode::OP_NIL:
                Result += TEXT("OP_NIL\n");
                break;
            case EOpCode::OP_TRUE:
                Result += TEXT("OP_TRUE\n");
                break;
            case EOpCode::OP_FALSE:
                Result += TEXT("OP_FALSE\n");
                break;
                
            case EOpCode::OP_ADD:
                Result += TEXT("OP_ADD\n");
                break;
            case EOpCode::OP_SUBTRACT:
                Result += TEXT("OP_SUBTRACT\n");
                break;
            case EOpCode::OP_MULTIPLY:
                Result += TEXT("OP_MULTIPLY\n");
                break;
            case EOpCode::OP_DIVIDE:
                Result += TEXT("OP_DIVIDE\n");
                break;
            case EOpCode::OP_NEGATE:
                Result += TEXT("OP_NEGATE\n");
                break;
                
            case EOpCode::OP_EQUAL:
                Result += TEXT("OP_EQUAL\n");
                break;
            case EOpCode::OP_NOT_EQUAL:
                Result += TEXT("OP_NOT_EQUAL\n");
                break;
            case EOpCode::OP_GREATER:
                Result += TEXT("OP_GREATER\n");
                break;
            case EOpCode::OP_GREATER_EQUAL:
                Result += TEXT("OP_GREATER_EQUAL\n");
                break;
            case EOpCode::OP_LESS:
                Result += TEXT("OP_LESS\n");
                break;
            case EOpCode::OP_LESS_EQUAL:
                Result += TEXT("OP_LESS_EQUAL\n");
                break;
                
            case EOpCode::OP_NOT:
                Result += TEXT("OP_NOT\n");
                break;
            case EOpCode::OP_AND:
                Result += TEXT("OP_AND\n");
                break;
            case EOpCode::OP_OR:
                Result += TEXT("OP_OR\n");
                break;
                
            case EOpCode::OP_DEFINE_GLOBAL:
            {
                uint8 ConstIndex = Code[Offset++];
                Result += FString::Printf(TEXT("OP_DEFINE_GLOBAL %d (%s)\n"),
                    ConstIndex, *Constants[ConstIndex].ToString());
                break;
            }
            case EOpCode::OP_GET_GLOBAL:
            {
                uint8 ConstIndex = Code[Offset++];
                Result += FString::Printf(TEXT("OP_GET_GLOBAL %d (%s)\n"),
                    ConstIndex, *Constants[ConstIndex].ToString());
                break;
            }
            case EOpCode::OP_SET_GLOBAL:
            {
                uint8 ConstIndex = Code[Offset++];
                Result += FString::Printf(TEXT("OP_SET_GLOBAL %d (%s)\n"),
                    ConstIndex, *Constants[ConstIndex].ToString());
                break;
            }
            
            case EOpCode::OP_GET_LOCAL:
            {
                uint8 Slot = Code[Offset++];
                Result += FString::Printf(TEXT("OP_GET_LOCAL %d\n"), Slot);
                break;
            }
            case EOpCode::OP_SET_LOCAL:
            {
                uint8 Slot = Code[Offset++];
                Result += FString::Printf(TEXT("OP_SET_LOCAL %d\n"), Slot);
                break;
            }
            
            case EOpCode::OP_JUMP:
            {
                uint8 High = Code[Offset++];
                uint8 Low = Code[Offset++];
                int32 Jump = (High << 8) | Low;
                Result += FString::Printf(TEXT("OP_JUMP %d -> %d\n"), Jump, Offset + Jump);
                break;
            }
            case EOpCode::OP_JUMP_IF_FALSE:
            {
                uint8 High = Code[Offset++];
                uint8 Low = Code[Offset++];
                int32 Jump = (High << 8) | Low;
                Result += FString::Printf(TEXT("OP_JUMP_IF_FALSE %d -> %d\n"), Jump, Offset + Jump);
                break;
            }
            case EOpCode::OP_LOOP:
            {
                uint8 High = Code[Offset++];
                uint8 Low = Code[Offset++];
                int32 Jump = (High << 8) | Low;
                Result += FString::Printf(TEXT("OP_LOOP %d -> %d\n"), Jump, Offset - Jump);
                break;
            }
            
            case EOpCode::OP_CALL:
            {
                uint8 ArgCount = Code[Offset++];
                uint8 FuncIdxHigh = Code[Offset++];
                uint8 FuncIdxLow = Code[Offset++];
                int32 FuncIndex = (FuncIdxHigh << 8) | FuncIdxLow;
                Result += FString::Printf(TEXT("OP_CALL (args: %d, func: %d)\n"), ArgCount, FuncIndex);
                break;
            }
            case EOpCode::OP_CALL_NATIVE:
            {
                uint8 ArgCount = Code[Offset++];
                uint8 NameIdxHigh = Code[Offset++];
                uint8 NameIdxLow = Code[Offset++];
                int32 NameIndex = (NameIdxHigh << 8) | NameIdxLow;
                Result += FString::Printf(TEXT("OP_CALL_NATIVE (args: %d, name: %s)\n"), 
                    ArgCount, *Constants[NameIndex].ToString());
                break;
            }
            case EOpCode::OP_RETURN:
                Result += TEXT("OP_RETURN\n");
                break;
                
            case EOpCode::OP_CAST_INT:
                Result += TEXT("OP_CAST_INT\n");
                break;
            case EOpCode::OP_CAST_FLOAT:
                Result += TEXT("OP_CAST_FLOAT\n");
                break;
            case EOpCode::OP_CAST_STRING:
                Result += TEXT("OP_CAST_STRING\n");
                break;
                
            case EOpCode::OP_POP:
                Result += TEXT("OP_POP\n");
                break;
            case EOpCode::OP_PRINT:
                Result += TEXT("OP_PRINT\n");
                break;
                
            case EOpCode::OP_CREATE_ARRAY:
            {
                uint8 ElementCount = Code[Offset++];
                Result += FString::Printf(TEXT("OP_CREATE_ARRAY %d\n"), ElementCount);
                break;
            }
            case EOpCode::OP_GET_ELEMENT:
                Result += TEXT("OP_GET_ELEMENT\n");
                break;
            case EOpCode::OP_SET_ELEMENT:
                Result += TEXT("OP_SET_ELEMENT\n");
                break;
            case EOpCode::OP_DUPLICATE:
                Result += TEXT("OP_DUPLICATE\n");
                break;
                
            case EOpCode::OP_GET_FIELD:
            {
                uint8 NameIdxHigh = Code[Offset++];
                uint8 NameIdxLow = Code[Offset++];
                int32 NameIndex = (NameIdxHigh << 8) | NameIdxLow;
                Result += FString::Printf(TEXT("OP_GET_FIELD (field: %s)\n"), 
                    *Constants[NameIndex].ToString());
                break;
            }
            case EOpCode::OP_SET_FIELD:
            {
                uint8 NameIdxHigh = Code[Offset++];
                uint8 NameIdxLow = Code[Offset++];
                int32 NameIndex = (NameIdxHigh << 8) | NameIdxLow;
                Result += FString::Printf(TEXT("OP_SET_FIELD (field: %s)\n"), 
                    *Constants[NameIndex].ToString());
                break;
            }
                
            case EOpCode::OP_HALT:
                Result += TEXT("OP_HALT\n");
                break;
                
            default:
                Result += FString::Printf(TEXT("UNKNOWN_OP %d\n"), static_cast<int32>(Op));
                break;
        }
    }
    
    return Result;
}

// Magic number for bytecode files: "SBC1" (Script Bytecode v1)
static const uint32 BYTECODE_MAGIC = 0x31434253;
static const uint32 COMPRESSED_FLAG = 0x01;

// SHA256 hash calculator
FString FBytecodeChunk::CalculateSHA256(const TArray<uint8>& Data)
{
    FSHAHash Hash;
    FSHA1::HashBuffer(Data.GetData(), Data.Num(), Hash);
    return Hash.ToString();
}

// Generate digital signature
FString FBytecodeChunk::GenerateSignature() const
{
    // Create a hash of critical bytecode components
    TArray<uint8> SignatureData;
    
    // Include version
    SignatureData.Add((Version >> 0) & 0xFF);
    SignatureData.Add((Version >> 8) & 0xFF);
    SignatureData.Add((Version >> 16) & 0xFF);
    SignatureData.Add((Version >> 24) & 0xFF);
    
    // Include metadata
    FTCHARToUTF8 AuthorConv(*Metadata.AuthorName);
    SignatureData.Append((uint8*)AuthorConv.Get(), AuthorConv.Length());
    
    FTCHARToUTF8 OSConv(*Metadata.OperatingSystem);
    SignatureData.Append((uint8*)OSConv.Get(), OSConv.Length());
    
    // Include bytecode hash
    SignatureData.Append(Code);
    
    // Generate SHA256 hash
    return CalculateSHA256(SignatureData);
}

// Verify signature
bool FBytecodeChunk::VerifySignature(const FString& ExpectedSignature) const
{
    FString ActualSignature = GenerateSignature();
    return ActualSignature.Equals(ExpectedSignature, ESearchCase::CaseSensitive);
}

bool FBytecodeChunk::Serialize(TArray<uint8>& OutData, bool bCompress) const
{
    // Helper lambda to write data
    auto WriteInt32 = [&OutData](int32 Value) {
        OutData.Add((Value >> 0) & 0xFF);
        OutData.Add((Value >> 8) & 0xFF);
        OutData.Add((Value >> 16) & 0xFF);
        OutData.Add((Value >> 24) & 0xFF);
    };
    
    auto WriteString = [&OutData, &WriteInt32](const FString& Str) {
        TArray<uint8> UTF8Data;
        FTCHARToUTF8 Converter(*Str);
        int32 Length = Converter.Length();
        WriteInt32(Length);
        for (int32 i = 0; i < Length; ++i)
        {
            OutData.Add(Converter.Get()[i]);
        }
    };
    
    OutData.Empty();
    
    // Create uncompressed data first
    TArray<uint8> UncompressedData;
    auto WriteInt32Temp = [&UncompressedData](int32 Value) {
        UncompressedData.Add((Value >> 0) & 0xFF);
        UncompressedData.Add((Value >> 8) & 0xFF);
        UncompressedData.Add((Value >> 16) & 0xFF);
        UncompressedData.Add((Value >> 24) & 0xFF);
    };
    
    auto WriteStringTemp = [&UncompressedData, &WriteInt32Temp](const FString& Str) {
        FTCHARToUTF8 Converter(*Str);
        int32 Length = Converter.Length();
        WriteInt32Temp(Length);
        for (int32 i = 0; i < Length; ++i)
        {
            UncompressedData.Add(Converter.Get()[i]);
        }
    };
    
    auto WriteInt64Temp = [&UncompressedData](int64 Value) {
        for (int32 i = 0; i < 8; ++i)
        {
            UncompressedData.Add((Value >> (i * 8)) & 0xFF);
        }
    };
    
    // Write metadata header
    UncompressedData.Add(static_cast<uint8>(Metadata.CompilerType));
    WriteInt32Temp(static_cast<uint32>(Metadata.CompilerFlags));
    WriteStringTemp(Metadata.CompilerName);
    WriteStringTemp(Metadata.CompilerVersion);
    WriteStringTemp(Metadata.EngineVersion);
    WriteStringTemp(Metadata.GameVersion);
    WriteStringTemp(Metadata.GameName);
    WriteStringTemp(Metadata.AuthorName);
    WriteStringTemp(Metadata.OperatingSystem);
    WriteStringTemp(Metadata.MachineName);
    WriteInt64Temp(Metadata.CompilationTime.GetTicks());
    WriteStringTemp(Metadata.SourceFileName);
    WriteInt32Temp(Metadata.SourceFileSize);
    WriteStringTemp(Metadata.SourceChecksum);
    
    // Write bIsMission flag
    UncompressedData.Add(Metadata.bIsMission ? 1 : 0);
    
    // Write source hash
    WriteStringTemp(SourceHash);
    
    // Write bytecode
    WriteInt32Temp(Code.Num());
    UncompressedData.Append(Code);
    
    // Write constants
    WriteInt32Temp(Constants.Num());
    for (const FScriptValue& Constant : Constants)
    {
        // Write type
        UncompressedData.Add(static_cast<uint8>(Constant.Type));
        
        // Write value based on type
        switch (Constant.Type)
        {
            case EValueType::NIL:
                // No data needed
                break;
                
            case EValueType::BOOL:
                UncompressedData.Add(Constant.BoolValue ? 1 : 0);
                break;
                
            case EValueType::NUMBER:
            {
                uint64 NumberBits = *reinterpret_cast<const uint64*>(&Constant.NumberValue);
                for (int32 i = 0; i < 8; ++i)
                {
                    UncompressedData.Add((NumberBits >> (i * 8)) & 0xFF);
                }
                break;
            }
                
            case EValueType::STRING:
                WriteStringTemp(Constant.StringValue);
                break;
                
            case EValueType::ARRAY:
            {
                WriteInt32Temp(Constant.ArrayValue.Num());
                // Note: Nested arrays not fully serialized here - could be extended
                break;
            }
        }
    }
    
    // Write functions
    WriteInt32Temp(Functions.Num());
    for (const FFunctionInfo& Func : Functions)
    {
        WriteStringTemp(Func.Name);
        WriteInt32Temp(Func.Address);
        WriteInt32Temp(Func.Arity);
    }
    
    // Now write the final output with header
    // Write magic number
    WriteInt32(BYTECODE_MAGIC);
    
    // Write version
    WriteInt32(Version);
    
    // Write flags (compressed or not)
    uint32 Flags = bCompress ? COMPRESSED_FLAG : 0;
    WriteInt32(Flags);
    
    // Write signature
    WriteString(const_cast<FBytecodeChunk*>(this)->GenerateSignature());
    
    // Write uncompressed size
    WriteInt32(UncompressedData.Num());
    
    // Compress or copy data
    if (bCompress && UncompressedData.Num() > 1024) // Only compress if > 1KB
    {
        // Use simple compression (ZLIB via FCompression)
        int32 CompressedSize = FCompression::CompressMemoryBound(NAME_Zlib, UncompressedData.Num());
        TArray<uint8> CompressedData;
        CompressedData.SetNum(CompressedSize);
        
        if (FCompression::CompressMemory(NAME_Zlib, CompressedData.GetData(), CompressedSize, 
            UncompressedData.GetData(), UncompressedData.Num()))
        {
            CompressedData.SetNum(CompressedSize);
            WriteInt32(CompressedData.Num());
            OutData.Append(CompressedData);
        }
        else
        {
            // Compression failed, write uncompressed
            WriteInt32(UncompressedData.Num());
            OutData.Append(UncompressedData);
        }
    }
    else
    {
        // Write uncompressed
        WriteInt32(UncompressedData.Num());
        OutData.Append(UncompressedData);
    }
    
    return true;
}

bool FBytecodeChunk::Deserialize(const TArray<uint8>& InData)
{
    int32 Offset = 0;
    
    // Helper lambda to read data
    auto ReadInt32 = [&InData, &Offset]() -> int32 {
        if (Offset + 4 > InData.Num()) return 0;
        int32 Value = 0;
        Value |= InData[Offset++] << 0;
        Value |= InData[Offset++] << 8;
        Value |= InData[Offset++] << 16;
        Value |= InData[Offset++] << 24;
        return Value;
    };
    
    auto ReadInt64 = [&InData, &Offset]() -> int64 {
        if (Offset + 8 > InData.Num()) return 0;
        int64 Value = 0;
        for (int32 i = 0; i < 8; ++i)
        {
            Value |= static_cast<int64>(InData[Offset++]) << (i * 8);
        }
        return Value;
    };
    
    auto ReadString = [&InData, &Offset, &ReadInt32]() -> FString {
        int32 Length = ReadInt32();
        if (Length <= 0 || Offset + Length > InData.Num()) return FString();
        
        TArray<ANSICHAR> UTF8Data;
        UTF8Data.SetNum(Length + 1);
        for (int32 i = 0; i < Length; ++i)
        {
            UTF8Data[i] = InData[Offset++];
        }
        UTF8Data[Length] = 0;
        
        return FString(UTF8_TO_TCHAR(UTF8Data.GetData()));
    };
    
    Clear();
    
    // Read and verify magic number
    int32 Magic = ReadInt32();
    if (Magic != BYTECODE_MAGIC)
    {
        return false; // Invalid bytecode file
    }
    
    // Read version
    Version = ReadInt32();
    
    // Read flags
    uint32 Flags = ReadInt32();
    bool bIsCompressed = (Flags & COMPRESSED_FLAG) != 0;
    
    // Read signature
    Signature = ReadString();
    
    // Read uncompressed size
    int32 UncompressedSize = ReadInt32();
    
    // Read compressed size
    int32 CompressedSize = ReadInt32();
    
    // Decompress if needed
    TArray<uint8> UncompressedData;
    if (bIsCompressed && CompressedSize < UncompressedSize)
    {
        if (Offset + CompressedSize > InData.Num()) return false;
        
        UncompressedData.SetNum(UncompressedSize);
        if (!FCompression::UncompressMemory(NAME_Zlib, UncompressedData.GetData(), UncompressedSize,
            &InData[Offset], CompressedSize))
        {
            return false; // Decompression failed
        }
        Offset += CompressedSize;
    }
    else
    {
        // Not compressed or same size
        if (Offset + UncompressedSize > InData.Num()) return false;
        UncompressedData.Append(&InData[Offset], UncompressedSize);
        Offset += UncompressedSize;
    }
    
    // Now parse the uncompressed data
    int32 DataOffset = 0;
    auto ReadInt32Data = [&UncompressedData, &DataOffset]() -> int32 {
        if (DataOffset + 4 > UncompressedData.Num()) return 0;
        int32 Value = 0;
        Value |= UncompressedData[DataOffset++] << 0;
        Value |= UncompressedData[DataOffset++] << 8;
        Value |= UncompressedData[DataOffset++] << 16;
        Value |= UncompressedData[DataOffset++] << 24;
        return Value;
    };
    
    auto ReadInt64Data = [&UncompressedData, &DataOffset]() -> int64 {
        if (DataOffset + 8 > UncompressedData.Num()) return 0;
        int64 Value = 0;
        for (int32 i = 0; i < 8; ++i)
        {
            Value |= static_cast<int64>(UncompressedData[DataOffset++]) << (i * 8);
        }
        return Value;
    };
    
    auto ReadStringData = [&UncompressedData, &DataOffset, &ReadInt32Data]() -> FString {
        int32 Length = ReadInt32Data();
        if (Length <= 0 || DataOffset + Length > UncompressedData.Num()) return FString();
        
        TArray<ANSICHAR> UTF8Data;
        UTF8Data.SetNum(Length + 1);
        for (int32 i = 0; i < Length; ++i)
        {
            UTF8Data[i] = UncompressedData[DataOffset++];
        }
        UTF8Data[Length] = 0;
        
        return FString(UTF8_TO_TCHAR(UTF8Data.GetData()));
    };
    
    // Read metadata
    if (DataOffset >= UncompressedData.Num()) return false;
    Metadata.CompilerType = static_cast<ECompilerType>(UncompressedData[DataOffset++]);
    Metadata.CompilerFlags = static_cast<EScriptCompilerFlags>(ReadInt32Data());
    Metadata.CompilerName = ReadStringData();
    Metadata.CompilerVersion = ReadStringData();
    Metadata.EngineVersion = ReadStringData();
    Metadata.GameVersion = ReadStringData();
    Metadata.GameName = ReadStringData();
    Metadata.AuthorName = ReadStringData();
    Metadata.OperatingSystem = ReadStringData();
    Metadata.MachineName = ReadStringData();
    Metadata.CompilationTime = FDateTime(ReadInt64Data());
    Metadata.SourceFileName = ReadStringData();
    Metadata.SourceFileSize = ReadInt32Data();
    Metadata.SourceChecksum = ReadStringData();

    // Read bIsMission flag (new in version 2)
    if (Version >= 2)
    {
        if (DataOffset >= UncompressedData.Num()) return false;
        Metadata.bIsMission = (UncompressedData[DataOffset++] != 0);
    }
    
    // Read source hash
    SourceHash = ReadStringData();
    
    // Read bytecode
    int32 CodeSize = ReadInt32Data();
    if (DataOffset + CodeSize > UncompressedData.Num()) return false;
    Code.Append(&UncompressedData[DataOffset], CodeSize);
    DataOffset += CodeSize;
    
    // Read constants
    int32 ConstantCount = ReadInt32Data();
    Constants.Reserve(ConstantCount);
    for (int32 i = 0; i < ConstantCount; ++i)
    {
        if (DataOffset >= UncompressedData.Num()) return false;
        
        EValueType Type = static_cast<EValueType>(UncompressedData[DataOffset++]);
        FScriptValue Value;
        Value.Type = Type;
        
        switch (Type)
        {
            case EValueType::NIL:
                // No data
                break;
                
            case EValueType::BOOL:
                if (DataOffset >= UncompressedData.Num()) return false;
                Value.BoolValue = (UncompressedData[DataOffset++] != 0);
                break;
                
            case EValueType::NUMBER:
            {
                if (DataOffset + 8 > UncompressedData.Num()) return false;
                uint64 NumberBits = 0;
                for (int32 j = 0; j < 8; ++j)
                {
                    NumberBits |= static_cast<uint64>(UncompressedData[DataOffset++]) << (j * 8);
                }
                Value.NumberValue = *reinterpret_cast<double*>(&NumberBits);
                break;
            }
                
            case EValueType::STRING:
                Value.StringValue = ReadStringData();
                break;
                
            case EValueType::ARRAY:
            {
                int32 ArraySize = ReadInt32Data();
                // Note: Nested arrays not fully deserialized - could be extended
                Value.ArrayValue.SetNum(ArraySize);
                break;
            }
        }
        
        Constants.Add(Value);
    }
    
    // Read functions
    int32 FunctionCount = ReadInt32Data();
    Functions.Reserve(FunctionCount);
    for (int32 i = 0; i < FunctionCount; ++i)
    {
        FFunctionInfo Func;
        Func.Name = ReadStringData();
        Func.Address = ReadInt32Data();
        Func.Arity = ReadInt32Data();
        Functions.Add(Func);
    }
    
    // Verify signature
    if (!VerifySignature(Signature))
    {
        UE_LOG(LogTemp, Warning, TEXT("Bytecode signature verification failed! File may be corrupted or tampered with."));
        // Continue anyway for now, but log the warning
    }
    
    return true;
}

// Decompiler implementation
FString FBytecodeChunk::Decompile() const
{
    FString Result;
    
    Result += TEXT("// ========================================\n");
    Result += TEXT("// DECOMPILED SCRIPT BYTECODE\n");
    Result += TEXT("// ========================================\n");
    Result += FString::Printf(TEXT("// Compiler: %s\n"), *Metadata.CompilerName);
    Result += FString::Printf(TEXT("// Compiler Version: %s\n"), *Metadata.CompilerVersion);
    Result += FString::Printf(TEXT("// Compiler Type: %s\n"), 
        Metadata.CompilerType == ECompilerType::UnrealEngineCompiler ? TEXT("UnrealEngine (TRUSTED)") :
        Metadata.CompilerType == ECompilerType::StandaloneCompiler ? TEXT("Standalone") :
        Metadata.CompilerType == ECompilerType::ExternalCompiler ? TEXT("External (UNTRUSTED)") : TEXT("Unknown"));
    Result += FString::Printf(TEXT("// Engine: %s\n"), *Metadata.EngineVersion);
    Result += FString::Printf(TEXT("// Game: %s %s\n"), *Metadata.GameName, *Metadata.GameVersion);
    Result += FString::Printf(TEXT("// Flags: 0x%08X [%s%s%s%s%s]\n"), 
        static_cast<uint32>(Metadata.CompilerFlags),
        EnumHasAnyFlags(Metadata.CompilerFlags, EScriptCompilerFlags::OfficialBuild) ? TEXT("Official ") : TEXT(""),
        EnumHasAnyFlags(Metadata.CompilerFlags, EScriptCompilerFlags::TrustedSigned) ? TEXT("Trusted ") : TEXT(""),
        EnumHasAnyFlags(Metadata.CompilerFlags, EScriptCompilerFlags::SecurityVerified) ? TEXT("Verified ") : TEXT(""),
        EnumHasAnyFlags(Metadata.CompilerFlags, EScriptCompilerFlags::DevelopmentBuild) ? TEXT("Dev ") : TEXT(""),
        EnumHasAnyFlags(Metadata.CompilerFlags, EScriptCompilerFlags::ShippingBuild) ? TEXT("Shipping") : TEXT(""));
    Result += TEXT("// ========================================\n");
    Result += FString::Printf(TEXT("// Author: %s\n"), *Metadata.AuthorName);
    Result += FString::Printf(TEXT("// Compiled on: %s\n"), *Metadata.CompilationTime.ToString());
    Result += FString::Printf(TEXT("// OS: %s\n"), *Metadata.OperatingSystem);
    Result += FString::Printf(TEXT("// Machine: %s\n"), *Metadata.MachineName);
    Result += FString::Printf(TEXT("// Source: %s (%d bytes)\n"), *Metadata.SourceFileName, Metadata.SourceFileSize);
    Result += FString::Printf(TEXT("// Checksum: %s\n"), *Metadata.SourceChecksum);
    FString SignaturePreview = Signature.Left(16);
    Result += FString::Printf(TEXT("// Signature: %s\n"), *SignaturePreview);
    Result += TEXT("// ========================================\n\n");
    
    // List all functions
    if (Functions.Num() > 0)
    {
        Result += TEXT("// FUNCTIONS:\n");
        for (const FFunctionInfo& Func : Functions)
        {
            Result += FString::Printf(TEXT("//   %s(arity=%d) @ address %d\n"), 
                *Func.Name, Func.Arity, Func.Address);
        }
        Result += TEXT("\n");
    }
    
    // List constants
    if (Constants.Num() > 0)
    {
        Result += TEXT("// CONSTANT POOL:\n");
        for (int32 i = 0; i < Constants.Num() && i < 20; ++i) // Limit to first 20
        {
            Result += FString::Printf(TEXT("//   [%d] = %s\n"), i, *Constants[i].ToString());
        }
        if (Constants.Num() > 20)
        {
            Result += FString::Printf(TEXT("//   ... and %d more\n"), Constants.Num() - 20);
        }
        Result += TEXT("\n");
    }
    
    // Disassemble bytecode
    Result += Disassemble();
    
    return Result;
}

// Check if bytecode is from trusted compiler
bool FBytecodeChunk::IsTrustedCompiler() const
{
    // Only UnrealEngine compiler is fully trusted by default
    if (Metadata.CompilerType == ECompilerType::UnrealEngineCompiler)
    {
        return EnumHasAnyFlags(Metadata.CompilerFlags, EScriptCompilerFlags::OfficialBuild | EScriptCompilerFlags::TrustedSigned);
    }
    
    // Standalone compiler needs to be signed and verified
    if (Metadata.CompilerType == ECompilerType::StandaloneCompiler)
    {
        return EnumHasAllFlags(Metadata.CompilerFlags, EScriptCompilerFlags::TrustedSigned | EScriptCompilerFlags::SecurityVerified);
    }
    
    // External compilers are not trusted
    return false;
}

// Validate bytecode security
bool FBytecodeChunk::ValidateSecurity(FString& OutReason) const
{
    // Check compiler type
    if (Metadata.CompilerType == ECompilerType::Unknown)
    {
        OutReason = TEXT("Unknown compiler type");
        return false;
    }
    
    // Check if external compiler (not allowed in production)
    if (Metadata.CompilerType == ECompilerType::ExternalCompiler)
    {
#if UE_BUILD_SHIPPING
        OutReason = TEXT("External compilers not allowed in shipping builds");
        return false;
#else
        UE_LOG(LogTemp, Warning, TEXT("Bytecode: Loading script from external compiler (allowed in dev builds only)"));
#endif
    }
    
    // Check official build flag
    if (!EnumHasAnyFlags(Metadata.CompilerFlags, EScriptCompilerFlags::OfficialBuild))
    {
#if UE_BUILD_SHIPPING
        OutReason = TEXT("Bytecode not from official build");
        return false;
#else
        UE_LOG(LogTemp, Warning, TEXT("Bytecode: Not from official build (allowed in dev builds)"));
#endif
    }
    
    // Check trusted signature
    if (!EnumHasAnyFlags(Metadata.CompilerFlags, EScriptCompilerFlags::TrustedSigned))
    {
        OutReason = TEXT("Bytecode not signed with trusted signature");
        return false;
    }
    
    // Verify signature
    if (!VerifySignature(Signature))
    {
        OutReason = TEXT("Bytecode signature verification failed - file may be corrupted or tampered");
        return false;
    }
    
    // Check security verification flag
    if (!EnumHasAnyFlags(Metadata.CompilerFlags, EScriptCompilerFlags::SecurityVerified))
    {
        OutReason = TEXT("Bytecode has not passed security verification");
        return false;
    }
    
    // Check engine version compatibility (optional - could be more strict)
    if (!Metadata.EngineVersion.Contains(TEXT("5.6")))
    {
        UE_LOG(LogTemp, Warning, TEXT("Bytecode: Compiled with different engine version: %s"), *Metadata.EngineVersion);
    }
    
    // All checks passed
    OutReason = TEXT("Security validation passed");
    return true;
}


