// Copyright Vampire Game Project. All Rights Reserved.
// Script loading, compilation, and execution management subsystem

#include "ScriptManager.h"
#include "ScriptLexer.h"
#include "ScriptParser.h"
#include "ScriptCompiler.h"
#include "ScriptLogger.h"
// #include "ScriptNativeAPI.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

void UScriptManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	SCRIPT_LOG(TEXT("=== Script Manager Initialized (PIE/Game Level) ==="));
	
	// Set up default paths
	ScriptsFolder = FPaths::ProjectDir() / TEXT("Scripts");
	CacheFolder = FPaths::ProjectSavedDir() / TEXT("ScriptCache");
	
	// Create cache folder if it doesn't exist
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*CacheFolder))
	{
		PlatformFile.CreateDirectoryTree(*CacheFolder);
		SCRIPT_LOG(FString::Printf(TEXT("Created cache folder: %s"), *CacheFolder));
	}
	
	// Hot-reload disabled by default (enable in development builds)
	bHotReloadEnabled = false;
	
	// Register console commands
	RegisterConsoleCommands();
	
	SCRIPT_LOG(FString::Printf(TEXT("Scripts Folder: %s"), *ScriptsFolder));
	SCRIPT_LOG(FString::Printf(TEXT("Cache Folder: %s"), *CacheFolder));
	
	// Startup logic: Ensure Main.sc is the root entry point
	// The module might have loaded startup scripts, but we ensure Main is handled here
	if (LoadScript(TEXT("Main.sc")).IsEmpty())
	{
		// Fallback if source not found, try compiled
		LoadScript(TEXT("Main.scc")); 
	}
	
	// Auto-run Main if loaded
	if (IsScriptLoaded(TEXT("Main")))
	{
		ExecuteScript(TEXT("Main"));
	}
	else 
	{
		SCRIPT_LOG_WARNING(TEXT("Main script not found! Game logic may not start."));
	}
}

void UScriptManager::Deinitialize()
{
	SCRIPT_LOG(TEXT("Script Manager Shutting Down"));
	
	// Unload all scripts
	UnloadAllScripts();
	
	// Unregister console commands
	UnregisterConsoleCommands();
	
	Super::Deinitialize();
}

//=============================================================================
// Script Loading & Compilation
//=============================================================================

