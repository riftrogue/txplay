#pragma once

// Header.hpp — application header bar.
//
// Pure element builder: no mutable state.
//
// Renders:
//     TXPLAY                                          LOCAL
//     ─────────────────────────────────────────────────────
//
// The separator line establishes clear visual separation from content below.

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace txplay::ui {

inline ftxui::Element build_header() {
    using namespace ftxui;

    return vbox({
        hbox({
            text(" txplay") | bold | color(Color::Cyan),
            filler(),
            text("local") | color(Color::GrayDark),
            text("  "),
        }),
        separator() | color(Color::GrayDark),
    });
}

} // namespace txplay::ui
