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

// TxplayUI owns all FTXUI state, components, rendering, and input handling.
//
// Dependency boundary:
//   TxplayUI → Application (public API only)
//   TxplayUI does NOT include or reference AudioEngine, Analyzer, Library, etc.
//
// The caller (main.cpp) retains ownership of signal handling.
// shutdown_requested is passed in by reference so the UI loop can observe it
// without owning process-level signal state.
class TxplayUI {
public:
    TxplayUI(
        txplay::application::Application& app,
        const txplay::config::VisualizerConfig& vis_config,
        const txplay::config::NavigationConfig& nav_config,
        volatile std::sig_atomic_t& shutdown_requested
    );
    ~TxplayUI(); // joins ticker_thread_

    // Runs the FTXUI event loop until exit or shutdown_requested is set.
    void run();

private:
    // Build the complete FTXUI component tree and return the root component.
    // Called once in run(). Starts the ticker thread.
    ftxui::Component build_ui();

    // --- References ---
    txplay::application::Application& app_;
    txplay::config::VisualizerConfig  vis_config_;
    txplay::config::NavigationConfig  nav_config_;
    volatile std::sig_atomic_t&       shutdown_requested_;

    // --- UI selection state ---
    int                                        selected_library_{0};
    int                                        selected_queue_{0};
    std::vector<std::string>                   library_items_;
    std::vector<std::string>                   queue_items_;
    std::vector<txplay::library::Track>        filtered_tracks_;
    std::string                                search_query_;

    // --- FTXUI lifecycle ---
    ftxui::ScreenInteractive screen_;
    std::atomic<bool>        keep_running_{true};
    std::thread              ticker_thread_;
};

} // namespace txplay::ui