FString UScriptManager::LoadScript(const FString& ScriptPath, bool bForceRecompile)
{
	// Resolve full path
	FString FullPath = ResolveScriptPath(ScriptPath);
	
	// Extract script name from path
	FString ScriptName = FPaths::GetBaseFilename(FullPath);
	
	SCRIPT_LOG(FString::Printf(TEXT("Loading script: %s from %s"), *ScriptName, *FullPath));
	
	// PRIORITY 1: Check for compiled bytecode (.scc) in Scripts/Compiled/
	// This is the PRIMARY way to run scripts - compiled bytecode always wins!
	FString CompiledPath = FPaths::ProjectDir() / TEXT("Scripts/Compiled") / ScriptName + TEXT(".scc");
	TSharedPtr<FBytecodeChunk> Bytecode;
	
	if (!bForceRecompile && FPaths::FileExists(CompiledPath))
	{
		SCRIPT_LOG(FString::Printf(TEXT("Found compiled bytecode: %s"), *CompiledPath));
		SCRIPT_LOG(TEXT("Loading compiled bytecode directly (source ignored)..."));
		
		// Load compiled bytecode directly
		Bytecode = MakeShared<FBytecodeChunk>();
		TArray<uint8> BytecodeData;
		if (FFileHelper::LoadFileToArray(BytecodeData, *CompiledPath))
		{
			if (Bytecode->Deserialize(BytecodeData))
			{
				SCRIPT_LOG(FString::Printf(TEXT("Successfully loaded compiled bytecode (%d bytes)"), BytecodeData.Num()));
			}
			else
			{
				SCRIPT_LOG_ERROR(TEXT("Failed to deserialize compiled bytecode"));
				Bytecode.Reset();
			}
		}
		else
		{
			SCRIPT_LOG_ERROR(FString::Printf(TEXT("Failed to load compiled bytecode file: %s"), *CompiledPath));
			Bytecode.Reset();
		}
	}
	
	// PRIORITY 2: If no compiled bytecode, check for source file and compile
	if (!Bytecode.IsValid())
	{
		SCRIPT_LOG(TEXT("No compiled bytecode found, checking for source file..."));
		
		// Check if source file exists
		if (!FPaths::FileExists(FullPath))
		{
			SCRIPT_LOG_ERROR(FString::Printf(TEXT("Neither compiled bytecode nor source file found for: %s"), *ScriptName));
			return TEXT("");
		}
		
		// Load source code from file
		FString SourceCode;
		if (!FFileHelper::LoadFileToString(SourceCode, *FullPath))
		{
			SCRIPT_LOG_ERROR(FString::Printf(TEXT("Failed to read source file: %s"), *FullPath));
			return TEXT("");
		}
		
		SCRIPT_LOG(TEXT("Compiling source code..."));
		
		// Compile source
		TArray<FString> Errors;
		Bytecode = CompileScript(SourceCode, Errors);
		
		if (!Bytecode.IsValid())
		{
			SCRIPT_LOG_ERROR(FString::Printf(TEXT("Failed to compile script: %s"), *ScriptName));
			for (const FString& Error : Errors)
			{
				SCRIPT_LOG_ERROR(FString::Printf(TEXT("  %s"), *Error));
			}
			return TEXT("");
		}
		
		// Populate metadata
		Bytecode->Metadata.SourceFileName = ScriptName + TEXT(".sc");
		Bytecode->Metadata.SourceFileSize = SourceCode.Len();
		Bytecode->Metadata.SourceChecksum = FMD5::HashAnsiString(*SourceCode);
		
		// Generate signature
		Bytecode->Signature = Bytecode->GenerateSignature();
		
		// Save compiled bytecode to Scripts/Compiled/
		FString CompiledDir = FPaths::ProjectDir() / TEXT("Scripts/Compiled");
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*CompiledDir))
		{
			PlatformFile.CreateDirectoryTree(*CompiledDir);
		}
		
		TArray<uint8> BytecodeData;
		if (Bytecode->Serialize(BytecodeData, true))
		{
			if (FFileHelper::SaveArrayToFile(BytecodeData, *CompiledPath))
			{
				float CompressionRatio = (float)BytecodeData.Num() / (float)SourceCode.Len();
				SCRIPT_LOG(FString::Printf(TEXT("Saved compiled bytecode: %s (%d bytes, %.1f%% of source)"),
					*CompiledPath, BytecodeData.Num(), CompressionRatio * 100.0f));
			}
		}
	}
	
	// Create compiled script entry
	FCompiledScript CompiledScript;
	CompiledScript.SourcePath = FullPath;
	CompiledScript.Bytecode = Bytecode;
	CompiledScript.VM = MakeShared<FScriptVM>();
	CompiledScript.LastModified = FDateTime::Now();
	CompiledScript.bExecuted = false;
	
	// Initialize VM with native functions
	InitializeVM(CompiledScript.VM);
	
	// Store in loaded scripts map
	LoadedScripts.Add(ScriptName, CompiledScript);
	
	SCRIPT_LOG(FString::Printf(TEXT("Script loaded successfully: %s"), *ScriptName));
	return ScriptName;
}

