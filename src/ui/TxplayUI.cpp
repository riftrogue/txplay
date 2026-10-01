#include "TxplayUI.hpp"
#include "Util.hpp"
#include "Widgets.hpp"
#include "Visualizer.hpp"
#include "InputAdapter.hpp"
#include "components/NowPlaying.hpp"
#include "components/Header.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>
#include <ftxui/screen/color.hpp>

#include "audio/PlaybackState.hpp"

#include <chrono>
#include <algorithm>

using namespace ftxui;
using namespace std::chrono_literals;
using txplay::common::Key;

namespace txplay::ui {

// ---------------------------------------------------------------------------
// Layout breakpoints
// ---------------------------------------------------------------------------
static constexpr int kWideWidth   = 100;
static constexpr int kMediumWidth =  60;

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

TxplayUI::TxplayUI(
    txplay::application::Application& app,
    txplay::config::Config&           config,
    volatile std::sig_atomic_t&       shutdown_requested
)
    : app_(app)
    , config_(config)
    , shutdown_requested_(shutdown_requested)
    , screen_(ScreenInteractive::Fullscreen())
{}

TxplayUI::~TxplayUI() {
    keep_running_ = false;
    if (ticker_thread_.joinable()) {
        ticker_thread_.join();
    }
}

// ---------------------------------------------------------------------------
// run()
// ---------------------------------------------------------------------------

void TxplayUI::run() {
    auto final_app = build_ui();

    Loop loop(&screen_, final_app);
    while (!loop.HasQuitted() && keep_running_) {
        if (shutdown_requested_) {
            keep_running_ = false;
            screen_.Exit();
            break;
        }
        loop.RunOnce();
    }

    keep_running_ = false;
}

// make_menu_option() — custom transform for refined song list row styling.
//
// The default FTXUI Menu uses a full-width inverted-color highlight block.
// We replace it with a subtler prefix-cursor style:
//   focused:   "▸ Title" in white/bold  (cursor is here)
//   active:    "▸ Title" in gray        (selected but focus elsewhere)
//   normal:    "  Title" in gray
// ---------------------------------------------------------------------------
static MenuOption make_menu_option() {
    MenuOption opt = MenuOption::Vertical();
    opt.entries_option.transform = [](const EntryState& state) -> Element {
        Element e = text(state.label);
        if (state.focused) {
            return hbox({
                text("\xe2\x96\xb8 ") | color(Color::Cyan),  // ▸
                e | bold | color(Color::White),
            });
        } else if (state.active) {
            return hbox({
                text("\xe2\x96\xb8 ") | color(Color::GrayDark),
                e | color(Color::GrayLight),
            });
        } else {
            return hbox({
                text("  "),
                e | color(Color::GrayLight),
            });
        }
    };
    return opt;
}

// make_queue_menu_option() — queue rows use a dot cursor; visually secondary.
static MenuOption make_queue_menu_option() {
    MenuOption opt = MenuOption::Vertical();
    opt.entries_option.transform = [](const EntryState& state) -> Element {
        Element e = text(state.label);
        if (state.focused) {
            return hbox({
                text("\xc2\xb7 ") | color(Color::Cyan),  // ·
                e | color(Color::White),
            });
        } else if (state.active) {
            return hbox({
                text("\xc2\xb7 ") | color(Color::GrayDark),
                e | color(Color::GrayDark),
            });
        } else {
            return hbox({
                text("  "),
                e | color(Color::GrayDark),
            });
        }
    };
    return opt;
}

// ---------------------------------------------------------------------------
// build_ui() — builds the FTXUI component tree
// ---------------------------------------------------------------------------

ftxui::Component TxplayUI::build_ui() {
    // Ticker thread: drives ~30 FPS redraws for progress bar and visualizer.
    ticker_thread_ = std::thread([this]() {
        while (keep_running_) {
            std::this_thread::sleep_for(33ms);
            screen_.PostEvent(Event::Custom);
        }
    });

    // Search input — styled with a custom transform.
    // Placeholder shows the search keybind hint; content is white when typing.
    InputOption search_opt;
    search_opt.multiline = false;
    search_opt.transform = [](InputState state) -> Element {
        using namespace ftxui;
        auto e = state.element;
        if (state.is_placeholder) {
            e = e | color(Color::GrayDark);
        } else if (state.focused) {
            e = e | color(Color::White);
        } else {
            e = e | color(Color::GrayLight);
        }
        return e;
    };
    auto search_input = Input(&search_query_, "/ search library...", search_opt);

    auto library_menu = Menu(&library_items_, &selected_library_, make_menu_option());
    auto queue_menu   = Menu(&queue_items_,   &selected_queue_,   make_queue_menu_option());

    // ClickToPlay: converts mouse release / Enter on library into play_track().
    class ClickToPlay : public ComponentBase {
        std::function<void()> on_play_;
    public:
        ClickToPlay(Component child, std::function<void()> on_play)
            : on_play_(std::move(on_play)) { Add(child); }

        bool OnEvent(Event event) override {
            if (event == Event::Return) { on_play_(); return true; }
            if (event.is_mouse() &&
                event.mouse().button == Mouse::Left &&
                event.mouse().motion == Mouse::Released) {
                Event fake = event;
                fake.mouse().motion = Mouse::Pressed;
                if (ComponentBase::OnEvent(fake)) { on_play_(); return true; }
            }
            return ComponentBase::OnEvent(event);
        }
    };

    auto wrapped_library_menu = Make<ClickToPlay>(
        library_menu,
        [this]() {
            if (selected_library_ >= 0 &&
                selected_library_ < static_cast<int>(filtered_tracks_.size())) {
                app_.play_track(filtered_tracks_[selected_library_].id);
            }
        }
    );

    // ---- Layout container --------------------------------------------------
    auto container = Container::Vertical({
        search_input,
        Container::Horizontal({ wrapped_library_menu, queue_menu })
    });

    // ---- Renderer (frame function) -----------------------------------------
    auto renderer = Renderer(container, [
        this,
        wrapped_library_menu,
        search_input,
        queue_menu
    ] {
        // Drive application state machine once per frame.
        app_.update();

        // ---- Synchronize Queue display -------------------------------------
        {
            auto q = app_.get_queue();
            queue_items_.clear();

            if (q.empty()) {
                queue_items_.push_back("empty");
            } else {
                auto all_tracks = app_.get_tracks();
                for (const auto& qid : q) {
                    bool found = false;
                    for (const auto& t : all_tracks) {
                        if (t.id == qid) {
                            queue_items_.push_back(format_track_row(t.title, t.artist));
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        queue_items_.push_back(
                            "[?] " + qid.substr(qid.rfind('/') + 1));
                    }
                }
            }
            if (selected_queue_ >= static_cast<int>(queue_items_.size())) {
                selected_queue_ = std::max(0, static_cast<int>(queue_items_.size()) - 1);
            }
        }

        // ---- Synchronize Library display -----------------------------------
        auto all_tracks = app_.get_tracks();
        filtered_tracks_.clear();
        library_items_.clear();

        const std::string query_lower = to_lower(search_query_);
        for (const auto& track : all_tracks) {
            if (query_lower.empty() ||
                to_lower(track.title).find(query_lower)  != std::string::npos ||
                to_lower(track.artist).find(query_lower) != std::string::npos) {
                filtered_tracks_.push_back(track);
                library_items_.push_back(format_track_row(track.title, track.artist));
            }
        }

        if (app_.is_scanning()) {
            library_items_.insert(library_items_.begin(), "scanning...");
        } else if (library_items_.empty()) {
            library_items_.push_back("no tracks found");
        }

        if (selected_library_ >= static_cast<int>(library_items_.size())) {
            selected_library_ = std::max(0, static_cast<int>(library_items_.size()) - 1);
        }

        auto term = Terminal::Size();
        const int width   = term.dimx;
        const int term_h  = term.dimy;

        // ---- Now Playing data ----------------------------------------------
        NowPlayingData npd;
        npd.track       = app_.get_current_track();
        npd.state       = app_.get_playback_state();
        npd.position_ms = app_.get_position_ms();
        npd.duration_ms = app_.get_duration_ms();

        // ---- Header --------------------------------------------------------
        // build_header() knows its own height; we track it for vis_h math.
        auto header_el = build_header(width);
        // Header heights: wide=11, medium=9, narrow=6
        const int header_h = (width >= kWideWidth) ? 11
                           : (width >= kMediumWidth) ?  9 : 6;

        // ---- Search bar ----------------------------------------------------
        bool search_focused = search_input->Focused();
        Element search_el = hbox({
            text(search_focused
                 ? "  \xe2\x94\x82 "   // │ cyan when focused
                 : "  \xc2\xb7 ")      // · dim when idle
                | color(search_focused ? Color::Cyan : Color::GrayDark),
            search_input->Render() | flex,
        });
        const int search_h = 1;

        // ---- Visualizer ----------------------------------------------------
        // Placed above the song/queue content.
        // Height is computed dynamically so the entire terminal is used.
        //
        // Fixed overheads:
        //   header_h  (varies by width)
        //   search_h  = 1
        //   vis_sep   = 1 (separator above vis)
        //   content   = songs_cap (the main songs+queue area)
        //   np_h      = 6 (border + title + artist + blank + seek + border)
        //
        // For small screens, adjust for tab_bar (1 row extra).
        const int np_h      = 6;
        const int vis_sep_h = 1;
        const bool small_screen = (width < kMediumWidth);
        const int tab_h     = small_screen ? 1 : 0;

        // Songs/queue region height: ~25% of terminal, capped at 12, min 4.
        // On small screens, slightly reduced.
        const int songs_cap = small_screen
            ? std::max(4, std::min(8, term_h / 4))
            : std::max(6, std::min(12, term_h / 4));

        bool vis_enabled = config_.visualizer().enabled;
        Element visualizer_el = text(""); // unused when disabled
        int vis_h = 0;

        if (vis_enabled) {
            // vis_h = terminal_height minus everything else.
            vis_h = std::max(2,
                term_h - header_h - search_h - tab_h
                       - vis_sep_h - songs_cap - np_h);
            auto mags  = app_.get_visualizer_magnitudes();
            int  vis_w = std::max(1, width - 2);
            visualizer_el = vbox({
                separator() | color(Color::GrayDark),
                render_visualizer(mags, config_.visualizer().style, vis_w, vis_h),
            });
        }

        // ---- Now Playing ---------------------------------------------------
        auto np = build_now_playing(npd, width, /*compact=*/false);

        // ---- Section labels ------------------------------------------------
        std::string songs_label = "songs";
        if (app_.is_scanning()) songs_label = "songs  \xe2\x80\xa2 scanning";

        // ---- Mode column (narrow left strip) --------------------------------
        // Structurally separate so Online can be added later.
        // ♪ = UTF-8 E2 99 AA
        Element mode_col = vbox({
            hbox({ text(" "), text("mode") | color(Color::GrayDark) }),
            separator() | color(Color::GrayDark),
            hbox({
                text(" \xe2\x99\xaa ") | color(Color::Cyan),  // ♪
                text("Local") | color(Color::White) | bold,
            }),
            filler(),
        }) | size(WIDTH, EQUAL, 10);

        // ====================================================================
        // RESPONSIVE LAYOUT SELECTION
        // ====================================================================

        if (width >= kMediumWidth) {
            // ----------------------------------------------------------------
            // WIDE / MEDIUM  ─  Three-column: Mode | Songs | Queue
            //
            //  ┌────────── header ──────────────────────────────────────┐
            //  │  TXPLAY art                          shortcuts  local  │
            //  └────────────────────────────────────────────────────────┘
            //  · / search library...
            //  ─────────────────────────────────────────────────── (sep)
            //  ▌▌███▌▌█████▌██▌  (visualizer, if enabled)
            //  ┌──────┬──────────────────────────────┬────────────┐
            //  │ mode │ songs                        │ queue      │
            //  │ ♪ Lo │ ▸ Song - Artist              │ · Song     │
            //  │      │   Song - Artist              │   Song     │
            //  └──────┴──────────────────────────────┴────────────┘
            //  ╭───────────────────────────────────────────────────╮
            //  │ Title                                         ⏸   │
            //  │ Artist                                            │
            //  │                                                   │
            //  │ 00:36  ─────────●────────────────────────  04:12  │
            //  ╰───────────────────────────────────────────────────╯
            // ----------------------------------------------------------------

            const bool wide = (width >= kWideWidth);

            Element songs_col = vbox({
                hbox({ text("  "), text(songs_label) | color(Color::GrayDark) }),
                separator() | color(Color::GrayDark),
                wrapped_library_menu->Render() | vscroll_indicator | frame | flex,
            }) | flex | xflex_grow_factor(wide ? 4 : 3);

            Element queue_col = vbox({
                hbox({ text("  "), text("queue") | color(Color::GrayDark) }),
                separator() | color(Color::GrayDark),
                queue_menu->Render() | vscroll_indicator | frame | flex,
            }) | flex | xflex_grow_factor(wide ? 2 : 2);

            // Main content hbox: Mode | Songs | Queue
            // Songs region is size-capped so visualizer gets real estate.
            Element content_hbox = hbox({
                mode_col,
                separator() | color(Color::GrayDark),
                songs_col,
                separator() | color(Color::GrayDark),
                queue_col,
            });

            // When vis is enabled: content gets fixed songs_cap height.
            // When vis is disabled: content flexes to fill remaining space.
            Element content_el = vis_enabled
                ? (content_hbox | size(HEIGHT, EQUAL, songs_cap))
                : (content_hbox | flex);

            Elements rows;
            rows.push_back(header_el);
            rows.push_back(search_el);
            if (vis_enabled) rows.push_back(visualizer_el);
            rows.push_back(content_el);
            rows.push_back(np);
            return vbox(std::move(rows));

        } else {
            // ----------------------------------------------------------------
            // SMALL / TERMUX  ─  Single pane, tabbed Songs / Queue
            // ----------------------------------------------------------------

            bool songs_active = (small_screen_view_ == SmallScreenView::Songs);
            Element tab_bar = hbox({
                text("  "),
                text(songs_active ? "[songs]" : " songs ")
                    | color(songs_active ? Color::Cyan : Color::GrayDark),
                text("  "),
                text(!songs_active ? "[queue]" : " queue ")
                    | color(!songs_active ? Color::Cyan : Color::GrayDark),
                filler(),
                text(" \xe2\x99\xaa Local  ") | color(Color::GrayDark),
            });

            Element active_pane;
            if (songs_active) {
                active_pane = vbox({
                    separator() | color(Color::GrayDark),
                    wrapped_library_menu->Render()
                        | vscroll_indicator | frame | flex,
                }) | flex;
            } else {
                active_pane = vbox({
                    separator() | color(Color::GrayDark),
                    queue_menu->Render()
                        | vscroll_indicator | frame | flex,
                }) | flex;
            }

            Element content_el = vis_enabled
                ? (active_pane | size(HEIGHT, EQUAL, songs_cap))
                : (active_pane | flex);

            Elements rows;
            rows.push_back(header_el);
            rows.push_back(tab_bar);
            rows.push_back(search_el);
            if (vis_enabled) rows.push_back(visualizer_el);
            rows.push_back(content_el);
            rows.push_back(np);
            return vbox(std::move(rows));
        }
    });

    // ---- Global shortcuts + focus management -------------------------------
    class GlobalShortcuts : public ComponentBase {
    public:
        txplay::application::Application&  app_;
        txplay::config::Config&            config_;
        Component search_;
        Component lib_;
        Component queue_;
        std::atomic<bool>&                 keep_running_;
        ftxui::ScreenInteractive&          screen_;
        int&                               selected_library_;
        int&                               selected_queue_;
        std::vector<txplay::library::Track>& filtered_tracks_;
        SmallScreenView&                   small_screen_view_;

        GlobalShortcuts(
            Component child,
            txplay::application::Application& app,
            txplay::config::Config& config,
            Component search, Component lib, Component queue,
            std::atomic<bool>& keep_running,
            ftxui::ScreenInteractive& screen,
            int& sel_lib, int& sel_queue,
            std::vector<txplay::library::Track>& filtered,
            SmallScreenView& small_view
        )
            : app_(app), config_(config)
            , search_(search), lib_(lib), queue_(queue)
            , keep_running_(keep_running), screen_(screen)
            , selected_library_(sel_lib)
            , selected_queue_(sel_queue)
            , filtered_tracks_(filtered)
            , small_screen_view_(small_view)
        {
            Add(child);
        }

        bool OnEvent(Event event) override {
            if (event.is_mouse() || event == Event::Custom)
                return ComponentBase::OnEvent(event);

            const auto& kb      = config_.keybinds();
            const Key   pressed = key_from_event(event);
            const uint64_t seek_ms =
                static_cast<uint64_t>(config_.playback().seek_seconds) * 1000ULL;
            const int width = Terminal::Size().dimx;
            const bool small_screen = (width < kMediumWidth);

            // Focus cycling
            if (pressed == kb.focus_next || pressed == kb.focus_previous) {
                if (small_screen) {
                    small_screen_view_ =
                        (small_screen_view_ == SmallScreenView::Songs)
                        ? SmallScreenView::Queue
                        : SmallScreenView::Songs;
                    if (small_screen_view_ == SmallScreenView::Songs)
                        lib_->TakeFocus();
                    else
                        queue_->TakeFocus();
                } else {
                    std::vector<Component> focusables = { lib_, queue_ };
                    int current = 0;
                    for (int i = 0; i < static_cast<int>(focusables.size()); i++) {
                        if (focusables[i]->Focused()) { current = i; break; }
                    }
                    bool forward = (pressed == kb.focus_next);
                    current = forward
                        ? (current + 1) % static_cast<int>(focusables.size())
                        : (current - 1 + static_cast<int>(focusables.size()))
                            % static_cast<int>(focusables.size());
                    focusables[current]->TakeFocus();
                }
                return true;
            }

            // Seek
            if (!search_->Focused()) {
                if (pressed == kb.seek_forward) {
                    app_.seek(app_.get_position_ms() + seek_ms);
                    return true;
                }
                if (pressed == kb.seek_backward) {
                    uint64_t pos = app_.get_position_ms();
                    app_.seek(pos > seek_ms ? pos - seek_ms : 0);
                    return true;
                }
            }

            // Navigation remapping
            if (!search_->Focused()) {
                if (kb.navigation_up != Key::arrow_up() && pressed == kb.navigation_up)
                    return ComponentBase::OnEvent(Event::ArrowUp);
                if (kb.navigation_down != Key::arrow_down() && pressed == kb.navigation_down)
                    return ComponentBase::OnEvent(Event::ArrowDown);
            }

            // Play remapping
            if (kb.play != Key::enter() && pressed == kb.play)
                return ComponentBase::OnEvent(Event::Return);

            // Let container handle native navigation
            if (ComponentBase::OnEvent(event)) return true;

            // Application shortcuts
            if (pressed == kb.quit) {
                keep_running_ = false;
                screen_.Exit();
                return true;
            }
            if (pressed == kb.play_pause) { app_.toggle_pause();    return true; }
            if (pressed == kb.refresh)    { app_.rescan_library();  return true; }
            if (pressed == kb.next)       { app_.play_next();       return true; }
            if (pressed == kb.previous)   { app_.play_previous();   return true; }

            if (pressed == kb.search) {
                search_->TakeFocus();
                return true;
            }

            if (pressed == kb.back) {
                if (search_->Focused()) { lib_->TakeFocus(); return true; }
                return false;
            }

            if (!search_->Focused()) {
                if (pressed == kb.queue_add) {
                    if (selected_library_ >= 0 &&
                        selected_library_ < static_cast<int>(filtered_tracks_.size())) {
                        app_.queue_add(filtered_tracks_[selected_library_].id);
                    }
                    return true;
                }
                if (pressed == kb.queue_remove) {
                    if (queue_->Focused()) app_.queue_remove(selected_queue_);
                    return true;
                }
                if (pressed == kb.queue_clear) {
                    if (queue_->Focused()) app_.queue_clear();
                    return true;
                }
            }

            return false;
        }
    };

    return Make<GlobalShortcuts>(
        renderer,
        app_,
        config_,
        search_input, wrapped_library_menu, queue_menu,
        keep_running_, screen_,
        selected_library_, selected_queue_, filtered_tracks_,
        small_screen_view_
    );
}

} // namespace txplay::ui
