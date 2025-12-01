## Interaction Guidelines

**IMPORTANT:**
- **Communicate Clearly:** The user prefers an interactive style. **Always explain your plan** before executing complex tasks or using tools that modify code.
- **Confirm Risky Actions:** If an action involves deleting files or significant refactoring, wait for user confirmation or clearly state the intention first.
- **Do Not Be Silent:** Avoid the "ghost" mode where tools are used without explanation. A brief "I am checking X" or "I will now update Y" is required.
- **Always talk before using Tools.**
- **BE INTERACTIVE:** Do NOT be autonomous. Talk to the user. Ask questions. Confirm steps. DO NOT just go and do a massive chain of actions without keeping the user in the loop.
- **Stop and Ask:** If a task seems complex or involves multiple files, break it down and ASK the user if the plan looks good before proceeding.

## Development Philosophy
- **"Human-Like" Placeholders:** When implementing complex systems (like Mission logic, Save systems, State machines), it is acceptable and encouraged to use **Placeholders** or **Stub Implementations**.
    - Add a `TODO:` comment explaining what *will* happen there.
    - Return a safe default value (e.g., `true` for permissions, `false` for checks).
    - This allows us to build the architecture skeleton first and fill in the implementation details later, similar to how a human developer works.

## Project Specifics
- **InputManager:** Remember to implement and maintain a robust `InputManager` for context switching and control management (UI vs Game).
- **Code Editing Safety:** Be extremely careful when editing code to avoid breaking existing implementations. Use `view_file` to verify context before replacing.
- **Skeleton Structure:** Use the provided skeleton structure for precise animation controls (Bone Masks, IK, etc.).
- **Scripting System:** The project uses a custom scripting system. The plugin is named `Scripting` (formerly `SandboxScripting`). Always ensure API macros are `SCRIPTING_API` or `JUSTLIVE_API` as appropriate.
- **Native API:** The native API bridge is `ScriptNativeAPI`. It serves as the template for registering C++ functions to the script VM.
- **Compiler:** There is a standalone compiler tool in `Tools/StandaloneScriptCompiler`.

## Scripting Architecture

The project maintains two parallel but distinct environments for the scripting language:

### 1. In-Game Environment (Embedded)
*   **Location:** `Plugins/Scripting/Source/Scripting/...`
*   **Context:** Runs inside the Unreal Engine game process (Runtime).
*   **Dependencies:** Links against `Core`, `Engine`, and other UE modules.
*   **Entry Point:** The `ScriptManager` will always attempt to load and run `@data/Scripts/Compiled/Main.scc` (or `Main.sc`) on initialization. This script is responsible for bootstrapping the rest of the game logic.
*   **Key Classes:**
    *   `FScriptVM`: The Virtual Machine that executes bytecode.
    *   `ScriptNativeAPI`: Bridges C++ functions to the VM.
*   **API Macro:** `SCRIPTING_API` (exports/imports symbols from the DLL).

### 2. Standalone Toolchain (External)
*   **Location:** `Tools/StandaloneScriptCompiler/...`
*   **Context:** Runs as a standalone command-line tool (CLI) on the OS.
*   **Dependencies:** **NO** Unreal Engine runtime dependency. Uses a `Platform.h` shim to mimic UE types (`FString`, `TArray`, etc.) using the C++ Standard Library.
*   **Purpose:** Compiles source scripts (`.sc`) into bytecode (`.scc`) offline. This allows for external build pipelines and verification without running the Editor.
*   **Key Classes:**
    *   `ScriptCompiler`: The core compiler logic (shared with embedded, but used here in isolation).
    *   `StandaloneMain.cpp`: The entry point for the CLI tool.

**Note:** When modifying shared core files (Lexer, Parser, Compiler), ensure changes are compatible with *both* environments. The `Platform.h` file in the standalone tool handles the abstraction.