bool UScriptManager::LoadScriptFromString(const FString& ScriptName, const FString& SourceCode)
{
	SCRIPT_LOG(FString::Printf(TEXT("Loading script from string: %s"), *ScriptName));
	
	// Compile source code
	TArray<FString> Errors;
	TSharedPtr<FBytecodeChunk> Bytecode = CompileScript(SourceCode, Errors);
	
	if (!Bytecode.IsValid())
	{
		SCRIPT_LOG_ERROR(FString::Printf(TEXT("Failed to compile script: %s"), *ScriptName));
		for (const FString& Error : Errors)
		{
			SCRIPT_LOG_ERROR(FString::Printf(TEXT("  %s"), *Error));
		}
		return false;
	}
	
	// Create compiled script entry
	FCompiledScript CompiledScript;
	CompiledScript.SourcePath = TEXT("<string>");
	CompiledScript.Bytecode = Bytecode;
	CompiledScript.VM = MakeShared<FScriptVM>();
	CompiledScript.LastModified = FDateTime::Now();
	CompiledScript.bExecuted = false;
	
	// Initialize VM with native functions
	InitializeVM(CompiledScript.VM);
	
	// Store in loaded scripts map
	LoadedScripts.Add(ScriptName, CompiledScript);
	
	SCRIPT_LOG(FString::Printf(TEXT("Script loaded successfully from string: %s"), *ScriptName));
	return true;
}

FString UScriptManager::LoadCompiledBytecode(const FString& BytecodePath)
{
	// Resolve full path (support both relative and absolute paths)
	FString FullPath = BytecodePath;
	if (!FPaths::FileExists(FullPath))
	{
		// Try Scripts/Compiled/ folder
		FullPath = FPaths::ProjectDir() / TEXT("Scripts/Compiled") / BytecodePath;
		if (!FPaths::FileExists(FullPath))
		{
			// Try adding .scc extension
			FullPath = FPaths::ProjectDir() / TEXT("Scripts/Compiled") / BytecodePath + TEXT(".scc");
		}
	}
	
	// Extract script name
	FString ScriptName = FPaths::GetBaseFilename(FullPath);
	
	SCRIPT_LOG(FString::Printf(TEXT("Loading compiled bytecode: %s from %s"), *ScriptName, *FullPath));
	
	// Check if file exists
	if (!FPaths::FileExists(FullPath))
	{
		SCRIPT_LOG_ERROR(FString::Printf(TEXT("Compiled bytecode file not found: %s"), *FullPath));
		return TEXT("");
	}
	
	// Load bytecode file
	TArray<uint8> BytecodeData;
	if (!FFileHelper::LoadFileToArray(BytecodeData, *FullPath))
	{
		SCRIPT_LOG_ERROR(FString::Printf(TEXT("Failed to load bytecode file: %s"), *FullPath));
		return TEXT("");
	}
	
	// Deserialize bytecode
	TSharedPtr<FBytecodeChunk> Bytecode = MakeShared<FBytecodeChunk>();
	if (!Bytecode->Deserialize(BytecodeData))
	{
		SCRIPT_LOG_ERROR(FString::Printf(TEXT("Failed to deserialize bytecode: %s"), *FullPath));
		return TEXT("");
	}
	
	SCRIPT_LOG(FString::Printf(TEXT("Successfully loaded compiled bytecode (%d bytes)"), BytecodeData.Num()));
	SCRIPT_LOG(FString::Printf(TEXT("  Compiler: %s %s"), *Bytecode->Metadata.CompilerName, *Bytecode->Metadata.CompilerVersion));
	SCRIPT_LOG(FString::Printf(TEXT("  Game: %s %s"), *Bytecode->Metadata.GameName, *Bytecode->Metadata.GameVersion));
	SCRIPT_LOG(FString::Printf(TEXT("  Trusted: %s"), Bytecode->IsTrustedCompiler() ? TEXT("YES") : TEXT("NO")));
	
	// Create compiled script entry
	FCompiledScript CompiledScript;
	CompiledScript.SourcePath = FullPath;
	CompiledScript.Bytecode = Bytecode;
	CompiledScript.VM = MakeShared<FScriptVM>();
	CompiledScript.LastModified = FDateTime::Now();
	CompiledScript.bExecuted = false;
	
	// Initialize VM with native functions
	InitializeVM(CompiledScript.VM);
	
	// Store in loaded scripts map
	LoadedScripts.Add(ScriptName, CompiledScript);
	
	SCRIPT_LOG(FString::Printf(TEXT("Compiled bytecode loaded successfully: %s"), *ScriptName));
	return ScriptName;
}

