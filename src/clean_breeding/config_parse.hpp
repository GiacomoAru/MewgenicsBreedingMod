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

// "head:704, tail:704" -> {("head",704),("tail",704)}; malformed items are ignored.
inline std::set<std::pair<std::string, int>> parse_defects(std::string_view s) {
    std::set<std::pair<std::string, int>> out;
    for(const auto &item : parse_csv(s)) {
        size_t colon = item.find(':');
        if(colon == std::string::npos || colon == 0 || colon + 1 >= item.size()) {
            continue;
        }
        try {
            size_t used = 0;
            int id = std::stoi(item.substr(colon + 1), &used);
            if(used == item.size() - colon - 1) {
                out.emplace(item.substr(0, colon), id);
            }
        } catch(...) {
        }
    }
    return out;
}

inline int clamp_level(int v) {
    return v < 0 ? 0 : (v > 3 ? 3 : v);
}
