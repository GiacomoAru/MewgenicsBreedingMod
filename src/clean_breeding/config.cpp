#include "config.hpp"
#include "amoeboid.hpp"
#include "config_parse.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/strings.hpp"

#include <windows.h>

#include <filesystem>

static std::wstring ini_path() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(reinterpret_cast<HMODULE>(G.dll_base_va), buf, MAX_PATH);
    return (std::filesystem::path(buf).parent_path() / "config.ini").wstring();
}

static std::string read_string(const wchar_t *section, const wchar_t *key, const wchar_t *def = L"") {
    static std::wstring path = ini_path();
    wchar_t buf[4096];
    GetPrivateProfileStringW(section, key, def, buf, 4096, path.c_str());
    return convert_utf16_wstring_to_utf8_string(buf);
}

static void write_string(const wchar_t *section, const wchar_t *key, const std::wstring &value) {
    static std::wstring path = ini_path();
    if(!WritePrivateProfileStringW(section, key, value.c_str(), path.c_str())) {
        D::error("Could not write config.ini (error {})", GetLastError());
    }
}

Config &config() {
    static Config c;
    return c;
}

void config_load() {
    Config &c = config();
    auto level = [](const wchar_t *key) {
        std::string s = read_string(L"breeding", key, L"0");
        try {
            return clamp_level(std::stoi(s));
        } catch(...) {
            return 0;
        }
    };
    c.inbreeding = level(L"inbreeding");
    c.heredity = level(L"heredity");
    D::info("Config: inbreeding={} heredity={}",
        c.inbreeding, c.heredity);
}

void config_save_breeding() {
    write_string(L"breeding", L"inbreeding", std::to_wstring(config().inbreeding));
    write_string(L"breeding", L"heredity", std::to_wstring(config().heredity));
}
