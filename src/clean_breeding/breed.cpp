#include "amoeboid.hpp"
#include "breed.hpp"
#include "breed_logic.hpp"
#include "cat_factory.hpp"
#include "config.hpp"
#include "parts.hpp"
#if CB_DEV_TOOLS
#include "snapshot.hpp"
#endif
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
#include <random>
#include <string>
#include <vector>

// Hook on glaiel::CatData::breed: the breeding rules of the mod (docs/DESIGN.md).

#if CB_DEV_TOOLS
// Development tools: per-call log, last_breed.txt for the simulator, snapshots.
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
#endif // CB_DEV_TOOLS

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

// ---- heredity axis ----------------------------------------------------------------------------

// The mod's own RNG: never the game's, so the game's random sequence is not disturbed.
static std::mt19937_64 &mod_rng() {
    static std::mt19937_64 rng{std::random_device{}()};
    return rng;
}

static double rnd() {
    return std::uniform_real_distribution<double>(0.0, 1.0)(mod_rng());
}

// Levels 1 and 2: hide parents' disorder slots from breed by overwriting them with "None" (level 1), the game's own
// empty-slot representation. The raw bytes of the string are saved and restored (never freed), also on exceptions.
class HiddenDisorders {
public:
    void hide(MsvcReleaseModeXString &name, int64_t &level) {
        Saved s;
        s.name = &name;
        s.level = &level;
        std::memcpy(s.bytes, static_cast<const void *>(&name), sizeof(s.bytes));
        s.old_level = level;
        saved_.push_back(s);
        name.construct("None", 4); // 4 bytes: inline buffer, no allocation, the old heap buffer stays owned by the saved bytes
        level = 1;
    }
    ~HiddenDisorders() {
        for(auto it = saved_.rbegin(); it != saved_.rend(); ++it) {
            std::memcpy(static_cast<void *>(it->name), it->bytes, sizeof(it->bytes));
            *it->level = it->old_level;
        }
    }

private:
    struct Saved {
        MsvcReleaseModeXString *name;
        int64_t *level;
        uint8_t bytes[sizeof(MsvcReleaseModeXString)];
        int64_t old_level;
    };
    std::vector<Saved> saved_;
};

static void hide_parent_disorders(HiddenDisorders &hidden, CatData &parent, int heredity) {
    MsvcReleaseModeXString *names[2] = {&parent.mutation_0, &parent.mutation_1};
    int64_t *levels[2] = {&parent.mutation_0_level, &parent.mutation_1_level};
    for(int i = 0; i < 2; i++) {
        std::string_view n = names[i]->as_native_string_view();
        if(n == "None" || n.empty() || config().whitelist_disorders.contains(std::string(n))) {
            continue;
        }
        if(should_block(heredity, rnd())) {
            hidden.hide(*names[i], *levels[i]);
        }
    }
}

// A normal part for every slot of `unit`, generated by the game on a temporary cat. The game's RNG is saved,
// reseeded from the mod's RNG and restored, so neither the game's sequence nor the simulator's is touched.
static bool generate_normal_unit(const PartUnit &unit, int32_t (&out)[2]) {
    Xoshiro256pContext &rng = game_rng();
    Xoshiro256pContext saved = rng;
    for(auto &w : rng.ctx) {
        w = mod_rng()();
    }
    bool ok = false;
    for(int tries = 0; tries < 30 && !ok; tries++) {
        TempCat temp = new_default_cat(); // fresh cat per try: generate_bodyparts is only ever called once per cat
        generate_bodyparts(&temp->body_parts);
        out[0] = part_id(temp->body_parts, unit.slots[0]);
        out[1] = part_id(temp->body_parts, unit.slots[unit.count - 1]);
        ok = !is_defect(unit.group, out[0]) && !is_defect(unit.group, out[1]);
    }
    rng = saved;
    return ok;
}

static void set_unit(CatData &cat, const PartUnit &unit, const int32_t (&ids)[2]) {
    for(int k = 0; k < unit.count; k++) {
        set_part_id(cat.body_parts, unit.slots[k], ids[k]);
    }
}

static void get_unit(const CatData &cat, const PartUnit &unit, int32_t (&ids)[2]) {
    for(int k = 0; k < 2; k++) {
        ids[k] = part_id(cat.body_parts, unit.slots[k < unit.count ? k : 0]);
    }
}

// Levels 1 and 2: a defective part the kitten inherited (same id as a parent's in that unit) is replaced, always at
// level 2 and with 50% at level 1: by the other parent's part if normal, otherwise by a game-generated normal part.
// The "symmetric side" of docs/DESIGN.md does not exist: both sides of a pair always carry the same id (re_notes S3).
static void block_inherited_defects(CatData &kitten, const CatData &a, const CatData &b, int heredity) {
    for(const PartUnit &u : PART_UNITS) {
        int32_t k[2], pa[2], pb[2];
        get_unit(kitten, u, k);
        if(!is_defect(u.group, k[0])) continue;
        get_unit(a, u, pa);
        get_unit(b, u, pb);
        bool from_a = k[0] == pa[0], from_b = k[0] == pb[0];
        if(!from_a && !from_b) continue; // a new defect from inbreeding, not inherited
        if(!should_block(heredity, rnd())) continue;
        const int32_t(&other)[2] = from_a ? pb : pa;
        if(!(from_a && from_b) && !is_defect(u.group, other[0]) && !is_defect(u.group, other[1])) {
            set_unit(kitten, u, other);
        } else {
            int32_t gen[2];
            if(generate_normal_unit(u, gen)) {
                set_unit(kitten, u, gen);
            }
        }
    }
}

