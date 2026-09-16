#pragma once

#include <string>
#include <vector>
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
    Application(const std::vector<std::string>& initial_music_paths);
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
    ApplicationState state_{ApplicationState::Initializing};
    std::string status_message_;
    std::string current_track_id_;
    std::vector<std::string> config_paths_;

    // The order of these declarations is important for clean shutdown RAII.
    // AudioEngine must be stopped and destroyed before Library.
    library::Library library_;
    audio::AudioEngine audio_engine_;
};

} // namespace txplay::application
