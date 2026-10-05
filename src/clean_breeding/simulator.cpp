#include "simulator.hpp"
#include "amoeboid.hpp"
#include "breed.hpp"
#include "breed_logic.hpp"
#include "cat_factory.hpp"
#include "config.hpp"
#include "parts.hpp"
#include "snapshot.hpp"
#include "types/glaiel.hpp"
#include "types/msvc.hpp"
#include "types/rng.hpp"
#include "utilities/strings.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/function_hook.hpp"
#include "utilities/portal.hpp"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <filesystem>
#include <functional>
#include <fstream>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <string_view>

MAKE_SFPORTAL(ADDRESS_glaiel__CatData__breed,
    void, __cdecl, glaiel__CatData__breed_call,
    (CatData *p_kitten, CatData *p_parent_a, CatData *p_parent_b, double coi, void *vector_of_furniture_effects),
    (p_kitten, p_parent_a, p_parent_b, coi, vector_of_furniture_effects)
)

namespace {

// (group, id) of every part slot, texture last; -1 / "" never happens for valid cats.
struct PartRef { const char *group; int32_t id; };
void part_refs(const CatData &c, PartRef (&out)[PART_COUNT + 1]) {
    for(int i = 0; i < PART_COUNT; i++) {
        out[i] = {PART_GROUPS[i], static_cast<int32_t>(part_of(c.body_parts, i)->part_sprite_idx)};
    }
    out[PART_COUNT] = {"texture", static_cast<int32_t>(c.body_parts.texture_sprite_idx)};
}

bool is_none(const MsvcReleaseModeXString &s) {
    return s.as_native_string_view() == "None" || s._Mysize == 0;
}

struct Acc {
    int n = 0;
    int with_disorder = 0, dis_inherited = 0, dis_new = 0;     // kittens with >=1 disorder (any / inherited / new)
    int with_defect = 0, def_inherited = 0, def_new = 0;       // kittens with >=1 defective part
    int64_t disorder_total = 0, defect_slots_total = 0;
    int64_t negative_total = 0;
    int active_from_parent = 0, passive_from_parent = 0;
    int coi_wrong = 0; // kitten->coi different from the real pair coi
    double stat_sum[7] = {};
};

struct SimState {
    bool running = false;
    SimRequest req;
    int done = 0;
    Acc acc;
    Xoshiro256pContext rng{};
    CatData *a = nullptr, *b = nullptr;
    std::unique_ptr<uint8_t[]> a_before, b_before;
    size_t cats_before = 0;
    std::string report;
} S;

std::mutex g_mutex;

void add_kitten(Acc &acc, const CatData &k, const CatData &a, const CatData &b, double real_coi) {
    acc.n++;
    acc.coi_wrong += std::abs(k.coi - real_coi) > 1e-12;
    // disorders: inherited = same name as a disorder of a parent
    const MsvcReleaseModeXString *kd[2] = {&k.mutation_0, &k.mutation_1};
    const MsvcReleaseModeXString *pd[4] = {&a.mutation_0, &a.mutation_1, &b.mutation_0, &b.mutation_1};
    bool any_dis = false, any_inh = false, any_new = false;
    for(auto *d : kd) {
        if(is_none(*d)) continue;
        any_dis = true;
        acc.disorder_total++;
        acc.negative_total++;
        bool inh = false;
        for(auto *p : pd) inh |= !is_none(*p) && p->as_native_string_view() == d->as_native_string_view();
        (inh ? any_inh : any_new) = true;
    }
    acc.with_disorder += any_dis;
    acc.dis_inherited += any_inh;
    acc.dis_new += any_new;

    // defective parts: inherited = same id as a parent in the same slot (DESIGN.md "casi limite")
    PartRef kp[PART_COUNT + 1], ap[PART_COUNT + 1], bp[PART_COUNT + 1];
    part_refs(k, kp);
    part_refs(a, ap);
    part_refs(b, bp);
    bool any_def = false, def_inh = false, def_new = false;
    for(int i = 0; i <= PART_COUNT; i++) {
        if(!is_defect(kp[i].group, kp[i].id)) continue;
        any_def = true;
        acc.defect_slots_total++;
        acc.negative_total++;
        (kp[i].id == ap[i].id || kp[i].id == bp[i].id ? def_inh : def_new) = true;
    }
    acc.with_defect += any_def;
    acc.def_inherited += def_inh;
    acc.def_new += def_new;

    // ability check (the mod must not change these)
    const MsvcReleaseModeXString *parent_actives[16];
    int pa = 0;
    for(const CatData *p : {&a, &b}) {
        for(auto &s : p->actives_accessible) parent_actives[pa++] = &s;
        for(auto &s : p->actives_inherited) parent_actives[pa++] = &s;
    }
    if(!is_none(k.actives_inherited[0])) {
        for(int i = 0; i < pa; i++) {
            if(parent_actives[i]->as_native_string_view() == k.actives_inherited[0].as_native_string_view()) {
                acc.active_from_parent++;
                break;
            }
        }
    }
    if(!is_none(k.passive_0)) {
        for(const CatData *p : {&a, &b}) {
            if(p->passive_0.as_native_string_view() == k.passive_0.as_native_string_view()
               || p->passive_1.as_native_string_view() == k.passive_0.as_native_string_view()) {
                acc.passive_from_parent++;
                break;
            }
        }
    }
    const int32_t *st = &k.stats_heritable.str;
    for(int i = 0; i < 7; i++) acc.stat_sum[i] += st[i];
}

std::string make_report(const SimRequest &r, const Acc &x, bool parents_unchanged, bool count_unchanged) {
    auto pct = [&](int v) { return x.n ? 100.0 * v / x.n : 0.0; };
    auto avg = [&](double v) { return x.n ? v / x.n : 0.0; };
    const Config &c = config();
    std::string s = std::format("{} x {}  coi {:.4f}  N {}  (Inbreeding penalties: {}, Inherited flaws: {})\n", r.parent_a, r.parent_b, r.coi, x.n, level_label(c.inbreeding), level_label(c.heredity));
    s += std::format("Disorders:  any {:.1f}%  inherited {:.1f}%  new {:.1f}%\n", pct(x.with_disorder), pct(x.dis_inherited), pct(x.dis_new));
    s += std::format("Bad parts:  any {:.1f}%  inherited {:.1f}%  new {:.1f}%\n", pct(x.with_defect), pct(x.def_inherited), pct(x.def_new));
    s += std::format("Negative traits per kitten: {:.3f} ({:.3f} disorders, {:.3f} bad part slots)\n",
        avg(static_cast<double>(x.negative_total)), avg(static_cast<double>(x.disorder_total)), avg(static_cast<double>(x.defect_slots_total)));
    s += std::format("Ability check: active from a parent {:.1f}%  passive from a parent {:.1f}%\n", pct(x.active_from_parent), pct(x.passive_from_parent));
    s += std::format("Mean heritable stats: str {:.2f} dex {:.2f} con {:.2f} int {:.2f} spd {:.2f} cha {:.2f} lck {:.2f}\n",
        avg(x.stat_sum[0]), avg(x.stat_sum[1]), avg(x.stat_sum[2]), avg(x.stat_sum[3]), avg(x.stat_sum[4]), avg(x.stat_sum[5]), avg(x.stat_sum[6]));
    s += std::format("No traces: parents unchanged {}, cat count unchanged {}; kitten coi wrong: {}\n", parents_unchanged ? "yes" : "NO", count_unchanged ? "yes" : "NO", x.coi_wrong);
    return s;
}

std::unique_ptr<uint8_t[]> raw_copy(const CatData *c) {
    auto p = std::make_unique<uint8_t[]>(sizeof(CatData));
    std::memcpy(p.get(), c, sizeof(CatData));
    return p;
}

// The Mewjector log is overwritten at every launch: also keep every report in sim_reports.txt next to the DLL.
void append_report_file(const std::string &text) {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(reinterpret_cast<HMODULE>(G.dll_base_va), buf, MAX_PATH);
    std::ofstream out(std::filesystem::path(buf).parent_path() / "sim_reports.txt", std::ios::app);
    SYSTEMTIME t;
    GetLocalTime(&t);
    out << std::format("=== {:04}-{:02}-{:02} {:02}:{:02}:{:02} ===\n", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond) << text << "\n";
}

void finish() {
    bool pa = std::memcmp(S.a_before.get(), S.a, sizeof(CatData)) == 0;
    bool pb = std::memcmp(S.b_before.get(), S.b, sizeof(CatData)) == 0;
    bool cnt = cat_count() == S.cats_before;
    S.report = make_report(S.req, S.acc, pa && pb, cnt);
    S.running = false;
    D::info("Simulation done:\n{}", S.report);
    append_report_file(S.report);
}

// ---------------------------------------------------------------------------------------------
// Automatic test suite: fixed cases on synthetic parents (random strays of the game, cleaned of
// negative traits, then given exactly the traits of the case). Several fresh parent pairs per case
// for coverage of stats / bodies. Verdicts only where the expected rate is defined for the current
// settings (all inbreeding and heredity levels).
// ---------------------------------------------------------------------------------------------

using Expect = std::function<double(int inbreeding, int heredity)>;
struct Check { const char *metric; double tol; Expect expected; };
struct TraitSpec {
    std::vector<std::string> disorders;                   // names, slots filled in order
    std::vector<std::pair<std::string, int>> defects;     // (group, id), all slots of the group
};
struct Case {
    std::string name;
    double coi;
    TraitSpec a, b;
    std::vector<Check> checks;
};

// Expected rates (percent). New traits follow the game's formulas on the coi handed to breed (scaled_coi);
// inbreeding level 2 also removes the new disorders. Inherited rates depend on the heredity level
// (docs/DESIGN.md): Mild blocks each inherited trait with 50%, None blocks all, Hard gives a second chance
// (disorder 15% if the kitten has none of that parent's, defect 50% if the part is normal).
// P_UNIT = measured chance that a defective parent passes a defect of one slot unit (re_notes S5, about 49%).
constexpr double P_UNIT = 0.49;

Expect constant(double v) {
    return [v](int, int) { return v; };
}

Expect by_heredity(double h0, double h1, double h2, double h3) {
    return [=](int, int h) { return h == 0 ? h0 : (h == 1 ? h1 : (h == 2 ? h2 : h3)); };
}

Expect dis_new_rate(double coi) {
    return [coi](int inb, int) {
        return inb == 2 ? 0.0 : std::min(34.0, std::max(2.0, 40.0 * scaled_coi(coi, inb) - 6.0));
    };
}

Expect def_new_rate(double coi) {
    return [coi](int inb, int) { return std::min(100.0, 150.0 * scaled_coi(coi, inb)); };
}

// chance (percent) that a kitten inherits the defect of `units` independent slot units of one defective parent
Expect defect_inherited(int units) {
    auto any = [units](double p) { return 100.0 * (1.0 - std::pow(1.0 - p, units)); };
    return by_heredity(any(P_UNIT), any(P_UNIT * 0.5), 0.0, any(P_UNIT + (1.0 - P_UNIT) * 0.5));
}

const std::vector<Case> &suite_cases() {
    static const std::vector<Case> cases = {
        {"clean x clean", 0.0, {}, {}, {{"dis_new", 1.5, dis_new_rate(0.0)}, {"def_new", 1, def_new_rate(0.0)}}},
        {"clean x clean", 0.125, {}, {}, {{"dis_new", 3, dis_new_rate(0.125)}, {"def_new", 3, def_new_rate(0.125)}}},
        {"clean x clean", 0.25, {}, {}, {{"dis_new", 3, dis_new_rate(0.25)}, {"def_new", 3, def_new_rate(0.25)}}},
        {"clean x clean", 0.5, {}, {}, {{"dis_new", 3, dis_new_rate(0.5)}, {"def_new", 3, def_new_rate(0.5)}}},
        {"clean x clean", 1.0, {}, {}, {{"dis_new", 3, dis_new_rate(1.0)}, {"def_new", 3, def_new_rate(1.0)}}},
        {"A: Pox", 0.0, {{"Pox"}, {}}, {}, {{"dis_inh", 2, by_heredity(15, 7.5, 0, 27.75)}}},
        {"A: Pox, B: Flu", 0.0, {{"Pox"}, {}}, {{"Flu"}, {}}, {{"dis_inh", 3, by_heredity(27.75, 14.4, 0, 47.8)}}},
        {"A: Pox+Flu", 0.0, {{"Pox", "Flu"}, {}}, {}, {{"dis_inh", 2, by_heredity(15, 11.25, 0, 27.75)}}},
        {"A: EternalYouth", 0.0, {{"EternalYouth"}, {}}, {}, {{"dis_inh", 2, by_heredity(15, 7.5, 0, 27.75)}}},
        {"A: legs+arms 700 (defect, 2 units)", 0.0, {{}, {{"legs", 700}}}, {}, {{"def_inh", 3, defect_inherited(2)}}},
        {"A: eyes 701 (defect, 1 unit)", 0.0, {{}, {{"eyes", 701}}}, {}, {{"def_inh", 3, defect_inherited(1)}}},
        {"A: head 704 (Cyclops, defect, 1 unit)", 0.0, {{}, {{"head", 704}}}, {}, {{"def_inh", 3, defect_inherited(1)}}},
    };
    return cases;
}

void set_disorder(MsvcReleaseModeXString &s, int64_t &level, std::string_view name) {
    s.destroy();
    s.construct(name.data(), name.size());
    level = 1;
}

void set_defect(CatData &c, const std::string &group, int32_t id) {
    for(int i = 0; i < SLOT_COUNT; i++) {
        if(group == PART_GROUPS[i]) {
            set_part_id(c.body_parts, i, id);
        }
    }
}

// A random stray of the game, cleaned of every disorder and every defective part, then given the traits of `spec`.
TempCat make_synthetic_parent(const TraitSpec &spec) {
    TempCat c;
    for(int tries = 0; tries < 50; tries++) {
        c = make_random_stray(); // fresh stray per try, until it has no defective part
        PartRef refs[PART_COUNT + 1];
        part_refs(*c, refs);
        bool bad = false;
        for(const auto &r : refs) {
            bad |= is_defect(r.group, r.id);
        }
        if(!bad) break;
    }
    set_disorder(c->mutation_0, c->mutation_0_level, "None");
    set_disorder(c->mutation_1, c->mutation_1_level, "None");
    int slot = 0;
    for(const auto &d : spec.disorders) {
        if(slot == 0) set_disorder(c->mutation_0, c->mutation_0_level, d);
        else if(slot == 1) set_disorder(c->mutation_1, c->mutation_1_level, d);
        slot++;
    }
    for(const auto &[g, id] : spec.defects) {
        set_defect(*c, g, id);
    }
    return c;
}
struct SuiteState {
    bool running = false;
    int n_per_case = 5000;
    size_t case_idx = 0;
    int batch = 0;
    int done_in_batch = 0;
    Acc acc;
    TempCat a, b;
    Xoshiro256pContext rng{};
    std::string report;
    int overall_fail = 0, overall_pass = 0;
} U;

constexpr int SUITE_BATCHES = 5;

double metric(const Acc &x, std::string_view m) {
    auto pct = [&](int v) { return x.n ? 100.0 * v / x.n : 0.0; };
    if(m == "dis_any") return pct(x.with_disorder);
    if(m == "dis_inh") return pct(x.dis_inherited);
    if(m == "dis_new") return pct(x.dis_new);
    if(m == "def_any") return pct(x.with_defect);
    if(m == "def_inh") return pct(x.def_inherited);
    return pct(x.def_new);
}

// Ability check (the mod must not change it): baselines measured in S5 on the same synthetic parents.
constexpr double ACTIVE_BASELINE = 21.6, ACTIVE_TOL = 3.0, PASSIVE_MAX = 3.0;

void suite_finish_case(const Case &c) {
    const Config &cfg = config();
    bool judged = !c.checks.empty();
    const Acc &x = U.acc;
    double act = x.n ? 100.0 * x.active_from_parent / x.n : 0.0;
    double pas = x.n ? 100.0 * x.passive_from_parent / x.n : 0.0;
    bool base_ok = x.coi_wrong == 0 && std::abs(act - ACTIVE_BASELINE) <= ACTIVE_TOL && pas <= PASSIVE_MAX;
    bool traits_ok = true;
    std::string expect;
    for(const auto &k : c.checks) {
        double v = metric(x, k.metric);
        double e = k.expected(cfg.inbreeding, cfg.heredity);
        traits_ok &= std::abs(v - e) <= k.tol;
        expect += std::format(" {}={:.1f} (exp {:.1f}+-{})", k.metric, v, e, k.tol);
    }
    const char *verdict = !base_ok ? "FAIL" : (!judged ? "base" : (traits_ok ? "PASS" : "FAIL"));
    if(std::string_view(verdict) == "PASS") U.overall_pass++;
    if(std::string_view(verdict) == "FAIL") U.overall_fail++;
    // mean heritable stats (str dex con int spd cha lck): must not change across levels (compare the runs)
    std::string stats;
    for(double s : x.stat_sum) {
        stats += std::format("{}{:.2f}", stats.empty() ? "" : "/", x.n ? s / x.n : 0.0);
    }
    U.report += std::format("{:4} | {} coi {} | dis any {:.1f} inh {:.1f} new {:.1f} | def any {:.1f} inh {:.1f} new {:.1f} | act {:.1f} pas {:.1f} coi_wrong {} | stats {} |{}\n",
        verdict, c.name, c.coi,
        metric(x, "dis_any"), metric(x, "dis_inh"), metric(x, "dis_new"),
        metric(x, "def_any"), metric(x, "def_inh"), metric(x, "def_new"),
        act, pas, x.coi_wrong, stats, expect);
}
// one chunk of the suite; the caller has already swapped in the simulator RNG
void suite_chunk() {
    constexpr int CHUNK = 250;
    const auto &cases = suite_cases();
    const Case &c = cases[U.case_idx];
    int per_batch = std::max(1, U.n_per_case / SUITE_BATCHES);
    if(!U.a) {
        U.a = make_synthetic_parent(c.a);
        U.b = make_synthetic_parent(c.b);
        U.done_in_batch = 0;
    }
    int todo = std::min(CHUNK, per_batch - U.done_in_batch);
    for(int i = 0; i < todo; i++) {
        TempCat kitten = new_default_cat();
        glaiel__CatData__breed_call(kitten.get(), U.a.get(), U.b.get(), c.coi, nullptr);
        add_kitten(U.acc, *kitten, *U.a, *U.b, c.coi);
    }
    U.done_in_batch += todo;
    if(U.done_in_batch >= per_batch) {
        U.a.reset();
        U.b.reset();
        if(++U.batch >= SUITE_BATCHES) {
            suite_finish_case(c);
            U.acc = Acc{};
            U.batch = 0;
            if(++U.case_idx >= cases.size()) {
                const Config &cfg = config();
                U.report = std::format("Test suite: Inbreeding penalties {}, Inherited flaws {}: {} kittens per case ({} parent pairs each): {} pass, {} fail (base = only abilities and kitten coi judged)\n",
                    level_label(cfg.inbreeding), level_label(cfg.heredity), U.n_per_case, SUITE_BATCHES, U.overall_pass, U.overall_fail) + U.report;
                U.running = false;
                D::info("Test suite done:\n{}", U.report);
                append_report_file(U.report);
            }
        }
    }
}

} // namespace

