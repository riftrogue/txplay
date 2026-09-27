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
static constexpr int kWideWidth   = 100; // >= 100: wide — side-by-side + full viz
static constexpr int kMediumWidth =  60; // >= 60:  medium — side-by-side + compact

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

    // ---- Components --------------------------------------------------------
    auto search_input = Input(&search_query_, "Search...");

    auto library_menu = Menu(&library_items_, &selected_library_);
    auto queue_menu   = Menu(&queue_items_,   &selected_queue_);

    // ClickToPlay: converts mouse release / Enter on library into play_track().
    //
    // Sits below GlobalShortcuts. Only receives Event::Return — either natively
    // (when play=Enter, the default) or synthesized by GlobalShortcuts when
    // play is remapped.
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
                queue_items_.push_back("[ Queue is empty ]");
            } else {
                auto all_tracks = app_.get_tracks();
                for (const auto& qid : q) {
                    bool found = false;
                    for (const auto& t : all_tracks) {
                        if (t.id == qid) {
                            // Queue entries shown as "Title - Artist"
                            queue_items_.push_back(format_track_row(t.title, t.artist));
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        queue_items_.push_back(
                            "[missing] " + qid.substr(qid.rfind('/') + 1));
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
                // Library rows: "Title - Artist" (no per-row truncation here;
                // FTXUI clips naturally at the container boundary)
                library_items_.push_back(format_track_row(track.title, track.artist));
            }
        }

        if (app_.is_scanning()) {
            library_items_.insert(library_items_.begin(), "[ Scanning... ]");
        } else if (library_items_.empty()) {
            library_items_.push_back("[ No tracks found ]");
        }

        if (selected_library_ >= static_cast<int>(library_items_.size())) {
            selected_library_ = std::max(0, static_cast<int>(library_items_.size()) - 1);
        }

        // ---- Terminal dimensions -------------------------------------------
        auto term = Terminal::Size();
        const int width = term.dimx;

        // ---- Now Playing data ----------------------------------------------
        NowPlayingData npd;
        npd.track    = app_.get_current_track();
        npd.state    = app_.get_playback_state();
        npd.position_ms = app_.get_position_ms();
        npd.duration_ms = app_.get_duration_ms();

        // ---- Visualizer ----------------------------------------------------
        // When disabled: no element is produced at all (region disappears).
        Element visualizer_el = text(""); // empty by default
        bool vis_enabled = config_.visualizer().enabled;
        if (vis_enabled) {
            auto mags      = app_.get_visualizer_magnitudes();
            int  vis_width = std::max(1, width - 4);
            visualizer_el  = render_visualizer(
                mags,
                config_.visualizer().style,
                vis_width,
                config_.visualizer().height
            ) | border;
        }

        // ---- Core panel elements -------------------------------------------
        auto header_el   = build_header();
        auto search_box  = hbox({
            text(" "),
            search_input->Render() | flex,
            text(" "),
        });

        auto library_box = window(
            hbox({ text(" SONGS "), filler() }),
            wrapped_library_menu->Render() | vscroll_indicator | frame
        ) | flex;

        auto queue_box = window(
            hbox({ text(" QUEUE "), filler() }),
            queue_menu->Render() | vscroll_indicator | frame
        ) | flex;

        // ---- Responsive layout selection -----------------------------------
        //
        // WIDE   (>= 100): header, search, songs+queue side-by-side, vis, now-playing
        // MEDIUM (>= 60):  header, search, songs+queue side-by-side, vis, now-playing
        // SMALL  (<  60):  header, search, one pane at a time, vis, now-playing
        //
        // Visualizer is omitted entirely (not just hidden) when disabled.

        if (width >= kMediumWidth) {
            // Wide & Medium: both panes visible.
            bool wide = (width >= kWideWidth);
            auto np = build_now_playing(npd, width, /*compact=*/!wide);

            Elements rows;
            rows.push_back(header_el);
            rows.push_back(search_box);
            rows.push_back(hbox({ library_box, queue_box }) | flex);
            if (vis_enabled) rows.push_back(visualizer_el);
            rows.push_back(np);
            return vbox(std::move(rows));

        } else {
            // Small / Termux: one pane at a time.
            // small_screen_view_ toggled by Tab in GlobalShortcuts.
            auto np = build_now_playing(npd, width, /*compact=*/true);

            Element active_pane;
            if (small_screen_view_ == SmallScreenView::Songs) {
                active_pane = vbox({
                    library_box,
                }) | flex;
            } else {
                active_pane = vbox({
                    queue_box,
                }) | flex;
            }

            // Small-screen tab indicator
            Element tab_bar = hbox({
                text(small_screen_view_ == SmallScreenView::Songs
                     ? "[SONGS]" : " SONGS ") | bold
                     | color(small_screen_view_ == SmallScreenView::Songs
                             ? Color::Cyan : Color::GrayLight),
                text(" "),
                text(small_screen_view_ == SmallScreenView::Queue
                     ? "[QUEUE]" : " QUEUE ") | bold
                     | color(small_screen_view_ == SmallScreenView::Queue
                             ? Color::Cyan : Color::GrayLight),
                filler(),
            });

            Elements rows;
            rows.push_back(header_el);
            rows.push_back(tab_bar);
            rows.push_back(search_box);
            rows.push_back(active_pane);
            if (vis_enabled) rows.push_back(visualizer_el);
            rows.push_back(np);
            return vbox(std::move(rows));
        }
    });

    // ---- Global shortcuts + focus management -------------------------------
    //
    // Keybinding dispatch order:
    //   1. Non-keyboard events (mouse, ticker) → pass to container.
    //   2. Convert FTXUI event → txplay::common::Key.
    //   3. Focus cycling → always intercept.
    //      On small screen: Tab toggles SmallScreenView (Songs↔Queue).
    //      On wide/medium:  Tab cycles between library and queue panes.
    //   4. Seek → intercept before container can steal arrow keys.
    //   5. Navigation remapping → only intercept when not using native keys.
    //   6. Play remapping → only intercept when not using native Enter.
    //   7. Let container handle (native Menu navigation, Input typing).
    //   8. Application shortcuts (after container had a chance):
    //      quit, play_pause, search, refresh, back, next, previous, queue_*.
    //
    // Config is re-read every event so future Settings changes take effect
    // immediately without rebuilding the component tree.
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
            // 1. Pass non-keyboard events to the container.
            if (event.is_mouse() || event == Event::Custom) {
                return ComponentBase::OnEvent(event);
            }

            const auto& kb      = config_.keybinds();
            const Key   pressed = key_from_event(event);
            const uint64_t seek_ms =
                static_cast<uint64_t>(config_.playback().seek_seconds) * 1000ULL;

            const int width = Terminal::Size().dimx;
            const bool small_screen = (width < kMediumWidth);

            // 3. Focus cycling.
            if (pressed == kb.focus_next || pressed == kb.focus_previous) {
                if (small_screen) {
                    // Toggle between Songs and Queue view.
                    small_screen_view_ =
                        (small_screen_view_ == SmallScreenView::Songs)
                        ? SmallScreenView::Queue
                        : SmallScreenView::Songs;
                    // Give focus to the right component.
                    if (small_screen_view_ == SmallScreenView::Songs)
                        lib_->TakeFocus();
                    else
                        queue_->TakeFocus();
                } else {
                    // Wide/medium: cycle between library and queue.
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

            // 4. Seek — intercept before container steals arrow keys.
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

            // 5. Navigation remapping.
            if (!search_->Focused()) {
                if (kb.navigation_up != Key::arrow_up() && pressed == kb.navigation_up)
                    return ComponentBase::OnEvent(Event::ArrowUp);
                if (kb.navigation_down != Key::arrow_down() && pressed == kb.navigation_down)
                    return ComponentBase::OnEvent(Event::ArrowDown);
            }

            // 6. Play remapping.
            if (kb.play != Key::enter() && pressed == kb.play)
                return ComponentBase::OnEvent(Event::Return);

            // 7. Let container handle (typing, native nav).
            if (ComponentBase::OnEvent(event)) return true;

            // 8. Application shortcuts.
            if (pressed == kb.quit) {
                keep_running_ = false;
                screen_.Exit();
                return true;
            }

            if (pressed == kb.play_pause) {
                app_.toggle_pause();
                return true;
            }

            if (pressed == kb.search) {
                search_->TakeFocus();
                return true;
            }

            if (pressed == kb.refresh) {
                app_.rescan_library();
                return true;
            }

            if (pressed == kb.back) {
                if (search_->Focused()) {
                    lib_->TakeFocus();
                    return true;
                }
                return false;
            }

            if (pressed == kb.next)     { app_.play_next();     return true; }
            if (pressed == kb.previous) { app_.play_previous(); return true; }

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
