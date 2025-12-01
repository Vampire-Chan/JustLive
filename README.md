# JustLive Project - Moddable Open World Game

## Project Vision

JustLive is an ambitious open-world game project focused on extreme moddability, performance, and a C++-driven core architecture. Our goal is to create a vibrant, dynamic sandbox where players and modders have unparalleled control over the game world, characters, and systems.

Forget the constraints of traditional game engines; we're building a highly optimized experience that empowers creativity.

## Key Architectural Principles

We are consciously building a game that moves away from heavy reliance on Unreal Engine's visual scripting (Blueprints) and asset pipeline for core gameplay logic, opting instead for:

1.  **C++-Driven Core:** All critical gameplay systems, movement, animation, AI, and game managers are implemented in optimized C++ for maximum performance and control.
2.  **Data-Driven System:** Game data (character stats, item properties, quest definitions) is managed via custom binary formats and text-based (XML/JSON) definitions, making it easily moddable and highly configurable.
3.  **Virtual File System (VFS):** Introducing a custom VFS with specialized "J-Formats" (e.g., `.jdm`, `.jtc`) to manage content efficiently, reduce disk clutter, and enable seamless modding without requiring Unreal Editor installations for content creators.
4.  **Custom Scripting Language:** A dedicated scripting language, compiled to bytecode (`.scc`), provides high-level logic and extensive native API access to the C++ engine.
5.  **Minimal Editor Use for Modders:** Our philosophy aims to free modders from the burden of installing and learning the Unreal Editor. Modders should be able to create and inject new content using simple external tools (J-Tools), text editors, and 3D modeling software.

## Current State & Features

*   **Core Game Module:** `JustLive` module with custom components and managers.
*   **Custom Scripting System:**
    *   **Compiler:** Standalone compiler (`.sc` to `.scc` bytecode).
    *   **Runtime VM:** Embedded in-game Virtual Machine.
    *   **Native API:** Extensive C++ API exposed to scripts (Math, String, Collections, Audio, UI, Lighting, VFS).
*   **Virtual File System (VFS):**
    *   Initial implementation of `UVFSManager` with generic archive handling and custom `.jdm`, `.jim`, `.jtc`, `.jan`, `.jab`, `.jmv` container definitions.
    *   Default mount points (`@content`, `@data`, `@scripts`, `@mods`) are supported for flexible pathing.
*   **Player/Pedestrian System:**
    *   C++-driven `APed` and `APlayerPed` base classes.
    *   C++-driven movement and camera control using Unreal's `Enhanced Input System`.
    *   `UPedAnimInstance` ready for a fully C++-controlled animation graph.
    *   Default Mannequin/Quinn references purged from C++ code.
*   **Specialized Audio Players:**
    *   `UMusicPlayer`, `USFXPlayer`, `USpeechPlayer` for high-level audio control, managed by `UAudioManager`.

## Future Goals & Contribution Opportunities

This project is in active development, laying down a robust foundation for a truly moddable experience. We are currently focusing on core systems, with many exciting features planned:

*   **Custom Asset Formats & Loaders:** Full implementation of `.jdm` (mesh dictionaries), `.jtc` (texture containers), `.jan` (animations), `.jab` (audio banks), `.jmv` (movies) loaders, designed to replace traditional `.uasset` workflow for modders.
*   **J-Tools Toolchain:** Development of standalone, lightweight tools (Packer, Mapper, Asset Editor) for modders to create content outside of the Unreal Editor.
*   **JMap (World Definition):** Implementation of a text-based (XML) to binary (`.jmap`) world format, enabling external level design.
*   **Modular Character System:** Data-driven system for swapping character body parts and animations.
*   **Modular Chaos Vehicle System:** Custom C++ vehicle physics and handling, supporting diverse vehicle types (cars, helis, bikes, boats, planes, hovercrafts) via script-configurable components.
*   **Advanced AI & Gameplay Systems:** Full C++ implementation of AI behaviors, mission systems, and core gameplay mechanics.

### **How You Can Contribute!**

We are building a community around this vision. If you are passionate about open-world games, modding, and robust C++ development, we'd love your help!

**Especially, we are looking for contributors who can help with:**

*   **3D Artists:** To create unique, modular player/pedestrian meshes, animations, and vehicle models.
*   **Animators:** To bring life to our characters with C++-ready animation sequences.
*   **Sound Designers:** To create audio assets for our custom audio banks.
*   **C++ Developers:** To enhance core systems, build J-Tools, implement custom loaders, or expand native APIs.

**Do you hate new devs ruining games without any improvements? Do you believe games should be truly moddable and empowering? Join us!**

---

This project aims to demonstrate that a small team can build a game with AAA-level moddability and control, by carefully architecting around Unreal Engine's strengths and weaknesses.

Welcome to JustLive!
