#pragma once

#include "imgui.h"

// The menu's "game look": the game's own fonts and watercolour paper, read from the player's resources.gpak at
// runtime (nothing is copied or shipped). Approach and code from Combat Roster Panel (MIT, see ATTRIBUTION.md).
// If the files cannot be read, the menu keeps the default ImGui style (active() stays false) and one line goes to the log.
//
// Exporter: gamelook.cpp

namespace gamelook {

// Starts the background loader (once). `enabled` false = never load (development flag), the default style is used.
void start(bool enabled);

// Game thread, ImGui context created and GL context current, BEFORE the backends' NewFrame: uploads the paper to GL,
// adds the game's fonts when they are ready, tells the loader the GL context changed.
void before_frame();
void gl_context_lost();

// After the backends' NewFrame, before ImGui::NewFrame(): scale from the display height.
void apply_scale(float display_height);

// True when the game's fonts and paper are loaded: the menu then uses push_style()/paper()/tooltip().
bool active();
// UI scale (1.0 at 1080p).
float scale();

// Paper and ink colours for everything the menu draws. Pair with pop_style().
void push_style();
void pop_style();

// The font used for the window title (the game's title font), or the default font if not available.
ImFont* title_font();
// Unscaled pixel size to use for the title (multiply by scale()).
float title_px();

// The game's paper as a 9-slice rectangle (inked edge keeps its thickness, the middle stretches).
void paper(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 tint = IM_COL32(255, 249, 234, 255));

// Tooltip on paper, placed to the right of the mouse cursor (the default position is under the game's own cursor).
void tooltip(const char* text);

}  // namespace gamelook
