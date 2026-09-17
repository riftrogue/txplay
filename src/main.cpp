#include <csignal>

#include "config/Config.hpp"
#include "application/Application.hpp"
#include "ui/TxplayUI.hpp"

// ---------------------------------------------------------------------------
// Process-level signal handling.
// The flag is passed by reference into TxplayUI so the UI loop can observe it
// without owning process-level signal state.
// ---------------------------------------------------------------------------
volatile std::sig_atomic_t shutdown_requested = 0;

static void signal_handler(int /*signum*/) {
    shutdown_requested = 1;
}

int main() {
    std::signal(SIGHUP,  signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT,  signal_handler);

    txplay::config::Config config(
        txplay::config::Config::resolve_user_config_path()
    );

    txplay::application::Application app(config);

    txplay::ui::TxplayUI ui(app, config, shutdown_requested);

    ui.run();
    return 0;
}
