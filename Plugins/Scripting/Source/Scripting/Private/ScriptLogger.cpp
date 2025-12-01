// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#include "ScriptLogger.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"

TUniquePtr<FArchive> FScriptLogger::ScriptLogFile = nullptr;
TUniquePtr<FArchive> FScriptLogger::VMLogFile = nullptr;
FCriticalSection FScriptLogger::LogMutex;
bool FScriptLogger::bInitialized = false;

void FScriptLogger::Initialize()
{
    FScopeLock Lock(&LogMutex);
    
    if (bInitialized)
    {
        return;
    }
    
    FString ScriptLogPath = GetScriptLogPath();
    FString VMLogPath = GetVMLogPath();
    
    // Create directory if it doesn't exist
    FString LogDir = FPaths::GetPath(ScriptLogPath);
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*LogDir))
    {
        PlatformFile.CreateDirectoryTree(*LogDir);
    }
    
    // Open Script.log (Parser/Compiler logs)
    ScriptLogFile = TUniquePtr<FArchive>(IFileManager::Get().CreateFileWriter(*ScriptLogPath, FILEWRITE_Append));
    
    // Open VM.log (VM execution logs)
    VMLogFile = TUniquePtr<FArchive>(IFileManager::Get().CreateFileWriter(*VMLogPath, FILEWRITE_Append));
    
    if (ScriptLogFile && VMLogFile)
    {
        bInitialized = true;
        
        // Write header to Script.log
        FString ScriptHeader = FString::Printf(TEXT("\n\n========================================\n"));
        ScriptHeader += FString::Printf(TEXT("Script Log Session Started: %s\n"), *FDateTime::Now().ToString());
        ScriptHeader += FString::Printf(TEXT("========================================\n\n"));
        WriteToFile(ScriptHeader, ELogTarget::Script);
        
        // Write header to VM.log
        FString VMHeader = FString::Printf(TEXT("\n\n========================================\n"));
        VMHeader += FString::Printf(TEXT("VM Log Session Started: %s\n"), *FDateTime::Now().ToString());
        VMHeader += FString::Printf(TEXT("========================================\n\n"));
        WriteToFile(VMHeader, ELogTarget::VM);
        
        UE_LOG(LogTemp, Log, TEXT("ScriptLogger initialized:"));
        UE_LOG(LogTemp, Log, TEXT("  Script.log: %s"), *ScriptLogPath);
        UE_LOG(LogTemp, Log, TEXT("  VM.log: %s"), *VMLogPath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create log files!"));
        if (!ScriptLogFile) UE_LOG(LogTemp, Error, TEXT("  Script.log failed: %s"), *ScriptLogPath);
        if (!VMLogFile) UE_LOG(LogTemp, Error, TEXT("  VM.log failed: %s"), *VMLogPath);
    }
}

void FScriptLogger::Shutdown()
{
    FScopeLock Lock(&LogMutex);
    
    if (!bInitialized)
    {
        return;
    }
    
    FString Footer = FString::Printf(TEXT("\n========================================\n"));
    Footer += FString::Printf(TEXT("Log Session Ended: %s\n"), *FDateTime::Now().ToString());
    Footer += FString::Printf(TEXT("========================================\n"));
    
    if (ScriptLogFile)
    {
        WriteToFile(Footer, ELogTarget::Script);
        ScriptLogFile->Close();
        ScriptLogFile.Reset();
    }
    
    if (VMLogFile)
    {
        WriteToFile(Footer, ELogTarget::VM);
        VMLogFile->Close();
        VMLogFile.Reset();
    }
    
    bInitialized = false;
}

void FScriptLogger::LogInfo(const FString& Message, ELogTarget Target)
{
    Log(ELogLevel::Info, Message, Target);
}

void FScriptLogger::LogWarning(const FString& Message, ELogTarget Target)
{
    Log(ELogLevel::Warning, Message, Target);
}

void FScriptLogger::LogError(const FString& Message, ELogTarget Target)
{
    Log(ELogLevel::Error, Message, Target);
}

