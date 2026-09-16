#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>
#include <ftxui/screen/color.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <algorithm>
#include <csignal>
#include <cstdio>
#include <fstream>

#include "config/Config.hpp"
#include "application/Application.hpp"
#include "audio/Analyzer.hpp"

using namespace ftxui;
using namespace std::chrono_literals;

// Signal handling
volatile std::sig_atomic_t shutdown_requested = 0;

void signal_handler(int signum) {
    shutdown_requested = 1;
}

// Utility string functions
std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    return s;
}

std::string format_time(uint64_t ms) {
    uint64_t total_seconds = ms / 1000;
    uint64_t hours = total_seconds / 3600;
    uint64_t minutes = (total_seconds % 3600) / 60;
    uint64_t seconds = total_seconds % 60;

    char buf[32];
    if (hours > 0) {
        snprintf(buf, sizeof(buf), "%lu:%02lu:%02lu", hours, minutes, seconds);
    } else {
        snprintf(buf, sizeof(buf), "%02lu:%02lu", minutes, seconds);
    }
    return std::string(buf);
}

Element build_seek_bar(float progress, int width) {
    if (width <= 0) return text("");
    int pos = std::clamp((int)(progress * width), 0, std::max(0, width - 1));
    std::string bar;
    for (int i = 0; i < width; i++) {
        if (i == pos) bar += "●";
        else bar += "─";
    }
    return text(bar);
}

