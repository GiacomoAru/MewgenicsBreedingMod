// Ported from Combat Roster Panel (https://github.com/TotSamiyMorzh/mewgenics-combat-roster), MIT License,
// Copyright (c) 2026 Combat Roster Panel authors. Used here to read the game's own fonts and paper art from the
// player's resources.gpak at runtime; nothing from the game is copied or shipped. See ATTRIBUTION.md.
// fontloader.h -- Dear ImGui font loader backed by the game's own SWF fonts
// (DefineFont3 outlines from swfs/international_fonts.swf), rasterised on
// demand by our SWF rasteriser. Missing glyphs fall through to a merged
// system font.
#pragma once

#include <memory>

struct ImFont;

namespace cr {

struct SwfFont;

// Adds `font` as a new ImGui font (plus a merged system fallback). The
// SwfFont must outlive the atlas. Returns null on failure.
// `fallback` (the game's Noto Sans CJK from swfs/unicodefont.swf) is merged
// behind it so Chinese / Japanese / Korean text renders instead of '?'.
ImFont* add_swf_font(std::shared_ptr<SwfFont> font, float size_px, std::shared_ptr<SwfFont> fallback = nullptr);

}  // namespace cr
