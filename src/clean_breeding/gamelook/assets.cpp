// Reduced from Combat Roster Panel's assets.cpp (MIT, Copyright (c) 2026 Combat Roster Panel authors, see
// ATTRIBUTION.md): the archive reader, the paper bitmap and the font loading, with the same worker-thread design.
#include "assets.h"

#include "gpak.h"
#include "swf.h"
#include "utilities/debug_console.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <thread>

namespace cr {
namespace {

constexpr int kPaperBitmap = 1;   // ui.swf: 920x923 watercolour paper with an inked edge

struct State {
    std::atomic<int> state{(int)AssetState::Loading};
    std::string failure;
    bool started = false;

    std::mutex mu;                 // guards the fields below
    std::shared_ptr<SwfFont> body_font, title_font, cjk_font;
    SwfImage paper_px;
    bool paper_ready = false;      // pixels available (worker -> game thread)
    unsigned paper_tex = 0;        // GL name, 0 until uploaded
} g;

void log_line(const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    D::info("{}", buf);
}

void fail(const std::string& why) {
    g.failure = why;
    g.state = (int)AssetState::Failed;
    log_line("Game look unavailable: %s", why.c_str());
}

void worker_body(const std::string& game_dir) {
    std::string path = game_dir + "\\resources.gpak";
    GPak gpak;
    if (!gpak.open(path)) return fail("cannot open " + path);

    std::vector<uint8_t> buf;
    {
        SwfDoc ui;
        if (!gpak.read("swfs/ui.swf", buf) || !ui.load(std::move(buf))) return fail("cannot read swfs/ui.swf");
        SwfImage im;
        if (!ui.bitmap((uint16_t)kPaperBitmap, im) || im.w <= 0) return fail("paper bitmap not found in swfs/ui.swf");
        std::lock_guard<std::mutex> lk(g.mu);
        g.paper_px = std::move(im);
        g.paper_ready = true;
    }
    {
        SwfDoc intl;   // 86 MB; only the two fonts we use are kept
        std::shared_ptr<SwfFont> body, title, cjk;
        if (gpak.read("swfs/international_fonts.swf", buf) && intl.load(std::move(buf))) {
            body = intl.font("TikaFontIntl");
            title = intl.font("Mewgenics Organ Grinder Cyr");
        }
        SwfDoc uni;   // Noto Sans CJK: what the game itself falls back to for Chinese/Japanese/Korean
        if (gpak.read("swfs/unicodefont.swf", buf)) {
            uni.load(std::move(buf));
            cjk = uni.font("Noto Sans CJK");
        }
        std::lock_guard<std::mutex> lk(g.mu);
        g.body_font = body;
        g.title_font = title;
        g.cjk_font = cjk;
        log_line("Game look: fonts body=%s title=%s cjk=%s, paper %dx%d", body ? "ok" : "MISSING", title ? "ok" : "MISSING",
                 cjk ? "ok" : "MISSING", g.paper_px.w, g.paper_px.h);
    }
    g.state = (int)AssetState::Ready;
}

// An exception escaping a std::thread would call std::terminate and take the game down with it.
void worker_main(std::string game_dir) {
    try {
        worker_body(game_dir);
    } catch (...) {
        fail("exception while reading the game's files");
    }
}

}  // namespace

void assets_start(const std::string& game_dir) {
    if (g.started) return;
    g.started = true;
    std::thread(worker_main, game_dir).detach();
}

AssetState assets_state() { return (AssetState)g.state.load(); }
const std::string& assets_failure() { return g.failure; }

Tex asset_paper() {
    Tex t;
    std::lock_guard<std::mutex> lk(g.mu);
    if (g.paper_tex) {
        t.id = g.paper_tex;
        t.w = (float)g.paper_px.w;
        t.h = (float)g.paper_px.h;
    }
    return t;
}

void assets_upload_pending() {
    std::lock_guard<std::mutex> lk(g.mu);
    if (!g.paper_ready || g.paper_tex) return;
    GLint prev = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev);
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);   // CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g.paper_px.w, g.paper_px.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, g.paper_px.rgba.data());
    glBindTexture(GL_TEXTURE_2D, (GLuint)prev);
    g.paper_tex = tex;   // pixels kept so a lost context can upload again
}

void assets_gl_lost() {
    std::lock_guard<std::mutex> lk(g.mu);
    g.paper_tex = 0;   // the name died with the old context
}

std::shared_ptr<SwfFont> font_body() { std::lock_guard<std::mutex> lk(g.mu); return g.body_font; }
std::shared_ptr<SwfFont> font_title() { std::lock_guard<std::mutex> lk(g.mu); return g.title_font; }
std::shared_ptr<SwfFont> font_cjk() { std::lock_guard<std::mutex> lk(g.mu); return g.cjk_font; }

}  // namespace cr
