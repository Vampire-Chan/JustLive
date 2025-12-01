// Copyright Vampire Game Project. All Rights Reserved.
// Script loading, compilation, and execution management subsystem

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ScriptBytecode.h"
#include "ScriptVM.h"

// Delegate for registering native functions
DECLARE_MULTICAST_DELEGATE_OneParam(FNativeAPIRegistrationDelegate, class FScriptVM*);

#include "ScriptManager.generated.h"

/**
 * Compiled script entry with cached bytecode
 */
USTRUCT()
struct FCompiledScript
{
	GENERATED_BODY()
	
	/** Original source file path */
	UPROPERTY()
	FString SourcePath;
	
	/** Compiled bytecode (shared pointer for efficient copying) */
	TSharedPtr<FBytecodeChunk> Bytecode;
	
	/** VM instance for this script (isolated execution) */
	TSharedPtr<FScriptVM> VM;
	
	/** Last modification time of source file */
	FDateTime LastModified;
	
	/** Whether the script has been executed at least once */
	bool bExecuted;
	
	FCompiledScript()
		: LastModified(FDateTime::MinValue())
		, bExecuted(false)
	{}
};

/**
 * Script Manager Subsystem
 * Handles loading, compiling, caching, and executing game scripts
 * This is a GameInstanceSubsystem - runs when PIE starts or game launches
 */
UCLASS()
class SCRIPTING_API UScriptManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    /** Delegate called to allow external modules to register native functions. */
    FNativeAPIRegistrationDelegate OnRegisterNativeAPI;

	//=============================================================================
	// Subsystem Lifecycle
	//=============================================================================
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//=============================================================================
	// Script Loading & Compilation
	//=============================================================================
	
	/**
	 * Load and compile a script from file
	 * @param ScriptPath - Path to .sc file (relative to Scripts folder or absolute)
	 * @param bForceRecompile - Force recompilation even if cached bytecode exists
	 * @return Script name for later execution, or empty string on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	FString LoadScript(const FString& ScriptPath, bool bForceRecompile = false);

	/**
	 * Load and compile a script directly from a string
	 * @param ScriptName - Unique name for the script
	 * @param SourceCode - The script source code
	 * @return True if successful
	 */
	bool LoadScriptFromString(const FString& ScriptName, const FString& SourceCode);

	/**
	 * Load compiled bytecode directly from .scc file (mod distribution case)
	 * This skips compilation entirely and loads pre-compiled bytecode.
	 * @param BytecodePath - Path to .scc file (relative to Scripts/Compiled or absolute)
	 * @return Script name for later execution, or empty string on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	FString LoadCompiledBytecode(const FString& BytecodePath);
	
	/**
	 * Check if a script is loaded
	 */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	bool IsScriptLoaded(const FString& ScriptName) const;

	/**
	 * Check if a script is currently running (has been executed)
	 */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	bool IsScriptRunning(const FString& ScriptName) const;
	
	/**
	 * Get list of all loaded scripts
	 */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	TArray<FString> GetLoadedScripts() const;

	//=============================================================================
	// Script Execution
	//=============================================================================
	
	/**
	 * Execute a loaded script
	 * @param ScriptName - Name of script to execute
	 * @param bCallMain - If true, calls Main() function if it exists; otherwise just runs top-level code
	 * @return True if execution succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	bool ExecuteScript(const FString& ScriptName, bool bCallMain = true);
	
	/**
	 * Execute a script function by name
	 * @param ScriptName - Name of script containing the function
	 * @param FunctionName - Name of function to call
	 * @param Args - Function arguments (optional)
	 * @return Return value from function
	 */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	FString CallScriptFunction(const FString& ScriptName, const FString& FunctionName, const TArray<FString>& Args);
	
	/**
	 * Stop execution of a running script
	 */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	void StopScript(const FString& ScriptName);

	//=============================================================================
	// Script Management
	//=============================================================================
	
	/**
	 * Unload a script and free its resources
	 */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	void UnloadScript(const FString& ScriptName);
	
	/**
	 * Unload all scripts
	 */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	void UnloadAllScripts();
	
	/**
	 * Reload a script (useful for hot-reload during development)
	 */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	bool ReloadScript(const FString& ScriptName);
	
	/**
	 * Get compilation/execution errors for a script
	 */
	UFUNCTION(BlueprintCallable, Category = "Scripting")
	TArray<FString> GetScriptErrors(const FString& ScriptName) const;

	//=============================================================================
	// Bytecode Caching
	//=============================================================================
	
	/**
	 * Save compiled bytecode to cache file (.scc)
	 */
	bool SaveBytecodeCache(const FString& ScriptName, TSharedPtr<FBytecodeChunk> Bytecode);
	
	/**
	 * Load compiled bytecode from cache file (.scc)
	 */
	TSharedPtr<FBytecodeChunk> LoadBytecodeCache(const FString& ScriptPath);
	
	/**
	 * Check if bytecode cache is up to date
	 */
	bool IsCacheValid(const FString& ScriptPath) const;

	//=============================================================================
	// Hot-Reload Support
	//=============================================================================
	
	/**
	 * Enable/disable hot-reload (watches script files for changes)
	 */
	UFUNCTION(BlueprintCallable, Category = "Scripting|Development")
	void SetHotReloadEnabled(bool bEnabled);
	
	/**
	 * Check if a script file has been modified since last load
	 */
	bool CheckForScriptChanges(const FString& ScriptName);

	//=============================================================================
	// Console Commands
	//=============================================================================
	
	void RegisterConsoleCommands();
	void UnregisterConsoleCommands();

private:
	//=============================================================================
	// Internal Methods
	//=============================================================================
	
	/** Compile source code to bytecode */
	TSharedPtr<FBytecodeChunk> CompileScript(const FString& SourceCode, TArray<FString>& OutErrors);
	
	/** Resolve script path (handle relative paths, Scripts folder, etc.) */
	FString ResolveScriptPath(const FString& ScriptPath) const;
	
	/** Get cache file path for a script */
	FString GetCacheFilePath(const FString& ScriptPath) const;
	
	/** Initialize VM with native functions */
	void InitializeVM(TSharedPtr<FScriptVM> VM);

	//=============================================================================
	// Member Variables
	//=============================================================================
	
	/** Map of loaded scripts (script name -> compiled script) */
	UPROPERTY()
	TMap<FString, FCompiledScript> LoadedScripts;

public: // Explicitly public
	const TMap<FString, FCompiledScript>& GetLoadedScriptsMap() const { return LoadedScripts; }
	
private: // Re-add private specifier for member variables
	/** Whether hot-reload is enabled */
	UPROPERTY()
	bool bHotReloadEnabled;
	
	/** Scripts folder path (default: ProjectDir/Scripts) */
	UPROPERTY()
	FString ScriptsFolder;
	
	/** Cache folder path (default: ProjectDir/Saved/ScriptCache) */
	UPROPERTY()
	FString CacheFolder;
	
	/** Console command handles */
	TArray<IConsoleObject*> ConsoleCommands;
};