void FScriptLogger::LogAssert(const FString& Message, const FString& File, int32 Line, ELogTarget Target)
{
    FString FullMessage = Message;
    if (!File.IsEmpty() && Line > 0)
    {
        FullMessage = FString::Printf(TEXT("%s [%s:%d]"), *Message, *File, Line);
    }
    Log(ELogLevel::Assert, FullMessage, Target);
}

void FScriptLogger::LogDebug(const FString& Message, ELogTarget Target)
{
#if !UE_BUILD_SHIPPING
    Log(ELogLevel::Debug, Message, Target);
#endif
}

void FScriptLogger::Log(ELogLevel Level, const FString& Message, ELogTarget Target)
{
    if (!bInitialized)
    {
        Initialize();
    }
    
    FScopeLock Lock(&LogMutex);
    
    FString FormattedMessage = FString::Printf(TEXT("[%s] [%s] %s\n"),
        *GetTimestamp(),
        *GetLevelString(Level),
        *Message
    );
    
    WriteToFile(FormattedMessage, Target);
    
    // Also output to console for convenience (can be disabled in shipping builds)
#if !UE_BUILD_SHIPPING
    FString Prefix = (Target == ELogTarget::VM) ? TEXT("VM") : TEXT("SCRIPT");
    
    switch (Level)
    {
        case ELogLevel::Info:
        case ELogLevel::Debug:
            UE_LOG(LogTemp, Log, TEXT("%s: %s"), *Prefix, *Message);
            break;
        case ELogLevel::Warning:
            UE_LOG(LogTemp, Warning, TEXT("%s: %s"), *Prefix, *Message);
            break;
        case ELogLevel::Error:
        case ELogLevel::Assert:
            UE_LOG(LogTemp, Error, TEXT("%s: %s"), *Prefix, *Message);
            break;
    }
#endif
}

void FScriptLogger::Flush()
{
    FScopeLock Lock(&LogMutex);
    
    if (ScriptLogFile)
    {
        ScriptLogFile->Flush();
    }
    
    if (VMLogFile)
    {
        VMLogFile->Flush();
    }
}

FString FScriptLogger::GetScriptLogPath()
{
    return FPaths::ProjectSavedDir() / TEXT("Logs") / TEXT("Script.log");
}

FString FScriptLogger::GetVMLogPath()
{
    return FPaths::ProjectSavedDir() / TEXT("Logs") / TEXT("VM.log");
}

void FScriptLogger::WriteToFile(const FString& FormattedMessage, ELogTarget Target)
{
    auto WriteToArchive = [&FormattedMessage](FArchive* Archive)
    {
        if (Archive)
        {
            FTCHARToUTF8 UTF8String(*FormattedMessage);
            Archive->Serialize((void*)UTF8String.Get(), UTF8String.Length());
            Archive->Flush(); // Flush immediately so we don't lose logs on crashes
        }
    };
    
    switch (Target)
    {
        case ELogTarget::Script:
            WriteToArchive(ScriptLogFile.Get());
            break;
        case ELogTarget::VM:
            WriteToArchive(VMLogFile.Get());
            break;
        case ELogTarget::Both:
            WriteToArchive(ScriptLogFile.Get());
            WriteToArchive(VMLogFile.Get());
            break;
    }
}

FString FScriptLogger::GetLevelString(ELogLevel Level)
{
    switch (Level)
    {
        case ELogLevel::Info:    return TEXT("INFO   ");
        case ELogLevel::Warning: return TEXT("WARNING");
        case ELogLevel::Error:   return TEXT("ERROR  ");
        case ELogLevel::Assert:  return TEXT("ASSERT ");
        case ELogLevel::Debug:   return TEXT("DEBUG  ");
        default:                 return TEXT("UNKNOWN");
    }
}

FString FScriptLogger::GetTimestamp()
{
    FDateTime Now = FDateTime::Now();
    return FString::Printf(TEXT("%02d:%02d:%02d.%03d"),
        Now.GetHour(),
        Now.GetMinute(),
        Now.GetSecond(),
        Now.GetMillisecond()
    );
}

