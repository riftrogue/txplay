#pragma once

// NowPlaying.hpp — persistent player bar at the bottom of the screen.
//
// Pure element builder: no mutable state, no FTXUI components.
//
// Visual design intent:
//   - A separator line anchors it as the "footer" of the application.
//   - No border box — it should feel integrated, not framed.
//   - Track line: icon + "Title - Artist" (left) + "01:24 / 04:12" (right)
//   - Progress line: subtle seek bar spanning the full width
//
// Nothing-playing state: minimal idle line, not an error message.

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
// status_icon() — Unicode play/pause/stop indicator.
// ---------------------------------------------------------------------------
inline std::string status_icon(txplay::audio::PlaybackState s) {
    using PS = txplay::audio::PlaybackState;
    if (s == PS::Playing) return "\xe2\x96\xb6"; // ▶
    if (s == PS::Paused)  return "\xe2\x8f\xb8"; // ⏸
    return "\xc2\xb7";                            // · (stopped — subtle)
}

// ---------------------------------------------------------------------------
// build_now_playing()
//
// Builds the persistent bottom player bar.
// `compact` param is kept for API compatibility but the layout is the same
// in both modes — the bar is always 3 rows: separator + track + progress.
// ---------------------------------------------------------------------------
inline ftxui::Element build_now_playing(const NowPlayingData& d,
                                        int terminal_width,
                                        bool /*compact*/) {
    using namespace ftxui;

    const int inner_width = std::max(10, terminal_width - 2);

    if (!d.track) {
        // Nothing selected — one subtle idle line.
        return vbox({
            separator() | color(Color::GrayDark),
            hbox({
                text("  "),
                text(status_icon(txplay::audio::PlaybackState::Stopped))
                    | color(Color::GrayDark),
                text("  ready") | color(Color::GrayDark),
                filler(),
            }),
            // Empty seek bar (position 0) for visual consistency.
            hbox({
                text("  "),
                build_seek_bar(0.0f, std::max(10, inner_width - 4))
                    | color(Color::GrayDark),
                text("  "),
            }),
        });
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

    // Budget for track label: width minus icon(3) minus time(len) minus padding(6)
    const int label_budget = terminal_width
        - 3                                         // icon + space
        - static_cast<int>(time_str.size())
        - 6;                                        // padding both sides
    const std::string display_label =
        truncate_track_row(t.title, t.artist, std::max(8, label_budget));

    // Seek bar width: full inner width minus small indent on both sides.
    const int bar_w = std::max(10, inner_width - 4);

    Element track_line = hbox({
        text("  "),
        text(status_icon(d.state)) | color(Color::Cyan),
        text("  "),
        text(display_label) | bold | color(Color::White) | flex,
        text(time_str) | color(Color::GrayLight),
        text("  "),
    });

    Element seek_line = hbox({
        text("  "),
        build_seek_bar(progress, bar_w) | color(Color::GrayDark),
        text("  "),
    });

    return vbox({
        separator() | color(Color::GrayDark),
        track_line,
        seek_line,
    });
}

} // namespace txplay::ui
