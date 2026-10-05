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

inline constexpr char MOD_AUTHOR[] = "cicci";
inline constexpr char MOD_NAME[] = "Clean Breeding";
inline constexpr char MOD_IDENTIFIER[] = "cicci.clean_breeding";
inline constexpr char MOD_URL[] = "(no URL yet)";
inline constexpr char MOD_VERSION[] = "0.1.0";

// These addresses were extracted from Mewgenics.exe
// The script under misc/find_rvas.py can help with recovering these addresses after a game update

// Semantic release version of the Mewgenics.exe binary last used to update hardcoded offsets
inline constexpr char EXE_VERSION[] = "1.1.21239";

// SHA-256 hash of the Mewgenics.exe binary last used to update hardcoded offsets
inline constexpr Hash256Bit EXE_SHA256 = c_str_to_hash256bit("4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea");

// Function offsets are encoded as relative VAs

// Data offsets are encoded as relative VAs

// TLS variable offsets are encoded relative to the base VA of their TLS slot

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
};
