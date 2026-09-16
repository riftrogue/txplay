#pragma once

#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <memory>

#include "../library/Library.hpp"
#include "../audio/AudioEngine.hpp"

namespace txplay::application {

enum class ApplicationState {
    Initializing,
    Ready,
    Error
};

class Application {
public:
    Application(const std::vector<std::string>& initial_music_paths, bool autoplay = false);
    ~Application();

    // Prevent copying
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Commands
    bool play_track(const std::string& track_id);
    void toggle_pause();
    void seek(uint64_t ms);
    void set_volume(float volume);
    void rescan_library();

    // Queue management
    void queue_add(const std::string& track_id);
    void queue_remove(size_t index);
    void queue_clear();
    std::deque<std::string> get_queue() const;
    bool queue_is_empty() const;

    // Called once per frame by the UI loop
    void update();

    // Status / Errors
    std::string get_status_message() const;
    void clear_status_message();

    // Queries
    ApplicationState get_state() const;
    std::optional<library::Track> get_current_track() const;
    txplay::audio::PlaybackState get_playback_state() const;
    uint64_t get_position_ms() const;
    uint64_t get_duration_ms() const;

    // Direct read-only library access for UI
    bool is_scanning() const;
    std::vector<library::Track> get_tracks() const;
    std::vector<std::string> get_library_errors() const;

    // Audio Analyzer access for UI
    std::shared_ptr<audio::Analyzer> get_analyzer() const;

private:
    // Advance autoplay: attempt to play next valid track from queue.
    // Skips missing/invalid tracks. Returns true if a track was started.
    bool advance_queue();

    ApplicationState state_{ApplicationState::Initializing};
    std::string status_message_;
    std::string current_track_id_;
    std::vector<std::string> config_paths_;

    // Queue: FIFO of canonical track IDs waiting to play.
    std::deque<std::string> queue_;

    // Autoplay flag: read from config, never modified at runtime.
    bool autoplay_{false};

    // Tracks whether the EOF transition for the current track has already been
    // processed. Reset to false every time a new track begins playing (whether
    // manually or via autoplay). Prevents update() from triggering multiple
    // transitions across frames while is_track_finished() remains true.
    bool eof_processed_{false};

    // The order of these declarations is important for clean shutdown RAII.
    // AudioEngine must be stopped and destroyed before Library.
    library::Library library_;
    audio::AudioEngine audio_engine_;
};

} // namespace txplay::application
