// Standalone Script Compiler
// Compiles .sc scripts to .scc bytecode files compatible with Sandbox game

#include "Platform.h"
#include "ScriptToken.h"
#include "ScriptLexer.h"
#include "ScriptAST.h"
#include "ScriptParser.h"
#include "ScriptCompiler.h"
#include "ScriptBytecode.h"

#include <iostream>
#include <chrono>

void PrintUsage()
{
    std::cout << "SBS Script Compiler v1.0\n";
    std::cout << "Compiles .sc scripts to .scc bytecode for Sandbox Game\n\n";
    std::cout << "Usage:\n";
    std::cout << "  ScriptCompiler <input.sc> [output.scc]\n";
    std::cout << "  ScriptCompiler <input.sc> -o <output.scc>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o <file>     Output file (default: same name as input with .scc extension)\n";
    std::cout << "  -d            Save decompiled .txt file for verification\n";
    std::cout << "  -v            Verbose output\n";
    std::cout << "  --help        Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  ScriptCompiler MyScript.sc\n";
    std::cout << "  ScriptCompiler MyScript.sc -o Compiled/MyScript.scc\n";
    std::cout << "  ScriptCompiler MyScript.sc -d -v\n";
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        PrintUsage();
        return 1;
    }
    
    // Parse arguments
    FString InputFile;
    FString OutputFile;
    bool bSaveDecompiled = false;
    bool bVerbose = false;
    
    for (int i = 1; i < argc; ++i)
    {
        FString arg = argv[i];
        
        if (arg == "--help" || arg == "-h")
        {
            PrintUsage();
            return 0;
        }
        else if (arg == "-o")
        {
            if (i + 1 < argc)
            {
                OutputFile = argv[++i];
            }
            else
            {
                LOG_ERROR("Missing output file after -o");
                return 1;
            }
        }
        else if (arg == "-d")
        {
            bSaveDecompiled = true;
        }
        else if (arg == "-v")
        {
            bVerbose = true;
        }
        else if (InputFile.empty())
        {
            InputFile = arg;
        }
        else
        {
            LOG_ERROR("Unknown argument: " + arg);
            return 1;
        }
    }
    
    if (InputFile.empty())
    {
        LOG_ERROR("No input file specified");
        PrintUsage();
        return 1;
    }
    
    // Set default output file
    if (OutputFile.empty())
    {
        OutputFile = FPaths::GetBaseFilename(InputFile) + ".scc";
    }
    
    LOG_INFO("================================================================================");
    LOG_INFO("SBS Script Compiler v1.0");
    LOG_INFO("================================================================================");
    LOG_INFO("Input:  " + InputFile);
    LOG_INFO("Output: " + OutputFile);
    LOG_INFO("");
    
    // Check if input file exists
    if (!FPaths::FileExists(InputFile))
    {
        LOG_ERROR("Input file not found: " + InputFile);
        return 1;
    }
    
    // Load source code
    FString SourceCode;
    if (!FFileHelper::LoadFileToString(SourceCode, InputFile))
    {
        LOG_ERROR("Failed to read input file: " + InputFile);
        return 1;
    }
    
    if (bVerbose)
    {
        LOG_INFO("Source code loaded: " + std::to_string(SourceCode.length()) + " bytes");
    }
    
    // Start compilation timer
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Lexical analysis
    LOG_INFO("[1/4] Lexical analysis...");
    FScriptLexer Lexer(SourceCode);
    TArray<FScriptToken> Tokens = Lexer.ScanTokens();
    
    if (Lexer.HasErrors())
    {
        LOG_ERROR("Lexer errors:");
        for (const auto& error : Lexer.GetErrors())
        {
            LOG_ERROR("  " + error);
        }
        return 1;
    }
    
    if (bVerbose)
    {
        LOG_INFO("  Tokens: " + std::to_string(Tokens.size()));
    }
    
    // Parsing
    LOG_INFO("[2/4] Parsing...");
    FScriptParser Parser(Tokens);
    TSharedPtr<FScriptProgram> Program = Parser.Parse();
    
    if (!Program.IsValid() || Parser.HasErrors())
    {
        LOG_ERROR("Parser errors:");
        for (const auto& error : Parser.GetErrors())
        {
            LOG_ERROR("  " + error);
        }
        return 1;
    }
    
    if (bVerbose)
    {
        LOG_INFO("  Functions: " + std::to_string(Program->Functions.size()));
    }
    
    // Compilation
    LOG_INFO("[3/4] Compiling to bytecode...");
    FScriptCompiler Compiler;
    TSharedPtr<FBytecodeChunk> Bytecode = Compiler.Compile(Program);
    
    if (!Bytecode.IsValid() || Compiler.HasErrors())
    {
        LOG_ERROR("Compiler errors:");
        for (const auto& error : Compiler.GetErrors())
        {
            LOG_ERROR("  " + error);
        }
        return 1;
    }
    
    if (bVerbose)
    {
        LOG_INFO("  Bytecode size: " + std::to_string(Bytecode->Code.size()) + " bytes");
        LOG_INFO("  Constants: " + std::to_string(Bytecode->Constants.size()));
        LOG_INFO("  Functions: " + std::to_string(Bytecode->Functions.size()));
    }
    
    // Populate metadata for standalone compiler
    Bytecode->Metadata.CompilerType = ECompilerType::StandaloneCompiler;
    Bytecode->Metadata.CompilerFlags = EScriptCompilerFlags::TrustedSigned | EScriptCompilerFlags::SecurityVerified;
    Bytecode->Metadata.CompilerName = "StandaloneCompiler";
    Bytecode->Metadata.CompilerVersion = "SBS Compiler C 2025 V1.0";
    Bytecode->Metadata.EngineVersion = FStringPrintf("UE %d.%d", ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION);
    Bytecode->Metadata.GameName = "Sandbox Game";
    Bytecode->Metadata.GameVersion = "V1.0";
    Bytecode->Metadata.AuthorName = FPlatformMisc::GetLoginName();
    
    #if PLATFORM_WINDOWS
        Bytecode->Metadata.OperatingSystem = "Windows";
    #elif PLATFORM_MAC
        Bytecode->Metadata.OperatingSystem = "Mac";
    #elif PLATFORM_LINUX
        Bytecode->Metadata.OperatingSystem = "Linux";
    #else
        Bytecode->Metadata.OperatingSystem = "Unknown";
    #endif
    
    Bytecode->Metadata.MachineName = FPlatformMisc::GetMachineName();
    Bytecode->Metadata.CompilationTime = FDateTime::Now();
    Bytecode->Metadata.SourceFileName = FPaths::GetCleanFilename(InputFile);
    Bytecode->Metadata.SourceFileSize = SourceCode.length();
    Bytecode->Metadata.SourceChecksum = FMD5::HashAnsiString(SourceCode);
    
    // Generate signature
    Bytecode->Signature = Bytecode->GenerateSignature();
    
    // Serialization
    LOG_INFO("[4/4] Serializing with compression...");
    TArray<uint8> BytecodeData;
    if (!Bytecode->Serialize(BytecodeData, true))
    {
        LOG_ERROR("Failed to serialize bytecode");
        return 1;
    }
    
    // Save to file
    if (!FFileHelper::SaveArrayToFile(BytecodeData, OutputFile))
    {
        LOG_ERROR("Failed to write output file: " + OutputFile);
        return 1;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Calculate compression ratio
    float CompressionRatio = (float)BytecodeData.size() / (float)SourceCode.length();
    float SizeReduction = (1.0f - CompressionRatio) * 100.0f;
    
    LOG_INFO("");
    LOG_INFO("================================================================================");
    LOG_INFO("Compilation successful!");
    LOG_INFO("================================================================================");
    LOG_INFO("Output file:       " + OutputFile);
    LOG_INFO("Source size:       " + std::to_string(SourceCode.length()) + " bytes");
    std::cout << "[INFO] Compiled size:     " << BytecodeData.size() << " bytes" << std::endl;
    std::cout << "[INFO] Compression:       " << (int)SizeReduction << "% smaller" << std::endl;
    std::cout << "[INFO] Compilation time:  " << duration.count() << " ms" << std::endl;
    std::cout << "[INFO] Compiler:          " << Bytecode->Metadata.CompilerName << " " << Bytecode->Metadata.CompilerVersion << std::endl;
    std::cout << "[INFO] Author:            " << Bytecode->Metadata.AuthorName << std::endl;
    std::cout << "[INFO] Machine:           " << Bytecode->Metadata.MachineName << std::endl;
    std::cout << "[INFO] Signature:         " << Bytecode->Signature << std::endl;
    LOG_INFO("");
    
    // Save decompiled version if requested
    if (bSaveDecompiled)
    {
        FString DecompiledFile = FPaths::GetBaseFilename(OutputFile) + ".decompiled.txt";
        FString DecompiledCode = Bytecode->Decompile();
        
        if (FFileHelper::SaveStringToFile(DecompiledCode, DecompiledFile))
        {
            LOG_INFO("Decompiled listing: " + DecompiledFile);
        }
        else
        {
            LOG_WARNING("Failed to save decompiled file");
        }
    }
    
    LOG_INFO("Ready for distribution to Sandbox Game!");
    LOG_INFO("Place .scc file in Scripts/Compiled/ folder");
    
    return 0;
}

