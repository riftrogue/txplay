#pragma once

// NowPlaying.hpp — persistent bottom player bar.
//
// Pure element builder: no mutable state, no FTXUI components.
//
// Design:
//   Left:   song title (bright) / artist (muted)
//   Right:  play/pause icon
//   Bottom: current-time ─●──── total-time   (seek bar with timestamps)
//
//   ╭──────────────────────────────────────────────────────────────────────╮
//   │  Chaleya                                                     ⏸      │
//   │  Arijit Singh                                                        │
//   │                                                                      │
//   │  00:36  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━●━━━━━━━━━━━━━━━━━━━━  03:20  │
//   ╰──────────────────────────────────────────────────────────────────────╯
//
// Idle (nothing selected):
//   ╭──────────────────────────────────────────────────────────────────────╮
//   │  No track selected                                           ·       │
//   │                                                                      │
//   │  --:--  ●─────────────────────────────────────────────────  --:--   │
//   ╰──────────────────────────────────────────────────────────────────────╯
//
// Height: always 6 rows (border×2 + title + artist + blank + seek).

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
    std::optional<txplay::library::Track> track;
    txplay::audio::PlaybackState state{txplay::audio::PlaybackState::Stopped};
    uint64_t position_ms{0};
    uint64_t duration_ms{0};
};

// ---------------------------------------------------------------------------
// status_icon() — shows what pressing the key WILL DO (toggle idiom).
// ---------------------------------------------------------------------------
inline std::string status_icon(txplay::audio::PlaybackState s) {
    using PS = txplay::audio::PlaybackState;
    if (s == PS::Playing) return "\xe2\x8f\xb8"; // ⏸
    if (s == PS::Paused)  return "\xe2\x96\xb6"; // ▶
    return "\xc2\xb7";                            // ·
}

// ---------------------------------------------------------------------------
// build_now_playing()
//
// terminal_width: used to size the seek bar and truncate labels.
// compact: reserved for API compatibility; layout is always the same.
//
// Fixed height: 6 rows.
//   Row 1 (border top)
//   Row 2: title (left)  +  icon (right)
//   Row 3: artist (muted)
//   Row 4: blank breathing room
//   Row 5: time_left  seek_bar  time_right
//   Row 6 (border bottom)
// ---------------------------------------------------------------------------
inline ftxui::Element build_now_playing(const NowPlayingData& d,
                                        int terminal_width,
                                        bool /*compact*/) {
    using namespace ftxui;

    const Color dim   = Color::GrayDark;
    const Color light = Color::GrayLight;

    // Seek-bar width: terminal minus border(2) minus timestamps(5+5)
    // minus spacing(4) minus icon-area(4) for title row.
    const int bar_w = std::max(10, terminal_width - 16);

    // ------------------------------------------------------------------
    // IDLE — nothing selected
    // ------------------------------------------------------------------
    if (!d.track) {
        Element title_row = hbox({
            text("  "),
            text("No track selected") | color(dim),
            filler(),
            text(status_icon(txplay::audio::PlaybackState::Stopped)) | color(dim),
            text("  "),
        });
        Element blank_row = text("  ");
        Element seek_row  = hbox({
            text("  --:--  ") | color(dim),
            build_seek_bar(0.0f, bar_w) | color(dim),
            text("  --:--  ") | color(dim),
        });

        return vbox({
            title_row,
            blank_row,
            seek_row,
        }) | border | color(dim);
    }

    // ------------------------------------------------------------------
    // PLAYING / PAUSED
    // ------------------------------------------------------------------
    const auto& t = *d.track;

    uint64_t pos_ms = d.position_ms;
    if (d.duration_ms > 0 && pos_ms > d.duration_ms) pos_ms = d.duration_ms;

    const float progress = (d.duration_ms > 0)
        ? static_cast<float>(pos_ms) / static_cast<float>(d.duration_ms)
        : 0.0f;

    const std::string pos_str = format_time(pos_ms);
    const std::string dur_str = (d.duration_ms > 0)
        ? format_time(d.duration_ms)
        : "--:--";

    // Title: truncated to fit (leave room for icon + padding on right).
    const int title_budget = std::max(8, terminal_width - 8);
    const std::string title_display = (static_cast<int>(t.title.size()) <= title_budget)
        ? t.title
        : t.title.substr(0, static_cast<size_t>(title_budget - 1)) + "\xe2\x80\xa6";

    // Artist: truncated similarly.
    const std::string artist_display = (static_cast<int>(t.artist.size()) <= title_budget)
        ? t.artist
        : t.artist.substr(0, static_cast<size_t>(title_budget - 1)) + "\xe2\x80\xa6";

    // Title row: title (left, white/bold) + icon (right, cyan).
    Element title_row = hbox({
        text("  "),
        text(title_display) | bold | color(Color::White) | flex,
        text(status_icon(d.state)) | color(Color::Cyan),
        text("  "),
    });

    // Artist row: muted.
    Element artist_row = hbox({
        text("  "),
        text(artist_display) | color(light) | flex,
    });

    // Blank breathing row.
    Element blank_row = text("  ");

    // Seek row: pos_time ─●──── dur_time
    Element seek_row = hbox({
        text("  "),
        text(pos_str) | color(dim),
        text("  "),
        build_seek_bar(progress, bar_w) | color(light),
        text("  "),
        text(dur_str) | color(dim),
        text("  "),
    });

    return vbox({
        title_row,
        artist_row,
        blank_row,
        seek_row,
    }) | border | color(dim);
}

} // namespace txplay::ui
