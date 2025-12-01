// Copyright Vampire Game Project. All Rights Reserved.
// Custom scripting system for secure modding support.

#pragma once

#include "CoreMinimal.h"

/**
 * Dedicated logging system for scripting system
 * Similar to GTA's script asserts - keeps script logs separate from engine logs
 * Outputs to separate log files:
 *  - Saved/Logs/VM.log - Virtual machine execution logs
 *  - Saved/Logs/Script.log - Parser/Compiler logs
 *  - Saved/Logs/Sandbox.log - General engine logs (Unreal default)
 */
class SCRIPTING_API FScriptLogger
{
public:
    enum class ELogLevel
    {
        Info,
        Warning,
        Error,
        Assert,
        Debug
    };

    enum class ELogTarget
    {
        Script,     // Parser/Compiler logs -> Script.log
        VM,         // VM execution logs -> VM.log
        Both        // Log to both files
    };
    
    /** Initialize logger and open log files */
    static void Initialize();
    
    /** Shutdown logger and close files */
    static void Shutdown();
    
    /** Log an info message to specified target */
    static void LogInfo(const FString& Message, ELogTarget Target = ELogTarget::Script);
    
    /** Log a warning message to specified target */
    static void LogWarning(const FString& Message, ELogTarget Target = ELogTarget::Script);
    
    /** Log an error message to specified target */
    static void LogError(const FString& Message, ELogTarget Target = ELogTarget::Script);
    
    /** Log an assert (critical error that should stop execution) */
    static void LogAssert(const FString& Message, const FString& File = TEXT(""), int32 Line = 0, ELogTarget Target = ELogTarget::Both);
    
    /** Log a debug message (only in debug builds) */
    static void LogDebug(const FString& Message, ELogTarget Target = ELogTarget::Script);
    
    /** Log with custom level and target */
    static void Log(ELogLevel Level, const FString& Message, ELogTarget Target = ELogTarget::Script);
    
    /** Flush the log files to disk */
    static void Flush();
    
    /** Get the log file paths */
    static FString GetScriptLogPath();
    static FString GetVMLogPath();

private:
    static void WriteToFile(const FString& FormattedMessage, ELogTarget Target);
    static FString GetLevelString(ELogLevel Level);
    static FString GetTimestamp();
    
    static TUniquePtr<FArchive> ScriptLogFile;
    static TUniquePtr<FArchive> VMLogFile;
    static FCriticalSection LogMutex;
    static bool bInitialized;
};

// Convenience macros for cleaner usage
// Script logs (Parser/Compiler)
#define SCRIPT_LOG(Message) FScriptLogger::LogInfo(Message, FScriptLogger::ELogTarget::Script)
#define SCRIPT_LOG_WARNING(Message) FScriptLogger::LogWarning(Message, FScriptLogger::ELogTarget::Script)
#define SCRIPT_LOG_ERROR(Message) FScriptLogger::LogError(Message, FScriptLogger::ELogTarget::Script)
#define SCRIPT_LOG_ASSERT(Message) FScriptLogger::LogAssert(Message, TEXT(__FILE__), __LINE__, FScriptLogger::ELogTarget::Script)

// VM logs (Virtual Machine execution)
#define VM_LOG(Message) FScriptLogger::LogInfo(Message, FScriptLogger::ELogTarget::VM)
#define VM_LOG_WARNING(Message) FScriptLogger::LogWarning(Message, FScriptLogger::ELogTarget::VM)
#define VM_LOG_ERROR(Message) FScriptLogger::LogError(Message, FScriptLogger::ELogTarget::VM)
#define VM_LOG_ASSERT(Message) FScriptLogger::LogAssert(Message, TEXT(__FILE__), __LINE__, FScriptLogger::ELogTarget::VM)

#if !UE_BUILD_SHIPPING
    #define SCRIPT_LOG_DEBUG(Message) FScriptLogger::LogDebug(Message, FScriptLogger::ELogTarget::Script)
    #define VM_LOG_DEBUG(Message) FScriptLogger::LogDebug(Message, FScriptLogger::ELogTarget::VM)
#else
    #define SCRIPT_LOG_DEBUG(Message)
    #define VM_LOG_DEBUG(Message)
#endif