static std::deque<SimRequest> g_queue;

static void start_locked(const SimRequest &req) {
    if(S.running || U.running) {
        return;
    }
    U.report.clear();
    S = SimState{};
    S.req = req;
    S.req.n = std::clamp(req.n, 1, 20000);
    S.a = find_cat(req.parent_a);
    S.b = find_cat(req.parent_b);
    if(S.a == nullptr || S.b == nullptr) {
        S.report = std::format("Parent not found ({} x {}): both sql_keys must be cats currently in memory.", req.parent_a, req.parent_b);
        D::warn("Simulation: {}", S.report);
        return;
    }
    S.a_before = raw_copy(S.a);
    S.b_before = raw_copy(S.b);
    S.cats_before = cat_count();
    // our own random start state for the kitten stream: the game's RNG is saved and restored around every chunk
    std::mt19937_64 seed{std::random_device{}()};
    for(auto &w : S.rng.ctx) {
        w = seed();
    }
    S.running = true;
}

void simulator_start(const SimRequest &req) {
    std::lock_guard lock(g_mutex);
    start_locked(req);
}

void simulator_enqueue(const SimRequest &req) {
    std::lock_guard lock(g_mutex);
    g_queue.push_back(req);
}

SimStatus simulator_status() {
    std::lock_guard lock(g_mutex);
    if(U.running) {
        int per_case = std::max(1, U.n_per_case / SUITE_BATCHES) * SUITE_BATCHES;
        int total = static_cast<int>(suite_cases().size()) * per_case;
        int done = static_cast<int>(U.case_idx) * per_case + U.batch * (per_case / SUITE_BATCHES) + U.done_in_batch;
        return {true, done, total, U.report};
    }
    if(!U.report.empty() && S.report.empty()) {
        return {S.running, S.done, S.req.n, U.report};
    }
    return {S.running, S.done, S.req.n, S.report};
}