bool UScriptManager::IsScriptLoaded(const FString& ScriptName) const
{
	return LoadedScripts.Contains(ScriptName);
}

bool UScriptManager::IsScriptRunning(const FString& ScriptName) const
{
	const FCompiledScript* Script = LoadedScripts.Find(ScriptName);
	return Script && Script->bExecuted;
}

TArray<FString> UScriptManager::GetLoadedScripts() const
{
	TArray<FString> ScriptNames;
	LoadedScripts.GetKeys(ScriptNames);
	return ScriptNames;
}

//=============================================================================
// Script Execution
//=============================================================================

bool UScriptManager::ExecuteScript(const FString& ScriptName, bool bCallMain)
{
	FCompiledScript* Script = LoadedScripts.Find(ScriptName);
	if (!Script)
	{
		SCRIPT_LOG_ERROR(FString::Printf(TEXT("Script not loaded: %s"), *ScriptName));
		return false;
	}
	
	SCRIPT_LOG(FString::Printf(TEXT("Executing script: %s"), *ScriptName));
	
	// Reset VM if already executed
	if (Script->bExecuted)
	{
		Script->VM->Reset();
		InitializeVM(Script->VM);
	}
	
	// Execute bytecode
	bool bSuccess = Script->VM->Execute(Script->Bytecode);
	
	if (!bSuccess)
	{
		SCRIPT_LOG_ERROR(FString::Printf(TEXT("Script execution failed: %s"), *ScriptName));
		for (const FString& Error : Script->VM->GetErrors())
		{
			SCRIPT_LOG_ERROR(FString::Printf(TEXT("  %s"), *Error));
		}
		return false;
	}
	
	// Call Main() if requested
	if (bCallMain)
	{
		if (Script->VM->CallMainIfExists())
		{
			SCRIPT_LOG(FString::Printf(TEXT("Main() function executed for script: %s"), *ScriptName));
		}
	}
	
	Script->bExecuted = true;
	SCRIPT_LOG(FString::Printf(TEXT("Script executed successfully: %s"), *ScriptName));
	return true;
}

FString UScriptManager::CallScriptFunction(const FString& ScriptName, const FString& FunctionName, const TArray<FString>& Args)
{
	SCRIPT_LOG(FString::Printf(TEXT("Calling function %s in script: %s"), *FunctionName, *ScriptName));
	
	// TODO: Implement function call support
	// This requires extending the VM API to support direct function calls by name
	
	SCRIPT_LOG_WARNING(TEXT("CallScriptFunction not yet implemented"));
	return TEXT("");
}

void UScriptManager::StopScript(const FString& ScriptName)
{
	FCompiledScript* Script = LoadedScripts.Find(ScriptName);
	if (Script && Script->VM.IsValid())
	{
		Script->VM->Reset();
		SCRIPT_LOG(FString::Printf(TEXT("Script stopped: %s"), *ScriptName));
	}
}

//=============================================================================
// Script Management
//=============================================================================

void UScriptManager::UnloadScript(const FString& ScriptName)
{
	if (LoadedScripts.Remove(ScriptName) > 0)
	{
		SCRIPT_LOG(FString::Printf(TEXT("Script unloaded: %s"), *ScriptName));
	}
}

void UScriptManager::UnloadAllScripts()
{
	int32 Count = LoadedScripts.Num();
	LoadedScripts.Empty();
	SCRIPT_LOG(FString::Printf(TEXT("Unloaded %d scripts"), Count));
}

