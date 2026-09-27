#pragma once

// Header.hpp — builds the minimal application header bar.
//
// Pure element builder: no mutable state.
// Layout:
//
//     TXPLAY                                            LOCAL
//
// The "LOCAL" badge indicates the current content mode.
// A settings gear placeholder is visually reserved for future use
// but has no interactive behavior (per design spec).

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace txplay::ui {

inline ftxui::Element build_header() {
    using namespace ftxui;

    return hbox({
        // App name — left-aligned, bold
        text(" TXPLAY") | bold | color(Color::Cyan),
        filler(),
        // Mode badge — right-aligned
        text("LOCAL") | color(Color::GrayLight),
        text("   "),
    }) | color(Color::Default);
}

} // namespace txplay::ui
