#include "simulator.hpp"
#include "snapshot.hpp"
#include "amoeboid.hpp"
#include "types/glaiel.hpp"
#include "types/glaiel_house.hpp"
#include "types/msvc.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/function_hook.hpp"
#include "utilities/json_writer.hpp"
#include "utilities/portal.hpp"
#include "utilities/strings.hpp"

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

// Snapshot of all cats in memory, written after every breed call.
//
// Timeline for one breed call (kitten has sql_key -1 when breed returns, the game assigns it later):
//   - next frame: snapshot "after_breed" (parents, house as it is);
//   - then, every POLL_FRAMES frames while an event is pending: when a cat key appears that was not
//     in the previous snapshot, snapshot "new_cats" listing the new keys; the event is then closed;
//   - give up after GIVE_UP_FRAMES frames (snapshot "timeout").
// The kitten as returned by breed is stored in the event, to compare with the cat that appears later.

namespace {

constexpr int POLL_FRAMES = 60;
constexpr int GIVE_UP_FRAMES = 60 * 120;

const char *const PART_NAMES[14] = {
    "body", "head", "tail", "leg1", "leg2", "arm1", "arm2",
    "leye", "reye", "lbrow", "rbrow", "lear", "rear", "mouth",
};

const BodyPartDescriptor *part_slot(const BodyParts &bp, int i) {
    const BodyPartDescriptor *slots[14] = {
        &bp.body, &bp.head, &bp.tail, &bp.leg1, &bp.leg2, &bp.arm1, &bp.arm2,
        &bp.lefteye, &bp.righteye, &bp.lefteyebrow, &bp.righteyebrow,
        &bp.leftear, &bp.rightear, &bp.mouth,
    };
    return slots[i];
}

// Plain copy of the fields of a cat we care about (the kitten returned by breed is temporary).
struct CatCopy {
    int64_t sql_key;
    double coi;
    int32_t parts[14];
    int32_t texture;
    std::string dis_name[2];
    int64_t dis_level[2];
};

CatCopy copy_cat(const CatData &c) {
    CatCopy r{};
    r.sql_key = c.sql_key;
    r.coi = c.coi;
    for(int i = 0; i < 14; i++) {
        r.parts[i] = static_cast<int32_t>(part_slot(c.body_parts, i)->part_sprite_idx);
    }
    r.texture = static_cast<int32_t>(c.body_parts.texture_sprite_idx);
    r.dis_name[0] = c.mutation_0.copy_to_native_string();
    r.dis_name[1] = c.mutation_1.copy_to_native_string();
    r.dis_level[0] = c.mutation_0_level;
    r.dis_level[1] = c.mutation_1_level;
    return r;
}

struct BreedEvent {
    int call_no;
    double coi_param;
    CatCopy parent_a, parent_b, kitten;
    int frames_waited = 0;
    bool post_done = false;
};

struct State {
    std::vector<BreedEvent> pending;
    std::set<int64_t> last_keys;
    bool have_last_keys = false;
    int seq = 0;
    int frame = 0;
} S;

MAKE_SDPORTAL(DATAOFF_glaiel__MewDirector__p_singleton,
    MewDirector *, get_p_mewdirector_singleton
)

std::string type_name_of(Component *c) {
    MsvcReleaseModeXString t = {};
    c->vtable->GetObjectTypeSTR(c, &t);
    std::string r(t.as_native_string_view());
    t.destroy();
    return r;
}

struct FoundCat {
    CatData *cat;
    HouseCat *house_cat;
};

HouseCat *find_house_cat(CatParts *parts) {
    if(parts->entity == nullptr) {
        return nullptr;
    }
    for(auto sib : parts->entity->components) {
        if(sib != nullptr && type_name_of(sib) == "HouseCat") {
            return static_cast<HouseCat *>(sib);
        }
    }
    return nullptr;
}

std::unordered_map<int64_t, FoundCat> collect_all_cats() {
    std::unordered_map<int64_t, FoundCat> cats;
    MewDirector *md = get_p_mewdirector_singleton();
    if(md == nullptr || md->director == nullptr) {
        return cats;
    }
    for(auto scene : md->director->scenes) {
        if(scene == nullptr || scene->ComponentLists == nullptr) {
            continue;
        }
        for(auto comp : *scene->ComponentLists) {
            if(type_name_of(comp) != "CatParts") {
                continue;
            }
            auto *parts = static_cast<CatParts *>(comp);
            if(parts->cat == nullptr) {
                continue;
            }
            auto [it, _] = cats.try_emplace(parts->cat->sql_key, FoundCat{parts->cat, nullptr});
            if(it->second.house_cat == nullptr) {
                it->second.house_cat = find_house_cat(parts);
            }
        }
    }
    return cats;
}

int64_t current_day() {
    MewDirector *md = get_p_mewdirector_singleton();
    if(md == nullptr) {
        return -1;
    }
    // MewDirector + 0x580 = save property "current_day" (verified by cat-bridge, 1.1.21239)
    return *reinterpret_cast<const int64_t *>(reinterpret_cast<const uint8_t *>(md) + 0x580);
}

void write_catcopy(JsonWriter &w, const char *key, const CatCopy &c) {
    w.key(key).begin_object();
    w.kv("sql_key", c.sql_key).kv("coi", c.coi);
    w.key("parts").begin_object();
    for(int i = 0; i < 14; i++) {
        w.kv(PART_NAMES[i], c.parts[i]);
    }
    w.kv("texture", c.texture).end_object();
    w.key("disorders").begin_array();
    for(int s = 0; s < 2; s++) {
        w.begin_object().kv("name", c.dis_name[s]).kv("level", c.dis_level[s]).end_object();
    }
    w.end_array().end_object();
}

void write_cat(JsonWriter &w, const FoundCat &f) {
    const CatData &c = *f.cat;
    w.begin_object();
    CatCopy cc = copy_cat(c);
    w.kv("sql_key", c.sql_key);
    HouseRoom *room = f.house_cat != nullptr ? f.house_cat->room : nullptr;
    w.kv("in_house", room != nullptr);
    w.kv("outside", f.house_cat != nullptr && room == nullptr);
    w.kv("room", room != nullptr ? room->name.as_native_string_view() : std::string_view());
    w.kv("name", convert_utf16_wstring_to_utf8_string(c.name.as_native_wstring_view()));
    w.kv("sex", c.sex).kv("level", c.level).kv("lifestage", c.lifestage).kv("birthday", c.birthday);
    w.kv("dead", c.campaign_stats.dead).kv("coi", c.coi).kv("fertility", c.fertility);
    w.key("parts").begin_object();
    for(int i = 0; i < 14; i++) {
        w.kv(PART_NAMES[i], cc.parts[i]);
    }
    w.kv("texture", cc.texture).end_object();
    w.key("disorders").begin_array();
    for(int s = 0; s < 2; s++) {
        w.begin_object().kv("name", cc.dis_name[s]).kv("level", cc.dis_level[s]).end_object();
    }
    w.end_array();
    w.key("passives").begin_array();
    w.begin_object().kv("name", c.passive_0.as_native_string_view()).kv("level", c.passive_0_level).end_object();
    w.begin_object().kv("name", c.passive_1.as_native_string_view()).kv("level", c.passive_1_level).end_object();
    w.end_array();
    w.end_object();
}

std::filesystem::path snapshot_dir() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(reinterpret_cast<HMODULE>(G.dll_base_va), buf, MAX_PATH);
    auto dir = std::filesystem::path(buf).parent_path() / "snapshots";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

void write_snapshot(const char *phase, const std::vector<BreedEvent> &events, const std::set<int64_t> &new_keys,
                    const std::unordered_map<int64_t, FoundCat> &cats) {
    JsonWriter w;
    w.begin_object();
    w.kv("phase", std::string_view(phase)).kv("seq", static_cast<int64_t>(S.seq)).kv("frame", static_cast<int64_t>(S.frame));
    w.kv("current_day", current_day());
    w.key("breed_events").begin_array();
    for(const auto &e : events) {
        w.begin_object().kv("call_no", static_cast<int64_t>(e.call_no)).kv("coi_param", e.coi_param);
        write_catcopy(w, "parent_a", e.parent_a);
        write_catcopy(w, "parent_b", e.parent_b);
        write_catcopy(w, "kitten_at_breed_return", e.kitten);
        w.end_object();
    }
    w.end_array();
    w.key("new_keys").begin_array();
    for(auto k : new_keys) {
        w.value(k);
    }
    w.end_array();
    w.key("cats").begin_array();
    for(const auto &[k, f] : cats) {
        write_cat(w, f);
    }
    w.end_array().end_object();

    std::string name = std::format("snap_{:03}_{}.json", S.seq, phase);
    std::ofstream out(snapshot_dir() / name, std::ios::binary);
    out << w.str();
    D::info("Snapshot written: snapshots\\{} ({} cats, {} events)", name, cats.size(), events.size());
    ++S.seq;
}

std::set<int64_t> keys_of(const std::unordered_map<int64_t, FoundCat> &cats) {
    std::set<int64_t> r;
    for(const auto &[k, _] : cats) {
        r.insert(k);
    }
    return r;
}

// Test helper (config.ini [debug] test_resources=N): every 5 s, raise house food and gold to at least N.
// The test_disorders list is applied once per launch.
// HouseInventory + 0xb0 = house_food, + 0xb4 = house_gold (int32, verified by cat-bridge, 1.1.21239).
// Only ever raises the values, never lowers them. Off when the key is missing or 0.
void apply_test_resources_once() {
    static bool done = false;
    static int polls_with_inventory = 0;
    // before the first setup: look every second; afterwards: top up resources every 5 s
    if(S.frame % (done ? 300 : 60) != 0) {
        return;
    }
    MewDirector *md = get_p_mewdirector_singleton();
    if(md == nullptr || md->director == nullptr) {
        return;
    }
    Component *inv = nullptr;
    for(auto scene : md->director->scenes) {
        if(scene == nullptr || scene->ComponentLists == nullptr) {
            continue;
        }
        for(auto comp : *scene->ComponentLists) {
            if(type_name_of(comp) == "HouseInventory") {
                inv = comp;
            }
        }
    }
    // The house scene already exists at the main menu (before the save is loaded: food 20, gold 0, no cats),
    // so also require cats in memory and a day counter, stable for ~5 s, or the save load overwrites us.
    auto cats = inv != nullptr ? collect_all_cats() : std::unordered_map<int64_t, FoundCat>{};
    if(inv == nullptr || cats.empty() || current_day() < 1) {
        polls_with_inventory = 0;
        return;
    }
    if(!done && ++polls_with_inventory < 5) {
        return;
    }
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(reinterpret_cast<HMODULE>(G.dll_base_va), buf, MAX_PATH);
    auto ini = (std::filesystem::path(buf).parent_path() / "config.ini").wstring();
    int target = static_cast<int>(GetPrivateProfileIntW(L"debug", L"test_resources", 0, ini.c_str()));
    if(target > 0) {
        auto *money = reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(inv) + 0xb0);
        if(money[0] < target || money[1] < target) {
            D::info("test_resources: food {} gold {} -> at least {}", money[0], money[1], target);
            money[0] = std::max<int32_t>(money[0], target);
            money[1] = std::max<int32_t>(money[1], target);
        }
    }
    if(done) {
        return;
    }
    done = true;

