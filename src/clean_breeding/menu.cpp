#include "amoeboid.hpp"
#include <format>

#include "breed.hpp"
#include "config.hpp"
#include "simulator.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/function_hook.hpp"

#include "SDL3/SDL.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

// ImGui overlay (F8), hooked into SDL_GL_SwapWindow / SDL_PollEvent.
// Approach adapted from Amoeba (amoeba_imgui.cpp), minus viewports, demo and Lua.

namespace {
    bool g_initialized = false;
    bool g_visible = false;
}

// Level numbers are fixed by DESIGN.md (0 Vanilla, 1 Mild, 2 None, 3 Hard); the menu lists them by difficulty.
static const int LEVEL_ORDER[4] = {3, 0, 1, 2};
static const char *const LEVEL_NAMES[4] = {"Hard", "Vanilla", "Mild", "None"};

static bool level_combo(const char *label, int &level) {
    int idx = 0;
    for(int i = 0; i < 4; i++) {
        if(LEVEL_ORDER[i] == level) idx = i;
    }
    bool changed = false;
    if(ImGui::BeginCombo(label, LEVEL_NAMES[idx])) {
        for(int i = 0; i < 4; i++) {
            if(ImGui::Selectable(LEVEL_NAMES[i], i == idx) && LEVEL_ORDER[i] != level) {
                level = LEVEL_ORDER[i];
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

static void draw_menu() {
    Config &c = config();
    ImGui::SetNextWindowSize(ImVec2(380, 0), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Clean Breeding", &g_visible)) {
        ImGui::Text("Version %s | game %s (exe hash %s) | signatures OK | hooks active",
            MOD_VERSION, EXE_VERSION, G.exe_hash_mismatch_detected ? "MISMATCH" : "OK");
        ImGui::Separator();

        bool changed = false;
        changed |= level_combo("Inbreeding", c.inbreeding);
        changed |= level_combo("Heredity", c.heredity);

        struct Preset { const char *name; int inbreeding, heredity; };
        static const Preset presets[] = {
            {"Vanilla", 0, 0}, {"Assisted", 1, 1}, {"Free breeding", 2, 0}, {"Perfect genetics", 2, 2}, {"Hard mode", 3, 3},
        };
        for(const auto &p : presets) {
            if(ImGui::Button(p.name)) {
                c.inbreeding = p.inbreeding;
                c.heredity = p.heredity;
                changed = true;
            }
            ImGui::SameLine();
        }
        ImGui::NewLine();
        if(changed) {
            config_save_breeding();
        }

        ImGui::Separator();
        ImGui::Text("Cleanse");
        int mode = static_cast<int>(c.cleanse_mode);
        bool mode_changed = false;
        mode_changed |= ImGui::RadioButton("Disorders", &mode, static_cast<int>(CleanseMode::Disorders));
        ImGui::SameLine();
        mode_changed |= ImGui::RadioButton("Defects", &mode, static_cast<int>(CleanseMode::Defects));
        ImGui::SameLine();
        mode_changed |= ImGui::RadioButton("All", &mode, static_cast<int>(CleanseMode::All));
        if(mode_changed) {
            c.cleanse_mode = static_cast<CleanseMode>(mode);
            config_save_cleanse_mode();
        }
        ImGui::BeginDisabled();
        ImGui::Button("Cleanse all cats");
        ImGui::EndDisabled();
        ImGui::TextDisabled("(not available yet)");

        ImGui::Separator();
        if(ImGui::CollapsingHeader("Debug: breeding simulator")) {
            static int64_t parent_a = 0, parent_b = 0;
            static double coi = 0;
            static int n = 1000;
            simulator_want_cats();
            auto cats = simulator_cats();
            auto cat_combo = [&](const char *label, int64_t &key) {
                std::string preview = std::format("#{}", key);
                for(const auto &ci : cats) {
                    if(ci.key == key) preview = ci.label;
                }
                if(ImGui::BeginCombo(label, preview.c_str())) {
                    for(const auto &ci : cats) {
                        if(ImGui::Selectable(ci.label.c_str(), ci.key == key)) key = ci.key;
                    }
                    ImGui::EndCombo();
                }
            };
            cat_combo("Parent A", parent_a);
            cat_combo("Parent B", parent_b);
            ImGui::InputDouble("coi (pair kinship)", &coi, 0.0, 0.0, "%.4f");
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if(ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Coefficient of inbreeding of the pair (0..1), same as its kitten's 'Inbred' value.\n"
                                  "0 unrelated, 0.0625 cousins, 0.125 uncle/niece or half siblings,\n"
                                  "0.25 siblings or parent/child, 0.5 same cat.");
            }
            for(double v : {0.0, 0.0625, 0.125, 0.25, 0.5}) {
                if(ImGui::SmallButton(std::format("{}##coi{}", v, v).c_str())) coi = v;
                ImGui::SameLine();
            }
            ImGui::NewLine();
            ImGui::InputInt("N", &n);
            if(ImGui::Button("Fill from last breeding") && g_last_breed.valid) {
                parent_a = g_last_breed.parent_a;
                parent_b = g_last_breed.parent_b;
                coi = g_last_breed.coi;
            }
            if(!g_last_breed.valid) {
                ImGui::SameLine();
                ImGui::TextDisabled("(no breeding seen yet)");
            }
            SimStatus st = simulator_status();
            ImGui::BeginDisabled(st.running);
            if(ImGui::Button("Simulate")) {
                simulator_start({parent_a, parent_b, coi, n});
            }
            ImGui::SameLine();
            if(ImGui::Button("Run all tests (N per case)")) {
                simulator_run_suite(n);
            }
            ImGui::EndDisabled();
            if(st.running) {
                ImGui::ProgressBar(st.total ? static_cast<float>(st.done) / st.total : 0.f);
            }
            if(!st.report.empty()) {
                ImGui::BeginChild("sim_report", ImVec2(0, 280), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::TextUnformatted(st.report.c_str());
                ImGui::EndChild();
            }
        }
    }
    ImGui::End();
}
MAKE_PHOOK(1, "SDL_GL_SwapWindow",
    bool, __cdecl, SDL_GL_SwapWindow,
    SDL_Window *window
) {
    if(!g_initialized) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        ImGui_ImplSDL3_InitForOpenGL(window, SDL_GL_GetCurrentContext());
        ImGui_ImplOpenGL3_Init();
        g_initialized = true;
    }

    if(g_visible) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        draw_menu();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return SDL_GL_SwapWindow_hook.orig(window);
}

MAKE_PHOOK(1, "SDL_PollEvent",
    bool, __cdecl, SDL_PollEvent,
    SDL_Event *event
) {
    if(!g_initialized) {
        return SDL_PollEvent_hook.orig(event);
    }

    // Let imgui see each event first; swallow it if imgui wants it and the menu is open.
    ImGuiIO &io = ImGui::GetIO();
    while(SDL_PollEvent_hook.orig(event)) {
        if(event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_F8 && !event->key.repeat) {
            g_visible = !g_visible;
            continue; // F8 is ours, never reaches the game
        }
        if(!g_visible) {
            return true;
        }
        ImGui_ImplSDL3_ProcessEvent(event);
        switch(event->type) {
            case SDL_EVENT_MOUSE_MOTION:
            case SDL_EVENT_MOUSE_WHEEL:
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if(io.WantCaptureMouse) continue;
                return true;
            case SDL_EVENT_TEXT_INPUT:
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                if(io.WantCaptureKeyboard) continue;
                return true;
            default:
                return true;
        }
    }
    return false;
}
