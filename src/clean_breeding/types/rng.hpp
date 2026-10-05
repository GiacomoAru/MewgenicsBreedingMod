#pragma once

#include <cstdint>

// Game RNG state (xoshiro256+), from Amoeba types/glaiel_utility.hpp.
struct Xoshiro256pContext {
    uint64_t ctx[4];
};
