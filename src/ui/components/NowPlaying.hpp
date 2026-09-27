#pragma once

// NowPlaying.hpp — builds the persistent Now Playing FTXUI element.
//
// Pure element builders: no mutable state, no FTXUI components.
// Receives all data by value/const-ref; returns ftxui::Element.
//
// Two variants:
//   build_now_playing_large()   — full height, used in wide layouts
//   build_now_playing_compact() — two-line, used in medium/small layouts
//
// Both variants handle the "nothing playing" state cleanly.

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
// NowPlayingData — aggregates all data needed to render the now-playing bar.
// Populated by TxplayUI once per frame from Application queries.
// ---------------------------------------------------------------------------
struct NowPlayingData {
    std::optional<txplay::library::Track> track; // nullopt = nothing playing
    txplay::audio::PlaybackState state{txplay::audio::PlaybackState::Stopped};
    uint64_t position_ms{0};
    uint64_t duration_ms{0};
};

// ---------------------------------------------------------------------------
// status_icon() — returns the Unicode play/pause/stop icon for a state.
// ---------------------------------------------------------------------------
inline std::string status_icon(txplay::audio::PlaybackState s) {
    using PS = txplay::audio::PlaybackState;
    if (s == PS::Playing) return "\xe2\x96\xb6"; // ▶
    if (s == PS::Paused)  return "\xe2\x8f\xb8"; // ⏸
    return "\xe2\x8f\xb9";                        // ⏹
}

// ---------------------------------------------------------------------------
// build_now_playing()
//
// Single implementation that works for both wide and compact modes.
// `compact` = true  → one track line + seek bar (2 rows total)
// `compact` = false → track line + state·time line + seek bar (3 rows)
// ---------------------------------------------------------------------------
inline ftxui::Element build_now_playing(const NowPlayingData& d,
                                        int terminal_width,
                                        bool compact) {
    using namespace ftxui;

    const int bar_width = std::max(10, terminal_width - 4);

    if (!d.track) {
        // Nothing playing — minimal, non-ugly idle state.
        if (compact) {
            return hbox({
                text("  "),
                text(status_icon(txplay::audio::PlaybackState::Stopped))
                    | color(Color::GrayDark),
                text("  No track selected") | color(Color::GrayDark),
            }) | border;
        }
        return vbox({
            hbox({
                text("  "),
                text(status_icon(txplay::audio::PlaybackState::Stopped))
                    | color(Color::GrayDark),
                text("  No track selected") | color(Color::GrayDark),
            }),
            text("    " + std::string(static_cast<size_t>(std::max(0, bar_width - 4)), '-'))
                | color(Color::GrayDark),
        }) | border;
    }

    // --- Something is playing / paused. ---
    const auto& t = *d.track;

    // Cap position to duration to avoid overflow display.
    uint64_t pos_ms = d.position_ms;
    if (d.duration_ms > 0 && pos_ms > d.duration_ms) pos_ms = d.duration_ms;

    std::string time_str = format_time(pos_ms);
    if (d.duration_ms > 0) time_str += " / " + format_time(d.duration_ms);

    float progress = (d.duration_ms > 0)
        ? static_cast<float>(pos_ms) / static_cast<float>(d.duration_ms)
        : 0.0f;

    // Track label: "Title - Artist" using metadata fields.
    // format_track_row handles empty artist gracefully.
    std::string track_label = format_track_row(t.title, t.artist);

    // Reserve room for icon (2 cols) + time (e.g. "  01:24 / 04:12" = ~16 cols)
    // so the track label doesn't overlap.
    int label_budget = terminal_width - 2 - static_cast<int>(time_str.size()) - 4;
    std::string display_label = truncate_track_row(t.title, t.artist, label_budget);

    Element track_line = hbox({
        text("  "),
        text(status_icon(d.state)) | color(Color::Cyan),
        text("  "),
        text(display_label) | bold | color(Color::Cyan) | flex,
        text("  " + time_str) | color(Color::GrayLight),
        text("  "),
    });

    Element seek_line = hbox({
        text("    "),
        build_seek_bar(progress, std::max(10, bar_width - 4)) | color(Color::GrayDark),
        text("  "),
    });

    if (compact) {
        return vbox({ track_line, seek_line }) | border;
    }

    return vbox({ track_line, seek_line }) | border;
}

} // namespace txplay::ui
