#pragma once

#include <cctype>
#include <set>
#include <string>
#include <string_view>
#include <utility>

// Pure parsing helpers for config.ini values (no Windows API, so tests/test_config.cpp can run them).

// "a, b ,c" -> {"a","b","c"}; empty items dropped.
inline std::set<std::string> parse_csv(std::string_view s) {
    std::set<std::string> out;
    size_t pos = 0;
    while(pos <= s.size()) {
        size_t end = s.find(',', pos);
        if(end == std::string_view::npos) {
            end = s.size();
        }
        std::string_view item = s.substr(pos, end - pos);
        while(!item.empty() && std::isspace(static_cast<unsigned char>(item.front()))) item.remove_prefix(1);
        while(!item.empty() && std::isspace(static_cast<unsigned char>(item.back()))) item.remove_suffix(1);
        if(!item.empty()) {
            out.emplace(item);
        }
        pos = end + 1;
    }
    return out;
}

inline int clamp_level(int v) {
    return v < 0 ? 0 : (v > 3 ? 3 : v);
}
