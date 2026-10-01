// layout_preview.cpp — offline FTXUI render preview for visual inspection.
// Renders the key UI elements to a fixed-size Screen and prints them.
// Not linked against the full app; uses only FTXUI DOM + our headers.

#include "ui/components/Header.hpp"
#include "ui/components/NowPlaying.hpp"
#include "ui/Util.hpp"
#include "ui/Widgets.hpp"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/color.hpp>

#include <iostream>
#include <string>
#include <optional>
#include <vector>

using namespace ftxui;
using txplay::ui::NowPlayingData;
using txplay::ui::build_header;
using txplay::ui::build_now_playing;
using txplay::ui::format_track_row;

// Fake library data for preview
static const std::vector<std::string> kFakeSongs = {
    "Enna Sona - A. R. Rahman",
    "Ghod Pe Sawaar (From \"Qala\") - Amit Trivedi",
    "Kun Faya Kun - A. R. Rahman",
    "Jessie's Land - A. R. Rahman",
    "Tu Hi Re - Hariharan",
    "Dil Se Re - A. R. Rahman",
};
static const std::vector<std::string> kFakeQueue = {
    "Kun Faya Kun - A. R. Rahman",
    "Tu Hi Re - Hariharan",
};

// Simulate a simple row with cursor indicator
static Element make_song_row(const std::string& label, bool focused, bool /*active*/) {
    Element e = text(label);
    if (focused) {
        return hbox({ text("\xe2\x96\xb8 ") | color(Color::Cyan), e | bold | color(Color::White) });
    }
    return hbox({ text("  "), e | color(Color::GrayLight) });
}
static Element make_queue_row(const std::string& label, bool focused) {
    Element e = text(label);
    if (focused) {
        return hbox({ text("\xc2\xb7 ") | color(Color::Cyan), e | color(Color::White) });
    }
    return hbox({ text("  "), e | color(Color::GrayDark) });
}

static void render_at(int W, const std::string& scenario, NowPlayingData npd) {
    std::cout << "\n=== " << scenario << " [width=" << W << "] ===\n";

    // Pass width to build_header() for responsive art.
    auto header_el = build_header(W);
    auto np = build_now_playing(npd, W, /*compact=*/false);

    // Search bar (unfocused)
    Element search_el = hbox({
        text("  \xc2\xb7 ") | color(Color::GrayDark),
        text("/ search library...") | color(Color::GrayDark) | flex,
    });

    // Mode column (narrow, fixed 10 cols)
    Element mode_col = vbox({
        hbox({ text(" "), text("mode") | color(Color::GrayDark) }),
        separator() | color(Color::GrayDark),
        hbox({
            text(" \xe2\x99\xaa ") | color(Color::Cyan),  // ♪
            text("Local") | color(Color::White) | bold,
        }),
        filler(),
    }) | size(WIDTH, EQUAL, 10);

    // Songs pane
    Elements song_rows;
    song_rows.push_back(hbox({ text("  "), text("songs") | color(Color::GrayDark) }));
    song_rows.push_back(separator() | color(Color::GrayDark));
    for (int i = 0; i < (int)kFakeSongs.size(); ++i) {
        song_rows.push_back(make_song_row(kFakeSongs[i], i == 0, false));
    }
    Element songs_col = vbox(std::move(song_rows)) | flex | xflex_grow_factor(W >= 100 ? 4 : 3);

    // Queue pane
    Elements q_rows;
    q_rows.push_back(hbox({ text("  "), text("queue") | color(Color::GrayDark) }));
    q_rows.push_back(separator() | color(Color::GrayDark));
    for (int i = 0; i < (int)kFakeQueue.size(); ++i) {
        q_rows.push_back(make_queue_row(kFakeQueue[i], i == 0));
    }
    q_rows.push_back(make_queue_row("empty", false));
    Element queue_col = vbox(std::move(q_rows)) | flex | xflex_grow_factor(2);

    Element content;
    if (W >= 60) {
        content = hbox({
            mode_col,
            separator() | color(Color::GrayDark),
            songs_col,
            separator() | color(Color::GrayDark),
            queue_col,
        }) | flex;
    } else {
        // Small screen: just songs pane (no mode/queue column)
        content = vbox({ songs_col }) | flex;
    }

    auto layout = vbox({
        header_el,
        search_el,
        content,
        np,
    });

    auto screen = Screen::Create(Dimension::Fixed(W), Dimension::Fit(layout));
    Render(screen, layout);
    screen.Print();
    std::cout << "\n";
}

int main() {
    // ---- Scenario 1: Nothing playing, wide ----
    {
        NowPlayingData npd;
        render_at(100, "idle/wide", npd);
    }

    // ---- Scenario 2: Playing, wide ----
    {
        NowPlayingData npd;
        txplay::library::Track t;
        t.title  = "Enna Sona";
        t.artist = "A. R. Rahman";
        t.album  = "Ok Jaanu";
        t.duration_ms = 264000;
        npd.track = t;
        npd.state = txplay::audio::PlaybackState::Playing;
        npd.position_ms = 84000;  // 01:24
        npd.duration_ms = 264000; // 04:24
        render_at(100, "playing/wide", npd);
    }

    // ---- Scenario 3: Playing, medium ----
    {
        NowPlayingData npd;
        txplay::library::Track t;
        t.title  = "Enna Sona";
        t.artist = "A. R. Rahman";
        npd.track = t;
        npd.state = txplay::audio::PlaybackState::Paused;
        npd.position_ms = 84000;
        npd.duration_ms = 264000;
        render_at(72, "paused/medium", npd);
    }

    // ---- Scenario 4: Playing, narrow ----
    {
        NowPlayingData npd;
        txplay::library::Track t;
        t.title  = "Ghod Pe Sawaar";
        t.artist = "Amit Trivedi";
        npd.track = t;
        npd.state = txplay::audio::PlaybackState::Playing;
        npd.position_ms = 30000;
        npd.duration_ms = 220000;
        render_at(45, "playing/narrow", npd);
    }

    return 0;
}
