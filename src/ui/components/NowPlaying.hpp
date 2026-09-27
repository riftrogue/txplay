#pragma once

// NowPlaying.hpp — persistent player bar at the bottom of the screen.
//
// Pure element builder: no mutable state, no FTXUI components.
//
// Visual design intent:
//   - Proper bordered rectangle, matching the header's border language.
//   - Track information on one row: icon + label (left) + time (right).
//   - Progress bar on its own row below.
//   - Padding inside the border for breathing room (~3-4 rows total).
//
//   ┌──────────────────────────────────────────────────────────────┐
//   │  ▶  Chaleya - Arijit Singh                    00:36 / 03:20 │
//   │     ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ │
//   └──────────────────────────────────────────────────────────────┘
//
// Nothing-playing idle state uses the same border with a subtle message.

#include <string>
#include <optional>
#include <cstdint>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include "audio/PlaybackState.hpp"
#include "library/Track.hpp"
#include "ui/Util.hpp"
#include "ui/Widgets.hpp"

namespace txplay::ui {

// ---------------------------------------------------------------------------
// NowPlayingData
// ---------------------------------------------------------------------------
struct NowPlayingData {
    std::optional<txplay::library::Track> track; // nullopt = nothing playing
    txplay::audio::PlaybackState state{txplay::audio::PlaybackState::Stopped};
    uint64_t position_ms{0};
    uint64_t duration_ms{0};
};

// ---------------------------------------------------------------------------
// status_icon()
// ---------------------------------------------------------------------------
inline std::string status_icon(txplay::audio::PlaybackState s) {
    using PS = txplay::audio::PlaybackState;
    if (s == PS::Playing) return "\xe2\x8f\xb8"; // ⏸  (playing → press to pause)
    if (s == PS::Paused)  return "\xe2\x96\xb6"; // ▶  (paused  → press to play)
    return "\xc2\xb7";                            // ·  (stopped — subtle)
}

// ---------------------------------------------------------------------------
// build_now_playing()
//
// Renders the bottom player panel as a proper bordered box.
// terminal_width is used to budget the label truncation.
// The `compact` param is kept for API compatibility; layout is always the same.
// ---------------------------------------------------------------------------
inline ftxui::Element build_now_playing(const NowPlayingData& d,
                                        int terminal_width,
                                        bool /*compact*/) {
    using namespace ftxui;

    // Inner width for seek bar: subtract border (2) + side padding (4).
    const int bar_w = std::max(10, terminal_width - 6);

    if (!d.track) {
        // Idle state: same border shape, subtle content.
        return vbox({
            hbox({
                text("  "),
                text(status_icon(txplay::audio::PlaybackState::Stopped))
                    | color(Color::GrayDark),
                text("  ready") | color(Color::GrayDark),
                filler(),
            }),
            hbox({
                text("  "),
                build_seek_bar(0.0f, bar_w) | color(Color::GrayDark),
                text("  "),
            }),
        }) | border | color(Color::GrayDark);
    }

    // ----- Track is playing / paused -----
    const auto& t = *d.track;

    uint64_t pos_ms = d.position_ms;
    if (d.duration_ms > 0 && pos_ms > d.duration_ms) pos_ms = d.duration_ms;

    const std::string time_str = (d.duration_ms > 0)
        ? format_time(pos_ms) + " / " + format_time(d.duration_ms)
        : format_time(pos_ms);

    const float progress = (d.duration_ms > 0)
        ? static_cast<float>(pos_ms) / static_cast<float>(d.duration_ms)
        : 0.0f;

    // Label budget: terminal_width minus border(2) minus icon(4) minus
    //              time string minus side padding(6).
    const int label_budget = terminal_width
        - 2                                         // border chars
        - 4                                         // icon + spaces
        - static_cast<int>(time_str.size())
        - 6;                                        // padding both sides
    const std::string display_label =
        truncate_track_row(t.title, t.artist, std::max(8, label_budget));

    // Track line: icon + label (flex) + right-aligned time.
    Element track_line = hbox({
        text("  "),
        text(status_icon(d.state)) | color(Color::Cyan),
        text("  "),
        text(display_label) | bold | color(Color::White) | flex,
        text(time_str) | color(Color::GrayLight),
        text("  "),
    });

    // Progress bar on its own row.
    Element seek_line = hbox({
        text("  "),
        build_seek_bar(progress, bar_w) | color(Color::GrayDark),
        text("  "),
    });

    return vbox({
        track_line,
        seek_line,
    }) | border | color(Color::GrayDark);
}

} // namespace txplay::ui
