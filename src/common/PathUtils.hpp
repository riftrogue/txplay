#pragma once

#include <string>
#include <cstdlib>

// Generic, Txplay-agnostic path utilities.
// Header-only: all functions are inline pure helpers with no state.

namespace txplay::common {

// Expands a leading '~' to the value of $HOME.
// Returns the path unchanged if it does not start with '~' or if $HOME is unset.
inline std::string expand_tilde(const std::string& path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }
    const char* home = std::getenv("HOME");
    if (!home) {
        return path; // $HOME not set; return unchanged
    }
    if (path.length() == 1) {
        return std::string(home); // bare '~'
    }
    if (path[1] == '/') {
        return std::string(home) + path.substr(1); // ~/...
    }
    return path; // ~username/... — not handled, return unchanged
}

} // namespace txplay::common