bool UScriptManager::ReloadScript(const FString& ScriptName)
{
	FCompiledScript* Script = LoadedScripts.Find(ScriptName);
	if (!Script)
	{
		SCRIPT_LOG_ERROR(FString::Printf(TEXT("Cannot reload - script not loaded: %s"), *ScriptName));
		return false;
	}
	
	FString SourcePath = Script->SourcePath;
	UnloadScript(ScriptName);
	
	FString LoadedName = LoadScript(SourcePath, true); // Force recompile
	return !LoadedName.IsEmpty();
}

TArray<FString> UScriptManager::GetScriptErrors(const FString& ScriptName) const
{
	const FCompiledScript* Script = LoadedScripts.Find(ScriptName);
	if (Script && Script->VM.IsValid())
	{
		return Script->VM->GetErrors();
	}
	return TArray<FString>();
}

//=============================================================================
// Bytecode Caching
//=============================================================================

bool UScriptManager::SaveBytecodeCache(const FString& ScriptName, TSharedPtr<FBytecodeChunk> Bytecode)
{
	if (!Bytecode.IsValid())
	{
		return false;
	}
	
	FString CachePath = CacheFolder / ScriptName + TEXT(".scc");
	
	// Serialize bytecode to binary format
	TArray<uint8> BinaryData;
	FMemoryWriter Ar(BinaryData);
	
	// Write magic number
	uint32 MagicNumber = 0x53424300; // "SBC\0"
	Ar << MagicNumber;
	
	// Write version
	uint32 Version = 1;
	Ar << Version;
	
	// Write bytecode
	Ar << Bytecode->Code;
	
	// Write constants
	int32 ConstantCount = Bytecode->Constants.Num();
	Ar << ConstantCount;
	for (int32 i = 0; i < ConstantCount; ++i)
	{
		const FScriptValue& Value = Bytecode->Constants[i];
		uint8 Type = (uint8)Value.Type;
		Ar << Type;
		
		switch (Value.Type)
		{
			case EValueType::NUMBER:
				Ar << const_cast<double&>(Value.NumberValue);
				break;
			case EValueType::BOOL:
				Ar << const_cast<bool&>(Value.BoolValue);
				break;
			case EValueType::STRING:
				Ar << const_cast<FString&>(Value.StringValue);
				break;
			default:
				break;
		}
	}
	
	// Write line numbers
	Ar << Bytecode->LineNumbers;
	
	// Write function table
	int32 FunctionCount = Bytecode->Functions.Num();
	Ar << FunctionCount;
	for (int32 i = 0; i < FunctionCount; ++i)
	{
		const FFunctionInfo& Func = Bytecode->Functions[i];
		Ar << const_cast<FString&>(Func.Name);
		Ar << const_cast<int32&>(Func.Address);
		Ar << const_cast<int32&>(Func.Arity);
	}
	
	// Save to file
	if (FFileHelper::SaveArrayToFile(BinaryData, *CachePath))
	{
		SCRIPT_LOG(FString::Printf(TEXT("Saved bytecode cache: %s (%d bytes)"), *CachePath, BinaryData.Num()));
		return true;
	}
	
	SCRIPT_LOG_ERROR(FString::Printf(TEXT("Failed to save bytecode cache: %s"), *CachePath));
	return false;
}

