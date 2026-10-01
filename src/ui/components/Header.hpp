#pragma once

// Header.hpp — application branding panel.
//
// Pure element builder. Accepts terminal width so the wordmark
// degrades gracefully across screen sizes.
//
// Rendered shape:
//
//   ┌─────────────────────────────────────────────────────────────────────┐
//   │                                                                     │
//   │  ████████╗██╗  ██╗██████╗ ██╗      █████╗ ██╗   ██╗               │
//   │  ╚══██╔══╝╚██╗██╔╝██╔══██╗██║     ██╔══██╗╚██╗ ██╔╝   /  Search   │
//   │     ██║    ╚███╔╝ ██████╔╝██║     ███████║ ╚████╔╝    Spc Pause    │
//   │     ██║    ██╔██╗ ██╔═══╝ ██║     ██╔══██║  ╚██╔╝      q  Quit    │
//   │     ██║   ██╔╝ ██╗██║     ███████╗██║  ██║   ██║        r  Rescan  │
//   │     ╚═╝   ╚═╝  ╚═╝╚═╝     ╚══════╝╚═╝  ╚═╝   ╚═╝                 │
//   │                                       TERMINAL MUSIC PLAYER  local  │
//   └─────────────────────────────────────────────────────────────────────┘
//
// Heights by mode:
//   Wide  (≥100): 11 rows (border + blank + 6 art + blank + tagline + border)
//   Medium (≥60):  9 rows (border + 6 art + tagline + border... + local)
//   Narrow (<60):  5 rows (border + blank + name + tagline + border)

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace txplay::ui {

inline ftxui::Element build_header(int width) {
    using namespace ftxui;

    const Color amber    = Color::RGB(210, 105, 30);
    const Color dim      = Color::GrayDark;

    // Shortcut hint column — only shows on wide screens.
    // Uses only keybinds that actually exist in Txplay.
    auto shortcuts_col = vbox({
        hbox({ text("  /") | bold | color(Color::Cyan), text("  Search")   | color(dim) }),
        hbox({ text("Spc") | bold | color(Color::Cyan), text("  Play/Pause")| color(dim) }),
        hbox({ text("  q") | bold | color(Color::Cyan), text("  Quit")     | color(dim) }),
        hbox({ text("  r") | bold | color(Color::Cyan), text("  Rescan")   | color(dim) }),
    });

    // -----------------------------------------------------------------------
    // WIDE (≥100 cols): Full 6-line ASCII art + shortcuts on right
    // Border + blank + (art row 1) + art rows 2-6 + blank + tagline/local = 11
    // -----------------------------------------------------------------------
    if (width >= 100) {
        return vbox({
            text(""),
            hbox({
                // Art column
                vbox({
                    text("  \u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2557\u2588\u2588\u2557  \u2588\u2588\u2557\u2588\u2588\u2588\u2588\u2588\u2588\u2557 \u2588\u2588\u2557      \u2588\u2588\u2588\u2588\u2588\u2557 \u2588\u2588\u2557   \u2588\u2588\u2557") | bold | color(amber),
                    text("  \u255a\u2550\u2550\u2588\u2588\u2554\u2550\u2550\u255d\u255a\u2588\u2588\u2557\u2588\u2588\u2554\u255d\u2588\u2588\u2554\u2550\u2550\u2588\u2588\u2557\u2588\u2588\u2551     \u2588\u2588\u2554\u2550\u2550\u2588\u2588\u2557\u255a\u2588\u2588\u2557 \u2588\u2588\u2554\u255d") | bold | color(amber),
                    text("     \u2588\u2588\u2551    \u255a\u2588\u2588\u2588\u2554\u255d \u2588\u2588\u2588\u2588\u2588\u2588\u2554\u255d\u2588\u2588\u2551     \u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2551 \u255a\u2588\u2588\u2588\u2588\u2554\u255d ") | bold | color(amber),
                    text("     \u2588\u2588\u2551    \u2588\u2588\u2554\u2588\u2588\u2557 \u2588\u2588\u2554\u2550\u2550\u2550\u255d \u2588\u2588\u2551     \u2588\u2588\u2554\u2550\u2550\u2588\u2588\u2551  \u255a\u2588\u2588\u2554\u255d  ") | bold | color(amber),
                    text("     \u2588\u2588\u2551   \u2588\u2588\u2554\u255d \u2588\u2588\u2557\u2588\u2588\u2551     \u2588\u2588\u2588\u2588\u2588\u2588\u2557\u2588\u2588\u2551  \u2588\u2588\u2551   \u2588\u2588\u2551   ") | bold | color(amber),
                    text("     \u255a\u2550\u255d   \u255a\u2550\u255d  \u255a\u2550\u255d\u255a\u2550\u255d     \u255a\u2550\u2550\u2550\u2550\u2550\u255d\u255a\u2550\u255d  \u255a\u2550\u255d   \u255a\u2550\u255d   ") | color(amber),
                }),
                filler(),
                // Shortcuts column, right-aligned
                shortcuts_col,
                text("  "),
            }),
            text(""),
            hbox({
                text("  ") | color(dim),
                text("TERMINAL MUSIC PLAYER") | color(dim),
                filler(),
                text("local  ") | color(dim),
            }),
        }) | border | color(dim);
    }

    // -----------------------------------------------------------------------
    // MEDIUM (≥60 cols): 6-line art, no shortcuts panel, tagline + local
    // Border + 6 art + tagline/local = 9 rows total (no blank padding)
    // -----------------------------------------------------------------------
    if (width >= 60) {
        return vbox({
            text("\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2557\u2588\u2588\u2557  \u2588\u2588\u2557\u2588\u2588\u2588\u2588\u2588\u2588\u2557 \u2588\u2588\u2557      \u2588\u2588\u2588\u2588\u2588\u2557 \u2588\u2588\u2557   \u2588\u2588\u2557")
                | bold | color(amber),
            text("\u255a\u2550\u2550\u2588\u2588\u2554\u2550\u2550\u255d\u255a\u2588\u2588\u2557\u2588\u2588\u2554\u255d\u2588\u2588\u2554\u2550\u2550\u2588\u2588\u2557\u2588\u2588\u2551     \u2588\u2588\u2554\u2550\u2550\u2588\u2588\u2557\u255a\u2588\u2588\u2557 \u2588\u2588\u2554\u255d")
                | bold | color(amber),
            text("   \u2588\u2588\u2551    \u255a\u2588\u2588\u2588\u2554\u255d \u2588\u2588\u2588\u2588\u2588\u2588\u2554\u255d\u2588\u2588\u2551     \u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2551 \u255a\u2588\u2588\u2588\u2588\u2554\u255d ")
                | bold | color(amber),
            text("   \u2588\u2588\u2551    \u2588\u2588\u2554\u2588\u2588\u2557 \u2588\u2588\u2554\u2550\u2550\u2550\u255d \u2588\u2588\u2551     \u2588\u2588\u2554\u2550\u2550\u2588\u2588\u2551  \u255a\u2588\u2588\u2554\u255d  ")
                | bold | color(amber),
            text("   \u2588\u2588\u2551   \u2588\u2588\u2554\u255d \u2588\u2588\u2557\u2588\u2588\u2551     \u2588\u2588\u2588\u2588\u2588\u2588\u2557\u2588\u2588\u2551  \u2588\u2588\u2551   \u2588\u2588\u2551   ")
                | bold | color(amber),
            hbox({
                text("   \u255a\u2550\u255d   \u255a\u2550\u255d  \u255a\u2550\u255d\u255a\u2550\u255d     \u255a\u2550\u2550\u2550\u2550\u2550\u255d\u255a\u2550\u255d  \u255a\u2550\u255d   \u255a\u2550\u255d")
                    | color(amber),
                filler(),
                text("local  ") | color(dim),
            }),
            hbox({
                text(" TERMINAL MUSIC PLAYER") | color(dim),
                filler(),
            }),
        }) | border | color(dim);
    }

    // -----------------------------------------------------------------------
    // NARROW (<60 cols): Single bold line + tagline
    // Border + blank + TXPLAY + tagline + blank = 5 rows
    // -----------------------------------------------------------------------
    return vbox({
        text(""),
        hbox({
            text("  TXPLAY") | bold | color(amber),
            filler(),
            text("local  ") | color(dim),
        }),
        text("  terminal music player") | color(dim),
        text(""),
    }) | border | color(dim);
}

} // namespace txplay::ui
