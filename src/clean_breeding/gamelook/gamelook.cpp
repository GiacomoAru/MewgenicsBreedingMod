// The menu's game look. paper(), the colours and the tooltip come from Combat Roster Panel's panel.cpp / overlay.cpp
// (MIT, Copyright (c) 2026 Combat Roster Panel authors, see ATTRIBUTION.md).
#include "gamelook.h"

#include "assets.h"
#include "fontloader.h"
#include "utilities/debug_console.hpp"

#include "imgui_internal.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cfloat>
#include <cmath>
#include <string>

namespace gamelook {
namespace {

// palette sampled from the game's HUD
const ImU32 kInk = IM_COL32(24, 20, 22, 255);
const ImU32 kInkSoft = IM_COL32(96, 80, 66, 255);
const float kBaseFontPx = 20.0f;   // before the resolution scale

struct State {
    bool enabled = false;
    bool started = false;
    bool fonts_added = false;     // fonts requested from the loader (done once it is ready)
    bool fallback_logged = false;
    ImFont* body = nullptr;
    ImFont* title = nullptr;
    float scale = 1.0f;
    float applied_scale = -1.0f;
    ImGuiStyle base_style;
    bool base_style_ok = false;
} g;

std::string exe_dir() {
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string s(path, n);
    size_t slash = s.find_last_of("\\/");
    return slash == std::string::npos ? std::string() : s.substr(0, slash);
}

}  // namespace

void start(bool enabled) {
    if (g.started) return;
    g.started = true;
    g.enabled = enabled;
    if (!enabled) {
        D::info("Game look disabled by config, using the default style");
        return;
    }
    cr::assets_start(exe_dir());
}

void gl_context_lost() { cr::assets_gl_lost(); }

void before_frame() {
    if (!g.enabled) return;
    cr::AssetState st = cr::assets_state();
    if (st == cr::AssetState::Failed && !g.fallback_logged) {
        g.fallback_logged = true;
        D::info("Menu: using the default style ({})", cr::assets_failure());
    }
    if (st != cr::AssetState::Ready) return;
    cr::assets_upload_pending();
    if (!g.fonts_added) {
        // The game's own fonts, added between frames (ImGui 1.92 fonts are dynamic).
        g.fonts_added = true;
        g.body = cr::add_swf_font(cr::font_body(), kBaseFontPx, cr::font_cjk());
        g.title = cr::add_swf_font(cr::font_title(), kBaseFontPx, cr::font_cjk());
        if (g.body) ImGui::GetIO().FontDefault = g.body;
        D::info("Menu: game fonts {}", g.body ? "in use" : "unavailable, using the default font");
    }
}

bool active() { return g.enabled && g.body != nullptr && cr::asset_paper().id != 0; }

float scale() { return g.scale; }

void apply_scale(float display_h) {
    ImGuiStyle& st = ImGui::GetStyle();
    if (!g.base_style_ok) {
        g.base_style = st;
        g.base_style_ok = true;
    }
    // With the game's fonts the size follows the resolution like the game's own UI; with the default font the
    // menu keeps the fixed enlargement it always had.
    float s = 1.6f;
    if (g.body) {
        s = display_h / 1080.0f;
        if (s < 0.75f) s = 0.75f;
        if (s > 2.5f) s = 2.5f;
    }
    g.scale = s;
    if (s == g.applied_scale) return;
    g.applied_scale = s;
    st = g.base_style;
    st.ScaleAllSizes(g.body ? s : 1.0f);
    st.FontScaleMain = s;
}

ImFont* title_font() { return g.title ? g.title : ImGui::GetFont(); }
float title_px() { return kBaseFontPx * 1.5f; }

void push_style() {
    const float s = g.scale;
    ImGui::PushStyleColor(ImGuiCol_Text, kInk);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, kInkSoft);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);                 // the paper is drawn instead
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.91f, 0.88f, 0.82f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.86f, 0.80f, 0.66f, 0.55f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.90f, 0.82f, 0.62f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.85f, 0.74f, 0.50f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.72f, 0.56f, 0.55f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.88f, 0.76f, 0.50f, 0.80f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.78f, 0.62f, 0.38f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.86f, 0.76f, 0.52f, 0.60f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.88f, 0.76f, 0.50f, 0.80f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.78f, 0.62f, 0.38f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Separator, IM_COL32(96, 80, 66, 150));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30.0f * s, 26.0f * s));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * s);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f * s);
}

void pop_style() {
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(15);
}

// The game's paper as a 9-slice: the inked edge keeps its thickness, the watercolour middle stretches. Falls back
// to a flat card until loaded.
void paper(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 tint) {
    const float s = g.scale;
    dl->AddRectFilled(ImVec2(a.x + 4 * s, a.y + 6 * s), ImVec2(b.x + 4 * s, b.y + 6 * s), IM_COL32(0, 0, 0, 80), 6 * s);
    cr::Tex t = cr::asset_paper();
    if (!t.id) {
        dl->AddRectFilled(a, b, kInk, 6 * s);
        dl->AddRectFilled(ImVec2(a.x + 3 * s, a.y + 3 * s), ImVec2(b.x - 3 * s, b.y - 3 * s), IM_COL32(226, 220, 205, 255), 4 * s);
        return;
    }
    const float mt = 48.0f;                                                       // texture margin
    float m = std::fmin(28.0f * s, std::fmin((b.x - a.x), (b.y - a.y)) * 0.45f);   // screen margin
    float xs[4] = {a.x, a.x + m, b.x - m, b.x}, ys[4] = {a.y, a.y + m, b.y - m, b.y};
    float us[4] = {0, mt / t.w, 1 - mt / t.w, 1}, vs[4] = {0, mt / t.h, 1 - mt / t.h, 1};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            dl->AddImage((ImTextureID)t.id, ImVec2(xs[i], ys[j]), ImVec2(xs[i + 1], ys[j + 1]), ImVec2(us[i], vs[j]),
                         ImVec2(us[i + 1], vs[j + 1]), tint);
}

void tooltip(const char* text) {
    const float s = g.scale;
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    ImGui::SetNextWindowPos(ImVec2(mouse.x + 48.0f * std::fmax(1.0f, s), mouse.y + 6.0f));
    if (!active()) {
        if (ImGui::BeginTooltip()) {
            ImGui::TextUnformatted(text);
            ImGui::EndTooltip();
        }
        return;
    }
    // same structure as Combat Roster Panel's paper_tooltip(): content on channel 1, the paper behind it
    ImGui::PushStyleColor(ImGuiCol_PopupBg, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleColor(ImGuiCol_Text, kInk);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18 * s, 16 * s));
    ImGui::BeginTooltip();
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->ChannelsSplit(2);
    dl->ChannelsSetCurrent(1);
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 18.0f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    dl->ChannelsSetCurrent(0);
    dl->PushClipRectFullScreen();
    paper(dl, win->Pos, ImVec2(win->Pos.x + win->Size.x, win->Pos.y + win->Size.y));
    dl->PopClipRect();
    dl->ChannelsMerge();
    ImGui::EndTooltip();
    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(3);
}

}  // namespace gamelook