    // [debug] test_simulation=<keyA>:<keyB>:<coi>:<N>;...  queues simulator runs (results go to the log)
    wchar_t sims[1024];
    GetPrivateProfileStringW(L"debug", L"test_simulation", L"", sims, 1024, ini.c_str());
    std::string sim_spec = convert_utf16_wstring_to_utf8_string(sims);
    for(size_t p = 0; p < sim_spec.size();) {
        size_t end = sim_spec.find(';', p);
        std::string item = sim_spec.substr(p, end == std::string::npos ? std::string::npos : end - p);
        p = end == std::string::npos ? sim_spec.size() : end + 1;
        SimRequest r;
        long long a = 0, b = 0;
        int n = 0;
        if(sscanf_s(item.c_str(), " %lld:%lld:%lf:%d", &a, &b, &r.coi, &n) == 4) {
            r.parent_a = a;
            r.parent_b = b;
            r.n = n;
            simulator_enqueue(r);
        } else {
            D::warn("test_simulation: bad item '{}'", item);
        }
    }

    // [debug] test_disorders=<key>:<slot 0|1>:<DisorderName>:<level>,...  writes the disorder slots of live cats
    // (same technique as cat-bridge SET_PASSIVE: destroy() + construct(), game-heap string). Test slot only.
    wchar_t list[4096];
    GetPrivateProfileStringW(L"debug", L"test_disorders", L"", list, 4096, ini.c_str());
    std::string spec = convert_utf16_wstring_to_utf8_string(list);
    if(spec.empty()) {
        return;
    }
    size_t pos = 0;
    while(pos < spec.size()) {
        size_t end = spec.find(',', pos);
        std::string item = spec.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        pos = end == std::string::npos ? spec.size() : end + 1;
        long long key = 0, slot = 0, level = 1;
        char name[64] = {};
        if(sscanf_s(item.c_str(), " %lld:%lld:%63[^:]:%lld", &key, &slot, name, static_cast<unsigned>(sizeof(name)), &level) != 4
           || (slot != 0 && slot != 1) || level < 1) {
            D::warn("test_disorders: bad item '{}'", item);
            continue;
        }
        auto it = cats.find(key);
        if(it == cats.end()) {
            D::warn("test_disorders: cat {} not found", key);
            continue;
        }
        CatData *cat = it->second.cat;
        MsvcReleaseModeXString &s = slot == 0 ? cat->mutation_0 : cat->mutation_1;
        int64_t &lvl = slot == 0 ? cat->mutation_0_level : cat->mutation_1_level;
        D::info("test_disorders: cat {} slot {}: '{}' lvl {} -> '{}' lvl {}", key, slot, s.as_native_string_view(), lvl, name, level);
        s.destroy();
        s.construct(name, std::strlen(name));
        lvl = level;
    }
}

