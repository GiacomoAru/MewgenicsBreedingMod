#include "amoeboid.hpp"
#include "snapshot.hpp"
#include "types/glaiel.hpp"
#include "types/msvc.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/function_hook.hpp"

#include <cstdint>
#include <cstring>
#include <string>

// Hook on glaiel::CatData::breed.
// S3: read-only probe. Logs the inputs and outputs of every call, changes nothing.

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

MAKE_SHOOK(0, ADDRESS_glaiel__CatData__breed,
    void, __cdecl, glaiel__CatData__breed,
    CatData *kitten, CatData *parent_a, CatData *parent_b, double coi, void *furniture_effects
) {
    static int call_no = 0;
    ++call_no;
    D::info("[breed #{}] coi_param={} kitten_ptr={} A={} B={}", call_no, coi,
        static_cast<void *>(kitten), parent_a->sql_key, parent_b->sql_key);
    log_cat("parentA", *parent_a);
    log_cat("parentB", *parent_b);
    glaiel__CatData__breed_hook.orig(kitten, parent_a, parent_b, coi, furniture_effects);
    log_cat("kitten(after)", *kitten);
    D::info("  kitten coi after = {} (param was {})", kitten->coi, coi);
    snapshot_note_breed(call_no, *parent_a, *parent_b, coi, *kitten);
}
