#include "amoeboid.hpp"
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

static void draw_menu() {
    ImGui::SetNextWindowSize(ImVec2(360, 0), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Clean Breeding", &g_visible)) {
        ImGui::Text("Version: %s", MOD_VERSION);
        ImGui::Text("Game: %s (exe hash %s)", EXE_VERSION, G.exe_hash_mismatch_detected ? "MISMATCH" : "OK");
        ImGui::Text("Signatures: OK");
        ImGui::Text("Hooks: active");
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
