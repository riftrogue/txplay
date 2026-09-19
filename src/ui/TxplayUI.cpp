#include "TxplayUI.hpp"
#include "Util.hpp"
#include "Widgets.hpp"
#include "Visualizer.hpp"
#include "InputAdapter.hpp"

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
// build_ui() — builds the entire FTXUI component tree
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
    auto search_input = Input(&search_query_, "Type to search...");

    auto library_menu = Menu(&library_items_, &selected_library_);
    auto queue_menu   = Menu(&queue_items_,   &selected_queue_);

    // Click-to-play wrapper: converts mouse Release events to play commands.
    // Uses a local FTXUI ComponentBase subclass to intercept Enter and mouse.
    //
    // ClickToPlay sits BELOW GlobalShortcuts in the component hierarchy.
    // It only ever receives Event::Return, either:
    //   a) natively, when play=Enter (the default), or
    //   b) synthesized by GlobalShortcuts step 6 when play is remapped.
    //
    // Checking Event::Return directly here is intentional FTXUI component
    // behavior — it is NOT bypassing the global keybinding architecture.
    // GlobalShortcuts has already applied the key→event translation above.
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
    // Captures components by value (shared_ptr copy) so they stay alive.
    auto renderer = Renderer(container, [
        this,
        wrapped_library_menu,
        search_input,
        queue_menu
    ] {
        // Drive application state machine once per frame.
        app_.update();

        // ---- Synchronize Queue display (rebuilt each frame) ----------------
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
                            queue_items_.push_back(t.title);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        queue_items_.push_back("[missing] " +
                            qid.substr(qid.rfind('/') + 1));
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

        std::string query_lower = to_lower(search_query_);
        for (const auto& track : all_tracks) {
            if (query_lower.empty() ||
                to_lower(track.title).find(query_lower)  != std::string::npos ||
                to_lower(track.artist).find(query_lower) != std::string::npos) {
                filtered_tracks_.push_back(track);
                library_items_.push_back(track.title);
            }
        }

        if (app_.is_scanning()) {
            library_items_.insert(library_items_.begin(), "[ Scanning Library... ]");
        } else if (library_items_.empty()) {
            library_items_.push_back("[ No tracks found ]");
        }

        if (selected_library_ >= static_cast<int>(library_items_.size())) {
            selected_library_ = std::max(0, static_cast<int>(library_items_.size()) - 1);
        }

        auto term_size = Terminal::Size();
        int  width     = term_size.dimx;

        // ---- Now Playing panel ---------------------------------------------
        auto current_track  = app_.get_current_track();
        auto playback_state = app_.get_playback_state();

        std::string status_str =
            (playback_state == txplay::audio::PlaybackState::Playing) ? "▶" :
            (playback_state == txplay::audio::PlaybackState::Paused)  ? "⏸" : "⏹";

        std::string state_text =
            (playback_state == txplay::audio::PlaybackState::Playing) ? "Playing" :
            (playback_state == txplay::audio::PlaybackState::Paused)  ? "Paused"  : "Stopped";

        Element now_playing_large;
        Element now_playing_compact;

        if (current_track) {
            uint64_t pos_ms = app_.get_position_ms();
            uint64_t dur_ms = app_.get_duration_ms();
            if (pos_ms > dur_ms && dur_ms > 0) pos_ms = dur_ms;

            std::string time_str = format_time(pos_ms);
            if (dur_ms > 0) time_str += " / " + format_time(dur_ms);

            float progress_val = (dur_ms > 0)
                ? static_cast<float>(pos_ms) / static_cast<float>(dur_ms)
                : 0.0f;

            int seek_bar_width = std::max(10, width - 4);

            now_playing_large = vbox({
                text(status_str + " Now Playing: " + current_track->title)
                    | bold | color(Color::Cyan),
                text("  " + state_text + " · " + time_str) | color(Color::GrayLight),
                build_seek_bar(progress_val, seek_bar_width) | color(Color::GrayDark)
            }) | border;

            now_playing_compact = vbox({
                text(status_str + " " + current_track->title) | bold | color(Color::Cyan),
                text("  " + time_str) | color(Color::GrayLight),
                build_seek_bar(progress_val, seek_bar_width) | color(Color::GrayDark)
            }) | border;
        } else {
            now_playing_large = vbox({
                text("⏹ Nothing Playing") | bold | color(Color::GrayDark),
                text("  Stopped")         | color(Color::GrayLight),
                text("")
            }) | border;
            now_playing_compact =
                text("⏹ Nothing Playing") | bold | color(Color::GrayDark) | border;
        }

        // ---- Visualizer ----------------------------------------------------
        Element visualizer = text("");
        if (config_.visualizer().enabled) {
            auto mags      = app_.get_visualizer_magnitudes();
            int  vis_width = std::max(1, width - 4);
            visualizer = render_visualizer(
                mags,
                config_.visualizer().style,
                vis_width,
                config_.visualizer().height
            ) | border;
        }

        // ---- Search / Lists ------------------------------------------------
        auto search_box  = search_input->Render() | border;
        auto library_box = window(text("Library"),
            wrapped_library_menu->Render() | vscroll_indicator | frame) | flex;
        auto queue_box   = window(text("Queue"),
            queue_menu->Render() | vscroll_indicator | frame) | flex;

        // ---- Responsive layout ---------------------------------------------
        if (width >= 100) {
            return vbox({ search_box, now_playing_large,   visualizer,
                          hbox({library_box, queue_box}) | flex });
        } else if (width >= 60) {
            return vbox({ search_box, now_playing_compact, visualizer,
                          hbox({library_box, queue_box}) | flex });
        } else {
            return vbox({ search_box, now_playing_compact, visualizer, library_box });
        }
    });

    // ---- Global shortcuts + focus management -------------------------------
    //
    // GlobalShortcuts intercepts events before (and after) the container.
    //
    // Keybinding dispatch:
    //   1. Non-keyboard events (mouse, ticker) → pass directly to container.
    //   2. Identify pressed RIGHT-side Key via InputAdapter::key_from_event().
    //   3. Focus cycling (focus_next / focus_previous) → always intercept.
    //   4. Seek (seek_forward / seek_backward) → intercept before container.
    //   5. Navigation remapping — only intercept when user has changed the
    //      default key; otherwise let FTXUI handle native arrow keys directly.
    //   6. Play remapping — same principle (only when not Enter).
    //   7. Let the container handle the event.
    //   8. Application shortcuts — quit, play_pause, search, refresh, back,
    //      next, previous, queue actions.
    //
    // Reads keybindings from config_ on every event so that runtime changes
    // (future Settings UI) take effect immediately without rebuilding the UI.
    class GlobalShortcuts : public ComponentBase {
    public:
        txplay::application::Application& app_;
        txplay::config::Config&           config_;
        Component search_;
        Component lib_;
        Component queue_;
        std::atomic<bool>&                keep_running_;
        ftxui::ScreenInteractive&         screen_;
        int&                              selected_library_;
        int&                              selected_queue_;
        std::vector<txplay::library::Track>& filtered_tracks_;

        GlobalShortcuts(
            Component child,
            txplay::application::Application& app,
            txplay::config::Config& config,
            Component search, Component lib, Component queue,
            std::atomic<bool>& keep_running,
            ftxui::ScreenInteractive& screen,
            int& sel_lib, int& sel_queue,
            std::vector<txplay::library::Track>& filtered
        )
            : app_(app), config_(config), search_(search), lib_(lib), queue_(queue)
            , keep_running_(keep_running), screen_(screen)
            , selected_library_(sel_lib)
            , selected_queue_(sel_queue)
            , filtered_tracks_(filtered)
        {
            Add(child);
        }

        bool OnEvent(Event event) override {
            // ── 1. Pass non-keyboard events straight to the container ─────────
            // Custom events are the 30 FPS ticker; mouse is handled by ClickToPlay.
            if (event.is_mouse() || event == Event::Custom) {
                return ComponentBase::OnEvent(event);
            }

            // ── 2. Convert FTXUI event to our RIGHT-side Key ──────────────────
            const auto& kb      = config_.keybinds();
            const Key   pressed = key_from_event(event);
            const uint64_t seek_ms =
                static_cast<uint64_t>(config_.playback().seek_seconds) * 1000ULL;

            // ── 3. Focus cycling — always intercept (our custom Library↔Queue) ─
            // Search is intentionally excluded from Tab cycling; reached only via
            // the search LEFT-side binding.
            if (pressed == kb.focus_next || pressed == kb.focus_previous) {
                std::vector<Component> focusables = { lib_, queue_ };
                int current = 0;
                for (int i = 0; i < static_cast<int>(focusables.size()); i++) {
                    if (focusables[i]->Focused()) { current = i; break; }
                }
                bool forward = (pressed == kb.focus_next);
                current = forward
                    ? (current + 1) % static_cast<int>(focusables.size())
                    : (current - 1 + static_cast<int>(focusables.size())) % static_cast<int>(focusables.size());
                focusables[current]->TakeFocus();
                return true;
            }

            // ── 4. Seek — intercept before container can steal arrow keys ─────
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

            // ── 5. Navigation remapping — only intercept when NOT native key ──
            // When navigation_up=ArrowUp (default), ArrowUp passes through to
            // the FTXUI Menu natively (step 7).  When remapped to e.g. 'k',
            // synthesize Event::ArrowUp so the Menu still receives the correct
            // FTXUI event it expects.
            if (!search_->Focused()) {
                if (kb.navigation_up != Key::arrow_up() && pressed == kb.navigation_up)
                    return ComponentBase::OnEvent(Event::ArrowUp);
                if (kb.navigation_down != Key::arrow_down() && pressed == kb.navigation_down)
                    return ComponentBase::OnEvent(Event::ArrowDown);
            }

            // ── 6. Play remapping — only intercept when NOT native Enter ──────
            // ClickToPlay handles Event::Return natively.  Only synthesize when
            // the user has bound 'play' to a different key.
            if (kb.play != Key::enter() && pressed == kb.play)
                return ComponentBase::OnEvent(Event::Return);

            // ── 7. Let the container handle the event (native navigation, etc.) ─
            if (ComponentBase::OnEvent(event)) return true;

            // ── 8. Application shortcuts (after container had a chance) ────────

            // Quit — configurable; Escape no longer quits by default.
            if (pressed == kb.quit) {
                keep_running_ = false;
                screen_.Exit();
                return true;
            }

            // Play/Pause toggle
            if (pressed == kb.play_pause) {
                app_.toggle_pause();
                return true;
            }

            // Focus search input
            if (pressed == kb.search) {
                search_->TakeFocus();
                return true;
            }

            // Rescan library
            if (pressed == kb.refresh) {
                app_.rescan_library();
                return true;
            }

            // Back — exit search focus; no-op elsewhere for now.
            if (pressed == kb.back) {
                if (search_->Focused()) {
                    lib_->TakeFocus();
                    return true;
                }
                return false;
            }

            // Next / Previous — wired to Application.
            if (pressed == kb.next)     { app_.play_next();     return true; }
            if (pressed == kb.previous) { app_.play_previous(); return true; }

            // Queue shortcuts — disabled when search has focus.
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
        selected_library_, selected_queue_, filtered_tracks_
    );
}

} // namespace txplay::ui
