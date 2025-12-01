// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#include "Scripting.h"
#include "ScriptLexer.h"
#include "ScriptParser.h"
#include "ScriptCompiler.h"
#include "ScriptVM.h"
#include "ScriptLogger.h"
#include "ScriptToken.h"
#include "ScriptBytecode.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"

#define LOCTEXT_NAMESPACE "FScriptingModule"

void FScriptingModule::StartupModule()
{
    FScriptLogger::Initialize();
    SCRIPT_LOG(TEXT("=== Scripting Module Started ==="));
    
    // Delay startup script loading until post-engine-init
    FCoreDelegates::OnPostEngineInit.AddRaw(this, &FScriptingModule::OnPostEngineInit);
}

void FScriptingModule::ShutdownModule()
{
    SCRIPT_LOG(TEXT("Scripting Module Shutdown"));
    FScriptLogger::Shutdown();
}

void FScriptingModule::OnPostEngineInit()
{
    SCRIPT_LOG(TEXT("=== Post Engine Init: Compiling All Scripts ==="));
    
    // Step 1: Compile all scripts in Scripts/ root (including headers)
    CompileRootScripts();
    
    // Step 2: Load and EXECUTE scripts in Scripts/Startup/
    LoadAndExecuteStartupScripts();
}

void FScriptingModule::CompileRootScripts()
{
    SCRIPT_LOG(TEXT(""));
    SCRIPT_LOG(TEXT("=== COMPILING SCRIPTS IN Scripts/ ROOT ==="));
    
    FString ScriptsRootPath = FPaths::ProjectDir() / TEXT("Scripts");
    FString CompiledPath = FPaths::ProjectDir() / TEXT("Scripts/Compiled");
    
    // Ensure compiled directory exists
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*CompiledPath))
    {
        PlatformFile.CreateDirectoryTree(*CompiledPath);
    }
    
    // Find all .sc and .sh files in root (not subfolders)
    TArray<FString> SourceFiles;
    IFileManager::Get().FindFiles(SourceFiles, *(ScriptsRootPath / TEXT("*.sc")), true, false);
    
    TArray<FString> HeaderFiles;
    IFileManager::Get().FindFiles(HeaderFiles, *(ScriptsRootPath / TEXT("*.sh")), true, false);
    
    SCRIPT_LOG(FString::Printf(TEXT("Found %d script files and %d header files in root"), 
        SourceFiles.Num(), HeaderFiles.Num()));
    
    // Compile all root scripts
    for (const FString& File : SourceFiles)
    {
        FString ScriptName = FPaths::GetBaseFilename(File);
        FString SourcePath = ScriptsRootPath / File;
        
        SCRIPT_LOG(FString::Printf(TEXT("Compiling: %s"), *ScriptName));
        
        FString SourceCode;
        if (FFileHelper::LoadFileToString(SourceCode, *SourcePath))
        {
            TSharedPtr<FBytecodeChunk> Bytecode = CompileScript(SourceCode, ScriptName);
            if (Bytecode.IsValid())
            {
                SCRIPT_LOG(FString::Printf(TEXT("  ? Compiled: %s"), *ScriptName));
            }
            else
            {
                SCRIPT_LOG_ERROR(FString::Printf(TEXT("  ? Failed: %s"), *ScriptName));
            }
        }
    }
    
    // Compile all header files
    for (const FString& File : HeaderFiles)
    {
        FString HeaderName = FPaths::GetBaseFilename(File);
        FString SourcePath = ScriptsRootPath / File;
        
        SCRIPT_LOG(FString::Printf(TEXT("Compiling header: %s"), *HeaderName));
        
        FString SourceCode;
        if (FFileHelper::LoadFileToString(SourceCode, *SourcePath))
        {
            TSharedPtr<FBytecodeChunk> Bytecode = CompileScript(SourceCode, HeaderName);
            if (Bytecode.IsValid())
            {
                SCRIPT_LOG(FString::Printf(TEXT("  ? Compiled header: %s"), *HeaderName));
            }
            else
            {
                SCRIPT_LOG_ERROR(FString::Printf(TEXT("  ? Failed header: %s"), *HeaderName));
            }
        }
    }
    
    SCRIPT_LOG(TEXT("=== ROOT SCRIPTS COMPILATION COMPLETE ==="));
}