static void set_disorder_slot(MsvcReleaseModeXString &name, int64_t &level, const std::string &value, int64_t value_level) {
    name.destroy();
    name.construct(value.data(), value.size());
    level = value_level;
}

// Level 3 (Hard): second chance for the parents' negative traits (docs/DESIGN.md). Never whitelisted disorders.
static void hard_second_chance(CatData &kitten, const CatData &a, const CatData &b) {
    MsvcReleaseModeXString *kn[2] = {&kitten.mutation_0, &kitten.mutation_1};
    int64_t *kl[2] = {&kitten.mutation_0_level, &kitten.mutation_1_level};
    for(const CatData *p : {&a, &b}) {
        std::vector<std::string> pnames = {p->mutation_0.copy_to_native_string(), p->mutation_1.copy_to_native_string()};
        std::vector<std::string> knames = {kn[0]->copy_to_native_string(), kn[1]->copy_to_native_string()};
        auto cand = second_chance_candidates(pnames, knames, config().whitelist_disorders);
        int free_slot = knames[0] == "None" || knames[0].empty() ? 0 : (knames[1] == "None" || knames[1].empty() ? 1 : -1);
        if(cand.empty() || free_slot < 0 || rnd() >= HARD_DISORDER_CHANCE) continue;
        const std::string &pick = cand[static_cast<size_t>(rnd() * cand.size()) % cand.size()];
        int64_t lvl = pnames[0] == pick ? p->mutation_0_level : p->mutation_1_level;
        set_disorder_slot(*kn[free_slot], *kl[free_slot], pick, lvl);
    }
    for(const PartUnit &u : PART_UNITS) {
        int32_t k[2], pa[2], pb[2];
        get_unit(kitten, u, k);
        if(is_defect(u.group, k[0])) continue; // only a normal part gets a defect
        get_unit(a, u, pa);
        get_unit(b, u, pb);
        const int32_t *cands[2];
        int n = 0;
        if(is_defect(u.group, pa[0])) cands[n++] = pa;
        if(is_defect(u.group, pb[0])) cands[n++] = pb;
        if(n == 0 || rnd() >= HARD_DEFECT_CHANCE) continue;
        const int32_t *pick = cands[n == 1 ? 0 : (rnd() < 0.5 ? 0 : 1)];
        int32_t ids[2] = {pick[0], pick[1]};
        set_unit(kitten, u, ids);
    }
}

MAKE_SHOOK(0, ADDRESS_glaiel__CatData__breed,
    void, __cdecl, glaiel__CatData__breed,
    CatData *kitten, CatData *parent_a, CatData *parent_b, double coi, void *furniture_effects
) {
    const Config &cfg = config();
    const double real_coi = coi;
    const double passed_coi = scaled_coi(real_coi, cfg.inbreeding);
#if CB_DEV_TOOLS
    const bool logging = !g_breed_sim_active;
    int call_no = 0;
    if(logging) {
        static int counter = 0;
        call_no = ++counter;
        g_last_breed = {true, parent_a->sql_key, parent_b->sql_key, real_coi};
        std::ofstream(last_breed_path()) << parent_a->sql_key << ' ' << parent_b->sql_key << ' ' << std::format("{:.17g}", real_coi) << '\n';
        D::info("[breed #{}] coi_param={} passed_to_game={} (inbreeding {} heredity {}) kitten_ptr={} A={} B={}", call_no, real_coi, passed_coi,
            cfg.inbreeding, cfg.heredity, static_cast<void *>(kitten), parent_a->sql_key, parent_b->sql_key);
        log_cat("parentA", *parent_a);
        log_cat("parentB", *parent_b);
    }
#else
    D::info("[breed] A={} B={} coi={} (inbreeding {} heredity {})", parent_a->sql_key, parent_b->sql_key, real_coi, cfg.inbreeding, cfg.heredity);
#endif

    {
        HiddenDisorders hidden; // restored when this scope ends, also if breed throws
        if(cfg.heredity == 1 || cfg.heredity == 2) {
            hide_parent_disorders(hidden, *parent_a, cfg.heredity);
            hide_parent_disorders(hidden, *parent_b, cfg.heredity);
        }
        glaiel__CatData__breed_hook.orig(kitten, parent_a, parent_b, passed_coi, furniture_effects);
    }

    kitten->coi = real_coi; // the kitten stays "Inbred" and the pedigree gets the true value
    if(cfg.inbreeding == 2) {
        remove_new_kitten_disorders(*kitten, *parent_a, *parent_b);
    }
    if(cfg.heredity == 1 || cfg.heredity == 2) {
        block_inherited_defects(*kitten, *parent_a, *parent_b, cfg.heredity);
    } else if(cfg.heredity == 3) {
        hard_second_chance(*kitten, *parent_a, *parent_b);
    }

#if CB_DEV_TOOLS
    if(logging) {
        log_cat("kitten(after)", *kitten);
        D::info("  kitten coi after = {} (param was {})", kitten->coi, real_coi);
        snapshot_note_breed(call_no, *parent_a, *parent_b, real_coi, *kitten);
    }
#endif
}