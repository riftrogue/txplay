// ui_util_test_runner.cpp
// Tests for pure UI utility functions in Util.hpp.
// No FTXUI dependency — compiles without the UI framework.

#include "ui/Util.hpp"

#include <cassert>
#include <iostream>
#include <string>

using txplay::ui::format_track_row;
using txplay::ui::truncate_track_row;
using txplay::ui::format_time;

// ---------------------------------------------------------------------------
// format_track_row tests
// ---------------------------------------------------------------------------

static void test_format_track_row() {
    // Normal case
    assert(format_track_row("Enna Sona", "A. R. Rahman") == "Enna Sona - A. R. Rahman");

    // Empty artist: no trailing " - "
    assert(format_track_row("Enna Sona", "") == "Enna Sona");

    // Empty title: show artist only
    assert(format_track_row("", "A. R. Rahman") == "A. R. Rahman");

    // Both empty: show "Unknown"
    assert(format_track_row("", "") == "Unknown");

    // Title with a " - " in it (should not be treated as a delimiter)
    assert(format_track_row("A - B", "Artist") == "A - B - Artist");

    std::cout << "  format_track_row: PASS\n";
}

// ---------------------------------------------------------------------------
// truncate_track_row tests
// ---------------------------------------------------------------------------

static void test_truncate_track_row() {
    const std::string title  = "Enna Sona";
    const std::string artist = "A. R. Rahman";
    const std::string full   = "Enna Sona - A. R. Rahman"; // 24 chars

    // Full string fits at exactly its length
    assert(truncate_track_row(title, artist, 24) == full);

    // Full string fits with extra room
    assert(truncate_track_row(title, artist, 80) == full);

    // Zero / negative width -> empty
    assert(truncate_track_row(title, artist,  0) == "");
    assert(truncate_track_row(title, artist, -5) == "");

    // Width of 1 -> "..." only (ellipsis char)
    {
        std::string r = truncate_track_row(title, artist, 1);
        assert(!r.empty()); // should be "..."
    }

    // Artist should be preserved when title is truncated
    // " - A. R. Rahman" is 16 chars; we give total of 20 = 4 for title portion
    // => "Enn..." (3 chars + "...") + " - A. R. Rahman" = 4 + 16 = 20
    {
        std::string r = truncate_track_row(title, artist, 20);
        // Artist must appear fully in the result
        assert(r.find("A. R. Rahman") != std::string::npos);
        assert(static_cast<int>(r.size()) <= 20);
    }

    // Very narrow: artist alone won't fit, fall back to uniform truncation
    {
        std::string r = truncate_track_row(title, artist, 5);
        assert(static_cast<int>(r.size()) <= 5);
    }

    // Empty artist: just truncate title
    {
        std::string r = truncate_track_row("Long Title Here", "", 8);
        assert(static_cast<int>(r.size()) <= 8);
        // Must end with ellipsis
        assert(r.back() != ' ');
    }

    // Empty title, non-empty artist: returns artist (potentially truncated)
    {
        std::string r = truncate_track_row("", "A. R. Rahman", 80);
        assert(r == "A. R. Rahman");
    }

    // Both empty
    {
        std::string r = truncate_track_row("", "", 80);
        assert(r == "Unknown");
    }

    // Title with embedded " - " — artist info must still be at the end
    {
        std::string r = truncate_track_row("A - B", "Artist", 80);
        assert(r == "A - B - Artist");
    }

    // Exact boundary: 23 cols (one less than full) -> must truncate title
    {
        std::string r = truncate_track_row(title, artist, 23);
        assert(static_cast<int>(r.size()) <= 23);
        assert(r.find("A. R. Rahman") != std::string::npos);
    }

    std::cout << "  truncate_track_row: PASS\n";
}

// ---------------------------------------------------------------------------
// format_time tests (regression)
// ---------------------------------------------------------------------------

static void test_format_time() {
    assert(format_time(0)          == "00:00");
    assert(format_time(60000)      == "01:00");
    assert(format_time(3661000)    == "1:01:01");
    assert(format_time(90000)      == "01:30");

    std::cout << "  format_time: PASS\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    std::cout << "--- UI Util Test Runner ---\n";

    test_format_track_row();
    test_truncate_track_row();
    test_format_time();

    std::cout << "\nAll UI util assertions passed!\n";
    return 0;
}