int main() {
    // Register signal handlers
    std::signal(SIGHUP, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT, signal_handler);

    // 1. Config Layer
    std::string config_path = "config.txt";
    if (const char* home = std::getenv("HOME")) {
        std::string user_config = std::string(home) + "/.config/txplay/config.txt";
        // If the user config exists, prefer it over the local one.
        if (std::ifstream(user_config).good()) {
            config_path = user_config;
        }
    }
    txplay::config::Config config(config_path);
    std::vector<std::string> paths = config.get_music_paths();
    auto vis_config = config.get_visualizer();
    auto nav_config = config.get_navigation();
    auto playback_config = config.get_playback();

    // 2. Application Layer
    txplay::application::Application app(paths, playback_config.autoplay);

    // 3. UI State
    auto screen = ScreenInteractive::Fullscreen();
    std::atomic<bool> keep_running{true};

    int selected_library = 0;
    std::vector<std::string> library_items; // FTXUI menu expects vector of strings
    std::vector<txplay::library::Track> filtered_tracks; // The corresponding track models

    int selected_queue = 0;
    std::vector<std::string> queue_items; // rebuilt each frame from app.get_queue()

    std::string search_query;
    auto search_input = Input(&search_query, "Type to search...");

    // Ticker thread to ensure UI refreshes at ~30 FPS for progress bar & visualizer
    std::thread ticker_thread([&]() {
        while (keep_running) {
            std::this_thread::sleep_for(33ms);
            screen.PostEvent(Event::Custom);
        }
    });

    // Lists
    auto library_menu = Menu(&library_items, &selected_library);
    auto queue_menu = Menu(&queue_items, &selected_queue);

    // Wrapper to handle precise click-to-play using FTXUI's internal layout
    auto click_to_play_wrapper = [&](Component child) {
        class Wrapper : public ComponentBase {
            std::function<void()> on_play_;
        public:
            Wrapper(Component child, std::function<void()> on_play) : on_play_(on_play) {
                Add(child);
            }
            bool OnEvent(Event event) override {
                if (event == Event::Return) {
                    on_play_();
                    return true;
                }
                if (event.is_mouse() && event.mouse().button == Mouse::Left && event.mouse().motion == Mouse::Released) {
                    // Convert to Pressed so the underlying Menu evaluates the exact row coordinates
                    Event fake = event;
                    fake.mouse().motion = Mouse::Pressed;
                    bool handled = ComponentBase::OnEvent(fake);
                    if (handled) {
                        on_play_();
                        return true;
                    }
                }
                return ComponentBase::OnEvent(event);
            }
        };
        return Make<Wrapper>(child, [&]() {
            if (selected_library >= 0 && selected_library < filtered_tracks.size()) {
                app.play_track(filtered_tracks[selected_library].id);
            }
        });
    };

    auto wrapped_library_menu = click_to_play_wrapper(library_menu);

    // Layout container capturing focus
    auto container = Container::Vertical({
        search_input,
        Container::Horizontal({wrapped_library_menu, queue_menu})
    });

    auto renderer = Renderer(container, [&] {
        // App update hook
        app.update();

        // Synchronize Queue Data (rebuilt each frame)
        {
            auto q = app.get_queue();
            queue_items.clear();
            if (q.empty()) {
                queue_items.push_back("[ Queue is empty ]");
            } else {
                // Resolve each queue ID to a display title via the current library
                auto all_tracks = app.get_tracks();
                for (const auto& qid : q) {
                    bool found = false;
                    for (const auto& t : all_tracks) {
                        if (t.id == qid) {
                            queue_items.push_back(t.title);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        queue_items.push_back("[missing] " + qid.substr(qid.rfind('/') + 1));
                    }
                }
            }
            if (selected_queue >= (int)queue_items.size()) {
                selected_queue = std::max(0, (int)queue_items.size() - 1);
            }
        }

        auto all_tracks = app.get_tracks();
        filtered_tracks.clear();
        library_items.clear();
        
        std::string query_lower = to_lower(search_query);
        for (const auto& track : all_tracks) {
            if (query_lower.empty() || 
                to_lower(track.title).find(query_lower) != std::string::npos ||
                to_lower(track.artist).find(query_lower) != std::string::npos) {
                
                filtered_tracks.push_back(track);
                library_items.push_back(track.title);
            }
        }
        
        if (app.is_scanning()) {
            library_items.insert(library_items.begin(), "[ Scanning Library... ]");
        } else if (library_items.empty()) {
            library_items.push_back("[ No tracks found ]");
        }

        if (selected_library >= library_items.size()) {
            selected_library = std::max(0, (int)library_items.size() - 1);
        }

        auto term_size = Terminal::Size();
        int width = term_size.dimx;

        // 1. Now playing
        auto current_track = app.get_current_track();
        auto playback_state = app.get_playback_state();
        
        std::string status_str = (playback_state == txplay::audio::PlaybackState::Playing) ? "▶" :
                                 (playback_state == txplay::audio::PlaybackState::Paused)  ? "⏸" : "⏹";
                                 
        std::string state_text = (playback_state == txplay::audio::PlaybackState::Playing) ? "Playing" :
                                 (playback_state == txplay::audio::PlaybackState::Paused)  ? "Paused" : "Stopped";
                                 
        Element now_playing_large;
        Element now_playing_compact;

        if (current_track) {
            uint64_t pos_ms = app.get_position_ms();
            uint64_t dur_ms = app.get_duration_ms();
            if (pos_ms > dur_ms && dur_ms > 0) pos_ms = dur_ms;

            std::string time_str = format_time(pos_ms);
            if (dur_ms > 0) {
                time_str += " / " + format_time(dur_ms);
            }
            
            float progress_val = 0.0f;
            if (dur_ms > 0) {
                progress_val = (float)pos_ms / (float)dur_ms;
            }
            
            int seek_bar_width = std::max(10, width - 4); // inner width

            now_playing_large = vbox({
                text(status_str + " Now Playing: " + current_track->title) | bold | color(Color::Cyan),
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
                text("  Stopped") | color(Color::GrayLight),
                text("")
            }) | border;
            now_playing_compact = text("⏹ Nothing Playing") | bold | color(Color::GrayDark) | border;
        }

        // 2. Visualizer
        Element visualizer = text("");
        if (vis_config.enabled) {
            Elements vis_elements;
            int max_vis_bars = std::max(1, width - 4);
            
            auto analyzer = app.get_analyzer();
            if (analyzer) {
                auto mags = analyzer->get_latest_window();
                int bands = std::min((int)mags.size(), max_vis_bars);
                for(int i = 0; i < bands; i++) {
                    float val = std::min(1.0f, std::abs(mags[i]) * 2.0f);
                    vis_elements.push_back(gaugeUp(val) | color(Color::Green) | flex);
                }
            }
            
            while (vis_elements.size() < max_vis_bars) {
                vis_elements.push_back(gaugeUp(0.0f) | color(Color::Green) | flex);
            }
            
            visualizer = hbox(std::move(vis_elements)) | size(HEIGHT, EQUAL, vis_config.height) | border;
        }

        // 3. Search
        auto search_box = search_input->Render() | border;

        // 4. Lists
        auto library_box = window(text("Library"), wrapped_library_menu->Render() | vscroll_indicator | frame) | flex;
        auto queue_box = window(text("Queue"), queue_menu->Render() | vscroll_indicator | frame) | flex;

        // Responsive Logic
        Element final_layout;
        
        if (width >= 100) {
            final_layout = vbox({ search_box, now_playing_large, visualizer, hbox({library_box, queue_box}) | flex });
        } else if (width >= 60) {
            final_layout = vbox({ search_box, now_playing_compact, visualizer, hbox({library_box, queue_box}) | flex });
        } else {
            final_layout = vbox({ search_box, now_playing_compact, visualizer, library_box });
        }

        return final_layout;
    });

    // Global Keybinds and Focus Manager
    auto global_shortcuts = [&](Component child) {
        class Wrapper : public ComponentBase {
        public:
            txplay::application::Application& app_;
            Component search_;
            Component lib_;
            Component queue_;
            std::atomic<bool>& keep_running_;
            ScreenInteractive& screen_;
            // Configurable queue key characters (single-char string)
            std::string key_queue_add_;
            std::string key_queue_remove_;
            std::string key_queue_clear_;
            // References to UI state needed for queue operations
            int& selected_library_;
            int& selected_queue_;
            std::vector<txplay::library::Track>& filtered_tracks_;

            Wrapper(Component child, txplay::application::Application& app, Component search, Component lib, Component queue,
                    std::atomic<bool>& keep, ScreenInteractive& screen,
                    const txplay::config::NavigationConfig& nav,
                    int& sel_lib, int& sel_queue, std::vector<txplay::library::Track>& filt)
                : app_(app), search_(search), lib_(lib), queue_(queue),
                  keep_running_(keep), screen_(screen),
                  key_queue_add_(nav.queue_add), key_queue_remove_(nav.queue_remove), key_queue_clear_(nav.queue_clear),
                  selected_library_(sel_lib), selected_queue_(sel_queue), filtered_tracks_(filt)
            {
                Add(child);
            }

            bool OnEvent(Event event) override {
                // Focus Management — Tab cycles only between Library and Queue.
                // Search is exclusively triggered by '/'.
                if (event == Event::Tab || event == Event::TabReverse) {
                    std::vector<Component> focusables = { lib_, queue_ };
                    int current = 0;
                    for (int i = 0; i < (int)focusables.size(); i++) {
                        if (focusables[i]->Focused()) { current = i; break; }
                    }
                    if (event == Event::Tab) {
                        current = (current + 1) % focusables.size();
                    } else {
                        current = (current - 1 + focusables.size()) % focusables.size();
                    }
                    focusables[current]->TakeFocus();
                    return true;
                }

                // Seek Intercept (Intercept BEFORE so Container::Horizontal doesn't steal Left/Right for focus)
                if (!search_->Focused()) {
                    if (event == Event::ArrowRight) {
                        app_.seek(app_.get_position_ms() + 5000);
                        return true;
                    }
                    if (event == Event::ArrowLeft) {
                        uint64_t pos = app_.get_position_ms();
                        app_.seek(pos > 5000 ? pos - 5000 : 0);
                        return true;
                    }
                }

                // 1. Let focused component handle event natively (text input, menu navigation, etc.)
                if (ComponentBase::OnEvent(event)) {
                    return true;
                }

                // 2. Global Shortcuts (only if not consumed by the focused widget)
                if (event == Event::Character('q') || event == Event::Escape) {
                    keep_running_ = false;
                    screen_.Exit();
                    return true;
                }
                if (event == Event::Character('p') || event == Event::Character(' ')) {
                    app_.toggle_pause();
                    return true;
                }
                if (event == Event::Character('/')) {
                    search_->TakeFocus();
                    return true;
                }
                if (event == Event::Character('r')) {
                    app_.rescan_library();
                    return true;
                }

                // Queue shortcuts — active when search is not focused
                if (!search_->Focused()) {
                    // queue_add: add selected Library track to Queue
                    if (key_queue_add_.size() == 1 && event == Event::Character(key_queue_add_[0])) {
                        if (selected_library_ >= 0 && selected_library_ < (int)filtered_tracks_.size()) {
                            app_.queue_add(filtered_tracks_[selected_library_].id);
                        }
                        return true;
                    }
                    // queue_remove: remove selected Queue entry
                    if (key_queue_remove_.size() == 1 && event == Event::Character(key_queue_remove_[0])) {
                        if (queue_->Focused()) {
                            app_.queue_remove(selected_queue_);
                        }
                        return true;
                    }
                    // queue_clear: clear entire Queue
                    if (key_queue_clear_.size() == 1 && event == Event::Character(key_queue_clear_[0])) {
                        if (queue_->Focused()) {
                            app_.queue_clear();
                        }
                        return true;
                    }
                }

                return false;
            }
        };
        return Make<Wrapper>(child, app, search_input, wrapped_library_menu, queue_menu,
                             keep_running, screen, nav_config,
                             selected_library, selected_queue, filtered_tracks);
    };

    auto final_app = global_shortcuts(renderer);

    // Custom Loop Integration
    Loop loop(&screen, final_app);
    while (!loop.HasQuitted() && keep_running) {
        if (shutdown_requested) {
            keep_running = false;
            screen.Exit();
            break;
        }
        loop.RunOnce();
    }

    keep_running = false;

    if (ticker_thread.joinable()) {
        ticker_thread.join();
    }

    return 0;
}
