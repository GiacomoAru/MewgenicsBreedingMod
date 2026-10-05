#pragma once

#include "utilities/checksum.hpp"
#include "utilities/signature.hpp"

#include <cstdint>
#include <optional>

// Main program declarations.
//
// polymeric 2026

// CONSTANTS

// Mod information

inline constexpr char MOD_AUTHOR[] = "Geco";
inline constexpr char MOD_NAME[] = "Unnatural Selection";
inline constexpr char MOD_IDENTIFIER[] = "geco.unnatural_selection";
inline constexpr char MOD_URL[] = "https://www.nexusmods.com/mewgenics/mods/543";
inline constexpr char MOD_VERSION[] = "1.0.0";

// These addresses were extracted from Mewgenics.exe
// The script under misc/find_rvas.py can help with recovering these addresses after a game update

// Semantic release version of the Mewgenics.exe binary last used to update hardcoded offsets
inline constexpr char EXE_VERSION[] = "1.1.21239";

// SHA-256 hash of the Mewgenics.exe binary last used to update hardcoded offsets
inline constexpr Hash256Bit EXE_SHA256 = c_str_to_hash256bit("4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea");

// Function offsets are encoded as relative VAs
inline constexpr const auto ADDRESS_glaiel__CatData_ctor = DirectSig::make<"48 89 4C 24 08 48 83 EC 28 4C 8B C1 45 33 C9 4C 89 49 08 4C 89 49 10 0F 57 C0 0F 11 41 18 4C 89 49 28">(0);
inline constexpr const auto ADDRESS_glaiel__CatData_dtor = DirectSig::make<"40 53 48 83 EC 20 48 8B D9 48 81 C1 10 0C 00 00 E8 ?? ?? ?? ?? 48 8D 8B 90 0B 00 00 E8 ?? ?? ??">(0);
inline constexpr const auto ADDRESS_glaiel__CatData_unk_init = DirectSig::make<"48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC B0 00 00 00 45 0F B6 E1">(0);
inline constexpr const auto ADDRESS_glaiel__CatData_unk_init_bodyparts = DirectSig::make<"40 53 55 56 41 56 41 57 48 83 EC 60 48 8B D9 0F 57 C0 45 33 FF B9 20 00 00 00 0F 11 44 24 40 4C 89 7C 24 50">(0);
inline constexpr const auto ADDRESS_glaiel__CatData__breed = IndirectSig::make<"48 8B CB E8 ?? ?? ?? ?? 48 8B F8 48 8B D6 49 8B CD E8 ?? ?? ?? ?? 48 8B D8 48 8B D5 49 8B CD E8 ?? ?? ?? ?? 4C 89 74 24 20 0F 28 DE 4C 8B C3 48 8B D0 48 8B CF E8 ?? ?? ?? ??">(54, 4, true, true);

// Data offsets are encoded as relative VAs
// always_update: per-frame pump on the game thread
inline constexpr const auto ADDRESS_glaiel__MewDirector__always_update = DirectSig::make<"48 8B 05 ?? ?? ?? ?? F2 0F 10 05 ?? ?? ?? ?? 48 FF 81 30 05 00 00 F2 0F 5E 80 C8 0D 00 00 F2 0F 58 81 38 05 00 00">(0);
inline constexpr const auto DATAOFF_glaiel__MewDirector__p_singleton = IndirectSig::make<"48 89 5C 24 10 48 89 4C 24 08 57 48 83 EC 40 48 8B CA 48 8B 05 ?? ?? ?? ?? 48 8B B8 A8 05 00 00">(21, 4, true, true);

// TLS variable offsets are encoded relative to the base VA of their TLS slot
inline constexpr const auto TLS0OFF_xoshiro256p_rng_context = IndirectSig::make<"48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC B0 00 00 00 45 0F B6 E1 41 8B F0 48 8B F9 45 33 ED 41 BE ?? ?? ?? ??">(43, 4, false, false);

// Mod entry point, called after the exe hash check and hook installation succeeded.
// Exporter: clean_breeding.cpp
void clean_breeding_init();

// CROSS-TU DECLARATIONS

// The "everything" struct
// Exporter: amoeboid.cpp
struct GlobalContext;
extern GlobalContext G;

// TYPE DECLARATIONS

struct GlobalContext {
    // amoeboid.dll offset.
    uintptr_t dll_base_va;
    uintptr_t dll_image_size;

    // Mewgenics.exe offset.
    uintptr_t host_exec_base_va;
    uintptr_t host_exec_image_size;

    // Whether it is permissible for the dll to self-eject.
    // (false if the dll cannot self-uninstall its hooks)
    bool dll_can_self_eject;

    // Mewgenics.exe hash.
    std::optional<Hash256Bit> exe_actual_sha256;
    bool exe_hash_mismatch_detected;

    // True once the breeding hook is installed (supported game version, all signatures found).
    bool mod_active = false;
};
