// assets.h -- the game's own fonts and paper art, read from the player's resources.gpak while the game runs.
//
// Reduced from Combat Roster Panel's assets.h (MIT, see ATTRIBUTION.md): only the UI paper bitmap and the fonts.
// A worker thread opens the archive and parses swfs/ui.swf, swfs/international_fonts.swf and swfs/unicodefont.swf;
// the game thread uploads the paper to GL. Nothing is cached to disk or shipped.
#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace cr {

struct SwfFont;

enum class AssetState { Loading, Ready, Failed };

void assets_start(const std::string& game_dir);   // once; spawns the worker
AssetState assets_state();                          // Loading until the worker is done
const std::string& assets_failure();                // why, when Failed

struct Tex {
    uint64_t id = 0;   // GL texture name as an ImTextureID; 0 = not available (yet)
    float w = 0, h = 0;
};
// The watercolour paper with an inked edge (ui.swf bitmap 1). id == 0 until uploaded.
Tex asset_paper();

void assets_upload_pending();   // game thread, GL context current
void assets_gl_lost();          // the GL context was recreated: textures must be uploaded again

// The game's fonts (null if missing).
std::shared_ptr<SwfFont> font_body();    // TikaFontIntl
std::shared_ptr<SwfFont> font_title();   // Mewgenics Organ Grinder Cyr
std::shared_ptr<SwfFont> font_cjk();     // Noto Sans CJK, the game's own fallback for Chinese/Japanese/Korean

}  // namespace cr
