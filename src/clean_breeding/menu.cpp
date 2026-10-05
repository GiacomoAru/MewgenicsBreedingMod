#include "amoeboid.hpp"
#include "config.hpp"
#include <algorithm>
#include <iterator>
#if CB_DEV_TOOLS
#include <format>

#include "breed.hpp"
#include "simulator.hpp"
#endif
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

// Names and texts: docs/DESIGN.md, section "Nomi e testi". Level values in config.ini never change (0 Normal,
// 1 Reduced, 2 Off, 3 Increased); the menu lists them from the mildest effect to the strongest.
static constexpr int LEVEL_ORDER[4] = {2, 1, 0, 3};
static const char *const AXIS_TOOLTIPS[2][4] = { // [axis][index into LEVEL_ORDER]
    {"Related parents count as unrelated.", "Inbreeding counts half.", "Game default.", "Inbreeding counts double."},
    {"Parents never pass on disorders or birth defects.", "Half the usual chance.", "Game default.", "Flaws get a second chance to pass on."},
};

// Tooltip placed to the right of the mouse cursor (the default position is under the game's own cursor).
static void tip(const char *text) {
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    ImGui::SetNextWindowPos(ImVec2(mouse.x + 48.0f, mouse.y + 6.0f));
    if(ImGui::BeginTooltip()) {
        ImGui::TextUnformatted(text);
        ImGui::EndTooltip();
    }
}

// returns true when the level changed
static bool level_combo(const char *label, const char *description, int axis, int &level) {
    int current = 2;
    for(int i = 0; i < 4; i++) {
        if(LEVEL_ORDER[i] == level) current = i;
    }
    bool changed = false;
    if(ImGui::BeginCombo(label, level_label(level))) {
        for(int i = 0; i < 4; i++) {
            if(ImGui::Selectable(level_label(LEVEL_ORDER[i]), i == current) && LEVEL_ORDER[i] != level) {
                level = LEVEL_ORDER[i];
                changed = true;
            }
            if(ImGui::IsItemHovered()) {
                tip(AXIS_TOOLTIPS[axis][i]);
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextDisabled("%s", description);
    ImGui::PopTextWrapPos();
    return changed;
}

static void draw_menu() {
    Config &c = config();
    // Wide window, centered each time the menu opens.
    const ImVec2 screen = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSize(ImVec2(std::min(std::max(screen.x * 0.62f, 700.0f), 980.0f), 0.0f), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2(screen.x * 0.5f, screen.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if(ImGui::Begin("Clean Breeding", &g_visible)) {
        if(G.mod_active) {
            ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.35f, 1.0f), "Active Â· Mewgenics %s", EXE_VERSION);
        } else {
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "Inactive: unsupported game version (needs %s)", EXE_VERSION);
            ImGui::PopTextWrapPos();
        }
        ImGui::Separator();

        if(G.mod_active) {
            ImGui::PushItemWidth(ImGui::GetFontSize() * 11.0f);
            bool changed = false;
            changed |= level_combo("Inbreeding penalties", "New disorders and birth defects caused by breeding related cats.", 0, c.inbreeding);
            ImGui::Spacing();
            changed |= level_combo("Inherited flaws", "Disorders and birth defects passed down from the parents.", 1, c.heredity);
            ImGui::PopItemWidth();

            ImGui::Spacing();
            struct Preset { const char *name; int inbreeding, heredity; const char *tooltip; };
            static const Preset presets[] = {
                {"Vanilla", 0, 0, "The game's own rules."},
                {"Gentle", 1, 1, "Half the penalties and flaws."},
                {"Carefree", 2, 0, "Breed relatives freely; parents still pass on their own flaws."},
                {"Clean", 2, 2, "No disorders or birth defects from breeding."},
                {"Hardcore", 3, 3, "Inbreeding hits harder and flaws spread more."},
            };
            for(size_t i = 0; i < std::size(presets); i++) {
                const Preset &p = presets[i];
                if(ImGui::Button(p.name)) {
                    c.inbreeding = p.inbreeding;
                    c.heredity = p.heredity;
                    changed = true;
                }
                if(ImGui::IsItemHovered()) {
                    tip(p.tooltip);
                }
                if(i + 1 < std::size(presets)) ImGui::SameLine();
            }
            if(changed) {
                config_save_breeding();
            }
            ImGui::Spacing();
            ImGui::TextDisabled("F8: show/hide Â· Settings are saved automatically");
        }

#if CB_DEV_TOOLS
        // Developer tools: only when config.ini says so ([debug] developer_tools=1), never from the menu.
        if(c.developer_tools) {
        ImGui::Separator();
        if(ImGui::CollapsingHeader("Developer tools")) {
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
                tip("Coefficient of inbreeding of the pair (0..1), same as its kitten's 'Inbred' value.\n"
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
#endif
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
        ImGui::GetStyle().FontScaleMain = 1.6f; // larger text: the game runs at high resolutions
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