TSharedPtr<FBytecodeChunk> UScriptManager::LoadBytecodeCache(const FString& ScriptPath)
{
	FString ScriptName = FPaths::GetBaseFilename(ScriptPath);
	FString CachePath = CacheFolder / ScriptName + TEXT(".scc");
	
	// Load binary data
	TArray<uint8> BinaryData;
	if (!FFileHelper::LoadFileToArray(BinaryData, *CachePath))
	{
		return nullptr;
	}
	
	FMemoryReader Ar(BinaryData);
	
	// Read magic number
	uint32 MagicNumber = 0;
	Ar << MagicNumber;
	if (MagicNumber != 0x53424300)
	{
		SCRIPT_LOG_WARNING(FString::Printf(TEXT("Invalid bytecode cache (bad magic): %s"), *CachePath));
		return nullptr;
	}
	
	// Read version
	uint32 Version = 0;
	Ar << Version;
	if (Version != 1)
	{
		SCRIPT_LOG_WARNING(FString::Printf(TEXT("Incompatible bytecode cache version: %d"), Version));
		return nullptr;
	}
	
	// Create bytecode chunk
	TSharedPtr<FBytecodeChunk> Bytecode = MakeShared<FBytecodeChunk>();
	
	// Read bytecode
	Ar << Bytecode->Code;
	
	// Read constants
	int32 ConstantCount = 0;
	Ar << ConstantCount;
	Bytecode->Constants.Reserve(ConstantCount);
	
	for (int32 i = 0; i < ConstantCount; ++i)
	{
		uint8 Type = 0;
		Ar << Type;
		
		FScriptValue Value;
		Value.Type = (EValueType)Type;
		
		switch (Value.Type)
		{
			case EValueType::NUMBER:
				Ar << Value.NumberValue;
				break;
			case EValueType::BOOL:
				Ar << Value.BoolValue;
				break;
			case EValueType::STRING:
				Ar << Value.StringValue;
				break;
			default:
				break;
		}
		
		Bytecode->Constants.Add(Value);
	}
	
	// Read line numbers
	Ar << Bytecode->LineNumbers;
	
	// Read function table
	int32 FunctionCount = 0;
	Ar << FunctionCount;
	Bytecode->Functions.Reserve(FunctionCount);
	
	for (int32 i = 0; i < FunctionCount; ++i)
	{
		FFunctionInfo Func;
		Ar << Func.Name;
		Ar << Func.Address;
		Ar << Func.Arity;
		Bytecode->Functions.Add(Func);
	}
	
	SCRIPT_LOG(FString::Printf(TEXT("Loaded bytecode cache: %s (%d bytes, %d functions)"), 
		*CachePath, BinaryData.Num(), FunctionCount));
	return Bytecode;
}

bool UScriptManager::IsCacheValid(const FString& ScriptPath) const
{
	FString ScriptName = FPaths::GetBaseFilename(ScriptPath);
	FString CachePath = CacheFolder / ScriptName + TEXT(".scc");
	
	// Check if cache file exists
	if (!FPaths::FileExists(CachePath))
	{
		return false;
	}
	
	// Check if source is newer than cache
	FDateTime SourceTime = IFileManager::Get().GetTimeStamp(*ScriptPath);
	FDateTime CacheTime = IFileManager::Get().GetTimeStamp(*CachePath);
	
	return CacheTime > SourceTime;
}

//=============================================================================
// Hot-Reload Support
//=============================================================================

void UScriptManager::SetHotReloadEnabled(bool bEnabled)
{
	bHotReloadEnabled = bEnabled;
	SCRIPT_LOG(FString::Printf(TEXT("Hot-reload %s"), bEnabled ? TEXT("enabled") : TEXT("disabled")));
	
	// TODO: Implement file watching with FDirectoryWatcherModule
}

bool UScriptManager::CheckForScriptChanges(const FString& ScriptName)
{
	FCompiledScript* Script = LoadedScripts.Find(ScriptName);
	if (!Script || Script->SourcePath == TEXT("<string>"))
	{
		return false;
	}
	
	FDateTime CurrentModTime = IFileManager::Get().GetTimeStamp(*Script->SourcePath);
	return CurrentModTime > Script->LastModified;
}

//=============================================================================
// Console Commands
//=============================================================================

