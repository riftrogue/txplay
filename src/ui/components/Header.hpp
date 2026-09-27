#pragma once

// Header.hpp — application header / branding area.
//
// Pure element builder: no mutable state.
//
// Renders a bordered branding panel:
//
//   ┌──────────────────────────────────────────────────────────┐
//   │                                                          │
//   │  TXPLAY                                          local   │
//   │                                                          │
//   └──────────────────────────────────────────────────────────┘
//
// TXPLAY uses a warm burnt-orange/amber accent (Color::RGB).
// LOCAL stays muted on the right.

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace txplay::ui {

inline ftxui::Element build_header() {
    using namespace ftxui;

    // Burnt-orange / amber: warm, distinct, not neon.
    const Color amber = Color::RGB(210, 105, 30);

    return vbox({
        text(""),                                   // top padding
        hbox({
            text("  "),
            text("TXPLAY") | bold | color(amber),
            filler(),
            text("local") | color(Color::GrayDark),
            text("  "),
        }),
        text(""),                                   // bottom padding
    }) | border | color(Color::GrayDark);
}

} // namespace txplay::ui