void simulator_run_suite(int n_per_case) {
    std::lock_guard lock(g_mutex);
    if(S.running || U.running) {
        return;
    }
    U = SuiteState{};
    U.n_per_case = std::clamp(n_per_case, SUITE_BATCHES, 100000);
    S = SimState{}; // clear the single-run report: the status shows the suite report
    std::mt19937_64 seed{std::random_device{}()};
    for(auto &w : U.rng.ctx) {
        w = seed();
    }
    U.running = true;
}

static bool g_want_cats = false;
static int g_cats_age = 1 << 30;
static std::vector<CatInfo> g_cats;

void simulator_want_cats() {
    std::lock_guard lock(g_mutex);
    g_want_cats = true;
}

std::vector<CatInfo> simulator_cats() {
    std::lock_guard lock(g_mutex);
    return g_cats;
}

static void refresh_cats_locked() {
    std::vector<CatInfo> list;
    for(const CatData *c : all_cats()) {
        std::string dis;
        for(const auto *d : {&c->mutation_0, &c->mutation_1}) {
            if(!is_none(*d)) {
                dis += (dis.empty() ? "" : "+") + std::string(d->as_native_string_view());
            }
        }
        PartRef refs[PART_COUNT + 1];
        part_refs(*c, refs);
        int bad = 0;
        for(const auto &r : refs) {
            bad += is_defect(r.group, r.id);
        }
        list.push_back({c->sql_key, std::format("{} (#{}) {} | {} | {} bad parts",
            convert_utf16_wstring_to_utf8_string(c->name.as_native_wstring_view()), c->sql_key,
            c->sex == 0 ? "M" : "F", dis.empty() ? "no disorders" : dis, bad)});
    }
    std::sort(list.begin(), list.end(), [](const CatInfo &x, const CatInfo &y) { return x.label < y.label; });
    g_cats = std::move(list);
}