void UScriptManager::RegisterConsoleCommands()
{
	// script.load <path>
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("script.load"),
		TEXT("Load and compile a script file"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			if (Args.Num() < 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("Usage: script.load <path>"));
				return;
			}
			LoadScript(Args[0]);
		}),
		ECVF_Default
	));
	
	// script.exec <name>
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("script.exec"),
		TEXT("Execute a loaded script"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			if (Args.Num() < 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("Usage: script.exec <name>"));
				return;
			}
			ExecuteScript(Args[0]);
		}),
		ECVF_Default
	));
	
	// script.reload <name>
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("script.reload"),
		TEXT("Reload a script (hot-reload)"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			if (Args.Num() < 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("Usage: script.reload <name>"));
				return;
			}
			ReloadScript(Args[0]);
		}),
		ECVF_Default
	));
	
	// script.list
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("script.list"),
		TEXT("List all loaded scripts"),
		FConsoleCommandDelegate::CreateLambda([this]()
		{
			TArray<FString> Scripts = GetLoadedScripts();
			UE_LOG(LogTemp, Log, TEXT("Loaded scripts (%d):"), Scripts.Num());
			for (const FString& Script : Scripts)
			{
				UE_LOG(LogTemp, Log, TEXT("  - %s"), *Script);
			}
		}),
		ECVF_Default
	));
	
	// script.unload <name>
	ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("script.unload"),
		TEXT("Unload a script"),
		FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
		{
			if (Args.Num() < 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("Usage: script.unload <name>"));
				return;
			}
			UnloadScript(Args[0]);
		}),
		ECVF_Default
	));
}

void UScriptManager::UnregisterConsoleCommands()
{
	for (IConsoleObject* Cmd : ConsoleCommands)
	{
		IConsoleManager::Get().UnregisterConsoleObject(Cmd);
	}
	ConsoleCommands.Empty();
}

//=============================================================================
// Internal Methods
//=============================================================================

TSharedPtr<FBytecodeChunk> UScriptManager::CompileScript(const FString& SourceCode, TArray<FString>& OutErrors)
{
	OutErrors.Empty();
	
	// Lexer
	FScriptLexer Lexer(SourceCode);
	TArray<FScriptToken> Tokens = Lexer.ScanTokens();
	
	if (Lexer.HasErrors())
	{
		OutErrors.Append(Lexer.GetErrors());
		return nullptr;
	}
	
	// Parser
	FScriptParser Parser(Tokens);
	TSharedPtr<FScriptProgram> Program = Parser.Parse();
	
	if (Parser.HasErrors() || !Program.IsValid() || !Program->IsValid())
	{
		OutErrors.Append(Parser.GetErrors());
		return nullptr;
	}
	
	// Compiler
	FScriptCompiler Compiler;
	TSharedPtr<FBytecodeChunk> Bytecode = Compiler.Compile(Program);
	
	if (!Bytecode.IsValid() || Compiler.HasErrors())
	{
		OutErrors.Append(Compiler.GetErrors());
		return nullptr;
	}
	
	return Bytecode;
}

FString UScriptManager::ResolveScriptPath(const FString& ScriptPath) const
{
	// If absolute path, use as-is
	if (FPaths::IsRelative(ScriptPath))
	{
		// Try relative to Scripts folder
		FString TestPath = ScriptsFolder / ScriptPath;
		if (FPaths::FileExists(TestPath))
		{
			return FPaths::ConvertRelativePathToFull(TestPath);
		}
		
		// Try relative to project folder
		TestPath = FPaths::ProjectDir() / ScriptPath;
		if (FPaths::FileExists(TestPath))
		{
			return FPaths::ConvertRelativePathToFull(TestPath);
		}
		
		// Return Scripts folder path anyway
		return FPaths::ConvertRelativePathToFull(ScriptsFolder / ScriptPath);
	}
	
	return FPaths::ConvertRelativePathToFull(ScriptPath);
}

FString UScriptManager::GetCacheFilePath(const FString& ScriptPath) const
{
	FString ScriptName = FPaths::GetBaseFilename(ScriptPath);
	return CacheFolder / ScriptName + TEXT(".scc");
}

void UScriptManager::InitializeVM(TSharedPtr<FScriptVM> VM)
{
	if (!VM.IsValid())
	{
		return;
	}
	
	// Call the delegate to let the game module register its functions
	OnRegisterNativeAPI.Broadcast(VM.Get());
}