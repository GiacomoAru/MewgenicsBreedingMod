#include "amoeboid.hpp"
#include "breed.hpp"
#include "breed_logic.hpp"
#include "config.hpp"
#include "snapshot.hpp"
#include "types/glaiel.hpp"
#include "types/msvc.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/function_hook.hpp"

#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <string>

// Hook on glaiel::CatData::breed.
// S3: read-only probe. Logs the inputs and outputs of every call, changes nothing.

bool g_breed_sim_active = false;
LastBreed g_last_breed;

// The last real breeding is kept in last_breed.txt next to the DLL, so the menu can fill the simulator
// even right after a game launch.
static std::filesystem::path last_breed_path() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(reinterpret_cast<HMODULE>(G.dll_base_va), buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path() / "last_breed.txt";
}

void breed_load_last() {
    std::ifstream in(last_breed_path());
    LastBreed lb;
    if(in >> lb.parent_a >> lb.parent_b >> lb.coi) {
        lb.valid = true;
        g_last_breed = lb;
    }
}

static const char *const PART_NAMES[14] = {
    "body", "head", "tail", "leg1", "leg2", "arm1", "arm2",
    "leye", "reye", "lbrow", "rbrow", "lear", "rear", "mouth",
};

static const BodyPartDescriptor *part_slot(const BodyParts &bp, int i) {
    const BodyPartDescriptor *slots[14] = {
        &bp.body, &bp.head, &bp.tail, &bp.leg1, &bp.leg2, &bp.arm1, &bp.arm2,
        &bp.lefteye, &bp.righteye, &bp.lefteyebrow, &bp.righteyebrow,
        &bp.leftear, &bp.rightear, &bp.mouth,
    };
    return slots[i];
}

// raw dump of a std::string: size, capacity, 16 inline bytes in hex, then text
static std::string str_raw(const MsvcReleaseModeXString &s) {
    std::string out = std::format("size={} res={} buf=", s._Mysize, s._Myres);
    for(int i = 0; i < 16; i++) {
        out += std::format("{:02x}", static_cast<uint8_t>(s._Bx._Buf[i]));
    }
    // text only if it is a plausible string (inline, or heap with sane size)
    if(s._Myres >= 15 && s._Myres < 4096 && s._Mysize <= s._Myres) {
        out += std::format(" '{}'", s.as_native_string_view());
    } else {
        out += " <not-a-string>";
    }
    return out;
}

static std::string cat_disorders(const CatData &c) {
    return std::format("m0[{}] lvl={} | m1[{}] lvl={}",
        str_raw(c.mutation_0), c.mutation_0_level, str_raw(c.mutation_1), c.mutation_1_level);
}

static std::string cat_parts(const CatData &c) {
    std::string out;
    for(int i = 0; i < 14; i++) {
        out += std::format("{}={} ", PART_NAMES[i], static_cast<int32_t>(part_slot(c.body_parts, i)->part_sprite_idx));
    }
    out += std::format("tex={}", static_cast<int32_t>(c.body_parts.texture_sprite_idx));
    return out;
}

static void log_cat(const char *label, const CatData &c) {
    D::info("  {} key={} coi={}", label, c.sql_key, c.coi);
    D::info("    disorders: {}", cat_disorders(c));
    D::info("    parts: {}", cat_parts(c));
}

// Inbreeding level 2: drop the kitten's new disorders (see breed_logic.hpp), writing the game's strings with
// destroy() + construct() as cat-bridge does.
static void remove_new_kitten_disorders(CatData &kitten, const CatData &a, const CatData &b) {
    MsvcReleaseModeXString *names[2] = {&kitten.mutation_0, &kitten.mutation_1};
    int64_t *levels[2] = {&kitten.mutation_0_level, &kitten.mutation_1_level};
    DisorderSlot slots[2];
    for(int i = 0; i < 2; i++) {
        slots[i] = {names[i]->copy_to_native_string(), *levels[i]};
    }
    std::vector<std::string> parents;
    for(const CatData *p : {&a, &b}) {
        parents.push_back(p->mutation_0.copy_to_native_string());
        parents.push_back(p->mutation_1.copy_to_native_string());
    }
    DisorderSlot before[2] = {slots[0], slots[1]};
    if(remove_new_disorders(slots, parents, config().whitelist_disorders) == 0) {
        return;
    }
    for(int i = 0; i < 2; i++) {
        if(slots[i].name != before[i].name || slots[i].level != before[i].level) {
            names[i]->destroy();
            names[i]->construct(slots[i].name.data(), slots[i].name.size());
            *levels[i] = slots[i].level;
        }
    }
}

MAKE_SHOOK(0, ADDRESS_glaiel__CatData__breed,
    void, __cdecl, glaiel__CatData__breed,
    CatData *kitten, CatData *parent_a, CatData *parent_b, double coi, void *furniture_effects
) {
    const Config &cfg = config();
    const double real_coi = coi;
    const double passed_coi = scaled_coi(real_coi, cfg.inbreeding);
    const bool logging = !g_breed_sim_active;
    int call_no = 0;
    if(logging) {
        static int counter = 0;
        call_no = ++counter;
        g_last_breed = {true, parent_a->sql_key, parent_b->sql_key, real_coi};
        std::ofstream(last_breed_path()) << parent_a->sql_key << ' ' << parent_b->sql_key << ' ' << std::format("{:.17g}", real_coi) << '\n';
        D::info("[breed #{}] coi_param={} passed_to_game={} (inbreeding {}) kitten_ptr={} A={} B={}", call_no, real_coi, passed_coi,
            cfg.inbreeding, static_cast<void *>(kitten), parent_a->sql_key, parent_b->sql_key);
        log_cat("parentA", *parent_a);
        log_cat("parentB", *parent_b);
    }

    glaiel__CatData__breed_hook.orig(kitten, parent_a, parent_b, passed_coi, furniture_effects);

    kitten->coi = real_coi; // the kitten stays "Inbred" and the pedigree gets the true value
    if(cfg.inbreeding == 2) {
        remove_new_kitten_disorders(*kitten, *parent_a, *parent_b);
    }

    if(logging) {
        log_cat("kitten(after)", *kitten);
        D::info("  kitten coi after = {} (param was {})", kitten->coi, real_coi);
        snapshot_note_breed(call_no, *parent_a, *parent_b, real_coi, *kitten);
    }
}