void on_update_frame() {
    ++S.frame;
    simulator_tick();
    apply_test_resources_once();
    if(S.pending.empty()) {
        return;
    }
    // first frame after the breed: snapshot of the situation, for the new events
    bool need_post = false;
    for(const auto &e : S.pending) {
        need_post |= !e.post_done;
    }
    if(need_post) {
        auto cats = collect_all_cats();
        std::vector<BreedEvent> fresh;
        for(auto &e : S.pending) {
            if(!e.post_done) {
                e.post_done = true;
                fresh.push_back(e);
            }
        }
        write_snapshot("after_breed", fresh, {}, cats);
        if(!S.have_last_keys) {
            S.last_keys = keys_of(cats);
            S.have_last_keys = true;
        }
        return;
    }
    for(auto &e : S.pending) {
        ++e.frames_waited;
    }
    if(S.frame % POLL_FRAMES != 0) {
        return;
    }
    auto cats = collect_all_cats();
    std::set<int64_t> now = keys_of(cats);
    std::set<int64_t> added;
    for(auto k : now) {
        if(!S.last_keys.contains(k)) {
            added.insert(k);
        }
    }
    if(!added.empty()) {
        write_snapshot("new_cats", S.pending, added, cats);
        S.last_keys = now;
        S.pending.clear();
        return;
    }
    if(S.pending.front().frames_waited > GIVE_UP_FRAMES) {
        write_snapshot("timeout", S.pending, {}, cats);
        S.pending.clear();
    }
}

} // namespace

CatData *find_cat(int64_t sql_key) {
    auto cats = collect_all_cats();
    auto it = cats.find(sql_key);
    return it == cats.end() ? nullptr : it->second.cat;
}

std::vector<CatData *> all_cats() {
    std::vector<CatData *> r;
    for(const auto &[k, f] : collect_all_cats()) {
        r.push_back(f.cat);
    }
    return r;
}

size_t cat_count() {
    return collect_all_cats().size();
}

void snapshot_note_breed(int call_no, const CatData &parent_a, const CatData &parent_b, double coi_param, const CatData &kitten) {
    BreedEvent e;
    e.call_no = call_no;
    e.coi_param = coi_param;
    e.parent_a = copy_cat(parent_a);
    e.parent_b = copy_cat(parent_b);
    e.kitten = copy_cat(kitten);
    S.pending.push_back(std::move(e));
}

MAKE_SHOOK(0, ADDRESS_glaiel__MewDirector__always_update,
    void, __cdecl, glaiel__MewDirector__always_update,
    MewDirector *thiss
) {
    on_update_frame();
    glaiel__MewDirector__always_update_hook.orig(thiss);
}
