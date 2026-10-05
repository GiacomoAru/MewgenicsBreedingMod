#include "clean_breeding/config_parse.hpp"

#include <cassert>
#include <cstdio>

int main() {
    auto d = parse_csv(" EternalYouth , Chungus,,BirdFlu ");
    assert((d == std::set<std::string>{"EternalYouth", "Chungus", "BirdFlu"}));
    assert(parse_csv("").empty());

    assert(clamp_level(-1) == 0 && clamp_level(2) == 2 && clamp_level(9) == 3);
    std::puts("test_config OK");
    return 0;
}