void simulator_tick() {
    std::lock_guard lock(g_mutex);
    // refresh the selector list about once a second, and only while the menu shows it
    if(g_want_cats && ++g_cats_age >= 60) {
        g_cats_age = 0;
        refresh_cats_locked();
    }
    g_want_cats = false;
    if(U.running) {
        Xoshiro256pContext &rng = game_rng();
        Xoshiro256pContext backup = rng;
        rng = U.rng;
        g_suppress_name_history = true;
        g_breed_sim_active = true;
        suite_chunk();
        g_breed_sim_active = false;
        g_suppress_name_history = false;
        U.rng = rng;
        rng = backup;
        return;
    }
    if(!S.running && !g_queue.empty()) {
        SimRequest next = g_queue.front();
        g_queue.pop_front();
        start_locked(next);
    }
    if(!S.running) {
        return;
    }
    constexpr int CHUNK = 200;
    Xoshiro256pContext &rng = game_rng();
    Xoshiro256pContext backup = rng;
    rng = S.rng;
    g_suppress_name_history = true;
    g_breed_sim_active = true;
    int todo = std::min(CHUNK, S.req.n - S.done);
    for(int i = 0; i < todo; i++) {
        TempCat kitten = new_default_cat();
        glaiel__CatData__breed_call(kitten.get(), S.a, S.b, S.req.coi, nullptr);
        add_kitten(S.acc, *kitten, *S.a, *S.b, S.req.coi);
    }
    g_breed_sim_active = false;
    g_suppress_name_history = false;
    S.rng = rng;
    rng = backup;
    S.done += todo;
    if(S.done >= S.req.n) {
        finish();
    }
}
