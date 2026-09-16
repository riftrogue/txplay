#pragma once

#include <string>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>

// Pure UI display utilities. No FTXUI dependency.
// Functions in this header exist only to format data for terminal display.

namespace txplay::ui {

inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

inline std::string format_time(uint64_t ms) {
    uint64_t total_seconds = ms / 1000;
    uint64_t hours   = total_seconds / 3600;
    uint64_t minutes = (total_seconds % 3600) / 60;
    uint64_t seconds = total_seconds % 60;

    char buf[32];
    if (hours > 0) {
        snprintf(buf, sizeof(buf), "%lu:%02lu:%02lu", hours, minutes, seconds);
    } else {
        snprintf(buf, sizeof(buf), "%02lu:%02lu", minutes, seconds);
    }
    return std::string(buf);
}

} // namespace txplay::ui
