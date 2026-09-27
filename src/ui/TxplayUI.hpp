#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <csignal>

#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>

#include "application/Application.hpp"
#include "config/Config.hpp"
#include "library/Track.hpp"

namespace txplay::ui {

// ---------------------------------------------------------------------------
// SmallScreenView — the currently active pane in narrow-terminal mode.
// On wide/medium terminals both panes are visible simultaneously.
// On small terminals only one pane is shown at a time and the user can
// toggle between them via the focus_next / focus_previous keys.
// ---------------------------------------------------------------------------
enum class SmallScreenView { Songs, Queue };

// ---------------------------------------------------------------------------
// TxplayUI
//
// Owns all FTXUI state, component tree, rendering, and input handling.
//
// Dependency boundary:
//   TxplayUI -> Application (public API only)
//   TxplayUI -> Config      (reads user preferences)
//   TxplayUI does NOT reference AudioEngine, Analyzer, Library internals.
//
// shutdown_requested is owned by the signal handler in main(); passed in by
// reference so the UI loop can observe it without owning process-level state.
// ---------------------------------------------------------------------------
class TxplayUI {
public:
    TxplayUI(
        txplay::application::Application& app,
        txplay::config::Config&           config,
        volatile std::sig_atomic_t&       shutdown_requested
    );
    ~TxplayUI(); // joins ticker_thread_

    // Run the FTXUI event loop until exit or shutdown_requested is set.
    void run();

private:
    // Build the complete FTXUI component tree and return the root component.
    // Called once from run(). Also starts the ticker thread.
    ftxui::Component build_ui();

    // ---- Backend references ----
    txplay::application::Application& app_;
    txplay::config::Config&           config_;
    volatile std::sig_atomic_t&       shutdown_requested_;

    // ---- UI selection state ----
    int                                  selected_library_{0};
    int                                  selected_queue_{0};
    std::vector<std::string>             library_items_;
    std::vector<std::string>             queue_items_;
    std::vector<txplay::library::Track>  filtered_tracks_;
    std::string                          search_query_;

    // ---- Small-screen state ----
    // Active view in narrow-terminal mode (< 60 cols).
    SmallScreenView small_screen_view_{SmallScreenView::Songs};

    // ---- FTXUI lifecycle ----
    ftxui::ScreenInteractive screen_;
    std::atomic<bool>        keep_running_{true};
    std::thread              ticker_thread_;
};

} // namespace txplay::ui