void FScriptingModule::LoadAndExecuteStartupScripts()
{
    SCRIPT_LOG(TEXT(""));
    SCRIPT_LOG(TEXT("=== LOADING AND EXECUTING STARTUP SCRIPTS ==="));
    
    FString StartupScriptsPath = FPaths::ProjectDir() / TEXT("Scripts/Startup");
    FString CompiledPath = FPaths::ProjectDir() / TEXT("Scripts/Compiled");
    
    // Find all .sc files in Startup folder
    TSet<FString> StartupScriptNames;
    TArray<FString> StartupFiles;
    IFileManager::Get().FindFiles(StartupFiles, *(StartupScriptsPath / TEXT("*.sc")), true, false);
    for (const FString& File : StartupFiles)
    {
        StartupScriptNames.Add(FPaths::GetBaseFilename(File));
    }
    
    SCRIPT_LOG(FString::Printf(TEXT("Found %d startup scripts to execute"), StartupScriptNames.Num()));
    
    // Load and execute each startup script
    for (const FString& ScriptName : StartupScriptNames)
    {
        SCRIPT_LOG(TEXT(""));
        SCRIPT_LOG(TEXT("========================================"));
        SCRIPT_LOG(FString::Printf(TEXT("EXECUTING STARTUP SCRIPT: %s"), *ScriptName));
        SCRIPT_LOG(TEXT("========================================"));
        
        // Try compiled bytecode first
        FString CompiledFilePath = CompiledPath / ScriptName + TEXT(".scc");
        TSharedPtr<FBytecodeChunk> Bytecode;
        
        if (FPaths::FileExists(CompiledFilePath))
        {
            SCRIPT_LOG(FString::Printf(TEXT("Loading compiled bytecode: %s"), *CompiledFilePath));
            Bytecode = MakeShared<FBytecodeChunk>();
            TArray<uint8> BytecodeData;
            if (FFileHelper::LoadFileToArray(BytecodeData, *CompiledFilePath))
            {
                if (Bytecode->Deserialize(BytecodeData))
                {
                    SCRIPT_LOG(FString::Printf(TEXT("Loaded bytecode (%d bytes)"), BytecodeData.Num()));
                }
                else
                {
                    SCRIPT_LOG_ERROR(TEXT("Failed to deserialize bytecode"));
                    Bytecode.Reset();
                }
            }
        }
        
        // If no compiled bytecode, compile from source
        if (!Bytecode.IsValid())
        {
            FString SourceFilePath = StartupScriptsPath / ScriptName + TEXT(".sc");
            if (FPaths::FileExists(SourceFilePath))
            {
                SCRIPT_LOG(TEXT("No bytecode found, compiling from source..."));
                FString SourceCode;
                if (FFileHelper::LoadFileToString(SourceCode, *SourceFilePath))
                {
                    Bytecode = CompileScript(SourceCode, ScriptName);
                }
            }
        }
        
        // Execute the bytecode
        if (Bytecode.IsValid())
        {
            ExecuteStartupScript(Bytecode, ScriptName);
        }
        else
        {
            SCRIPT_LOG_ERROR(FString::Printf(TEXT("Failed to load/compile startup script: %s"), *ScriptName));
        }
    }
    
    SCRIPT_LOG(TEXT("=== ALL STARTUP SCRIPTS EXECUTED ==="));
}

TSharedPtr<FBytecodeChunk> FScriptingModule::CompileScript(const FString& SourceCode, const FString& ScriptName)
{
    // Compile the source
    FScriptLexer Lexer(SourceCode);
    TArray<FScriptToken> Tokens = Lexer.ScanTokens();
    
    if (Tokens.Num() == 0 || Lexer.HasErrors())
    {
        SCRIPT_LOG_ERROR(TEXT("Lexer failed or produced no tokens"));
        for (const FString& Error : Lexer.GetErrors())
        {
            SCRIPT_LOG_ERROR(FString::Printf(TEXT("  Lexer Error: %s"), *Error));
        }
        return nullptr;
    }
    
    FScriptParser Parser(Tokens);
    TSharedPtr<FScriptProgram> Program = Parser.Parse();
    
    if (!Program.IsValid() || Parser.HasErrors())
    {
        SCRIPT_LOG_ERROR(TEXT("Parser failed to produce program"));
        for (const FString& Error : Parser.GetErrors())
        {
            SCRIPT_LOG_ERROR(FString::Printf(TEXT("  Parser Error: %s"), *Error));
        }
        return nullptr;
    }
    
    FScriptCompiler Compiler;
    TSharedPtr<FBytecodeChunk> Bytecode = Compiler.Compile(Program);
    
    if (!Bytecode.IsValid())
    {
        SCRIPT_LOG_ERROR(TEXT("Compiler failed"));
        return nullptr;
    }
    
    // Save compiled bytecode
    FString CompiledDir = FPaths::ProjectDir() / TEXT("Scripts/Compiled");
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*CompiledDir))
    {
        PlatformFile.CreateDirectoryTree(*CompiledDir);
    }
    
    FString CompiledPath = CompiledDir / ScriptName + TEXT(".scc");
    TArray<uint8> BytecodeData;
    if (Bytecode->Serialize(BytecodeData, true))
    {
        if (FFileHelper::SaveArrayToFile(BytecodeData, *CompiledPath))
        {
            SCRIPT_LOG(FString::Printf(TEXT("Saved compiled bytecode: %s (%d bytes)"), *CompiledPath, BytecodeData.Num()));
        }
    }
    
    return Bytecode;
}

void FScriptingModule::ExecuteStartupScript(TSharedPtr<FBytecodeChunk> Bytecode, const FString& ScriptName)
{
    TSharedPtr<FScriptVM> VM = MakeShared<FScriptVM>();
    
    // Broadcast delegate to allow game module to register native functions
    GetStartupNativeAPIDelegate().Broadcast(VM.Get());
    
    // Execute the bytecode
    SCRIPT_LOG(TEXT("Executing startup script..."));
    bool bExecuteSuccess = VM->Execute(Bytecode);
    
    if (!bExecuteSuccess)
    {
        SCRIPT_LOG_ERROR(FString::Printf(TEXT("VM execution failed for: %s"), *ScriptName));
        return;
    }
    
    SCRIPT_LOG(FString::Printf(TEXT("COMPLETED STARTUP SCRIPT: %s"), *ScriptName));
}



#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FScriptingModule, Scripting)

