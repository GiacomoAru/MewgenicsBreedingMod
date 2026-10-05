## Attribution

This project gratefully uses the following libraries.

### C/C++

* Detours
  * Used for function hooking and DLL injection.
  * `Copyright (c) Microsoft Corporation.`
  * MIT License
  * https://github.com/microsoft/Detours/blob/main/LICENSE
* Combat Roster Panel
  * Source of the code that reads the game's own fonts and paper art from the player's `resources.gpak` at runtime
    (`gamelook/gpak`, `gamelook/swf`, `gamelook/fontloader`, parts of `gamelook/assets`, and the paper drawing, colours
    and renderer-rebuild code in `gamelook/gamelook.cpp` and `menu.cpp`). Nothing from the game is copied or shipped.
  * `Copyright (c) 2026 Combat Roster Panel authors`
  * MIT License
  * https://github.com/TotSamiyMorzh/mewgenics-combat-roster/blob/main/LICENSE
* Dear ImGui
  * Used for the in-game overlay menu.
  * `Copyright (c) 2014-2026 Omar Cornut`  
  * MIT License
  * https://github.com/ocornut/imgui/blob/master/LICENSE.txt
* LibTomCrypt
  * Used to hash Mewgenics.exe to detect version mismatches.
  * `LibTomCrypt, modular cryptographic library -- Tom St Denis`
  * Unlicense
  * https://github.com/libtom/libtomcrypt/blob/develop/LICENSE
* Mewjector
  * Used for coordinated function hooking and logging.
  * `Copyright (c) 2026 Mewjector Contributors`
  * MIT License
  * https://github.com/githubuser508/mewjector/blob/main/LICENSE
* SDL3
  * Used to interface with Mewgenics' game engine.
  * `Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>`
  * zlib License
  * https://github.com/libsdl-org/SDL/blob/main/LICENSE.txt
* stb_image
  * Used for its zlib inflate, to read the bitmaps of the game's SWF files.
  * `Sean Barrett`
  * MIT License / Public Domain
  * https://github.com/nothings/stb/blob/master/LICENSE
* STL
  * Referenced to write `types/msvc.hpp`.
  * `Copyright (c) Microsoft Corporation.`
  * Apache License v2.0 with LLVM Exception
  * https://github.com/microsoft/STL/blob/main/LICENSE.txt

### Python

* pefile
  * Used by `find_rvas.py` to parse Mewgenics.exe's PE structures for signature scanning.
  * `Copyright (c) 2004-2024 Ero Carrera`
  * MIT License
  * https://github.com/erocarrera/pefile/blob/master/LICENSE

