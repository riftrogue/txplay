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

// ---------------------------------------------------------------------------
// format_track_row(title, artist)
//
// Produces the canonical display string for a track row:
//
//     "Title - Artist"
//
// Edge cases:
//   - Empty artist  -> "Title"         (no trailing " - ")
//   - Empty title   -> "Unknown"       (no leading " - Artist")
//   - Both empty    -> "Unknown"
// ---------------------------------------------------------------------------
inline std::string format_track_row(const std::string& title,
                                    const std::string& artist) {
    if (title.empty() && artist.empty()) return "Unknown";
    if (title.empty())  return artist;
    if (artist.empty()) return title;
    return title + " - " + artist;
}

// ---------------------------------------------------------------------------
// truncate_track_row(title, artist, max_cols)
//
// Returns a display string for (title, artist) that fits within max_cols
// terminal columns.
//
// Priority: preserve artist information before truncating title.
//
// Algorithm:
//   1. Full "Title - Artist" fits -> return it unchanged.
//   2. Truncate title so that " - Artist" is always fully visible:
//      "Truncated Title... - Artist"
//   3. If even " - Artist" alone exceeds max_cols, truncate the whole
//      compound string uniformly.
//   4. max_cols <= 0 -> "".
// ---------------------------------------------------------------------------
inline std::string truncate_track_row(const std::string& title,
                                      const std::string& artist,
                                      int max_cols) {
    if (max_cols <= 0) return "";

    const std::string full = format_track_row(title, artist);

    // Common case: everything fits.
    if (static_cast<int>(full.size()) <= max_cols) return full;

    // If artist is empty, just truncate title with ellipsis.
    if (artist.empty()) {
        if (max_cols <= 1) return "\xe2\x80\xa6"; // UTF-8 for "..."
        return title.substr(0, static_cast<size_t>(max_cols - 1)) + "\xe2\x80\xa6";
    }

    // Attempt to show truncated title + " - Artist"
    // Suffix = " - Artist" (3 chars separator + artist)
    const std::string suffix = " - " + artist;
    const int suffix_cols = static_cast<int>(suffix.size());

    // We need room for at least 1 title char + "..." (1 col) + suffix.
    if (2 + suffix_cols <= max_cols) {
        int title_budget = max_cols - 1 - suffix_cols; // "..." counts as 1
        std::string trunc_title =
            title.substr(0, static_cast<size_t>(title_budget)) + "\xe2\x80\xa6";
        return trunc_title + suffix;
    }

    // Suffix alone does not fit either — fall back to uniform truncation.
    if (max_cols <= 1) return "\xe2\x80\xa6";
    return full.substr(0, static_cast<size_t>(max_cols - 1)) + "\xe2\x80\xa6";
}

} // namespace txplay::ui
