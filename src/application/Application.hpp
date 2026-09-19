#pragma once

#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <memory>

#include "audio/PlaybackState.hpp"
#include "config/Config.hpp"
#include "library/Library.hpp"
#include "audio/AudioEngine.hpp"

namespace txplay::application {

enum class ApplicationState {
    Initializing,
    Ready,
    Error
};

class Application {
public:
    explicit Application(txplay::config::Config& config);
    ~Application();

    // Prevent copying
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Commands
    bool play_track(const std::string& track_id);
    void play_next();     // explicit user command: queue front → library next → (shuffle: future)
    void play_previous(); // explicit user command: library previous (no queue history)
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

    // Visualizer data for UI — returns latest FFT magnitude window.
    // Returns an empty vector when nothing is playing or analyzer is unavailable.
    // The UI should NOT access the Analyzer directly.
    std::vector<float> get_visualizer_magnitudes() const;

private:
    // Advance queue: pop and play the next valid FIFO queue entry.
    // Skips entries that are missing from the library. Returns true if started.
    bool advance_queue();

    // Advance autoplay: pick the next local track in library order when the
    // queue is empty and autoplay=true. Increments autoplay_tracks_played_.
    // Returns true if a track was started.
    bool advance_autoplay();

    // Reset autoplay session state. Called on manual play_track() so that
    // the counter and position restart for the new playback context.
    void reset_autoplay_counter();

    ApplicationState state_{ApplicationState::Initializing};
    std::string status_message_;
    std::string current_track_id_;

    // Queue: explicit FIFO of canonical track IDs with playback priority.
    // Queue is checked before autoplay on every EOF event.
    std::deque<std::string> queue_;

    // One-step playback history: the canonical ID of the track that was playing
    // immediately before the current track started.  Empty string means there
    // is no previous track.  Populated on every track transition; consumed
    // (cleared) by play_previous() so that pressing b twice does nothing.
    std::string previous_track_id_;

    // Tracks whether the EOF transition for the current track has already been
    // processed. Reset to false every time a new track begins playing (whether
    // manually or via queue/autoplay). Prevents update() from triggering
    // multiple transitions across frames.
    bool eof_processed_{false};

    // ---- Autoplay runtime state (session-only; never persisted) ----
    // Index into the library track vector for sequential local autoplay.
    // Points to the current track so advance_autoplay() picks track+1.
    // -1 means no autoplay context is established yet.
    int autoplay_local_index_{-1};

    // Count of local tracks automatically selected in the current autoplay
    // sequence. Counts only advance_autoplay() selections; queue tracks and
    // manual selections do not count and do not increment this value.
    std::size_t autoplay_tracks_played_{0};

    // Config reference: single runtime source of truth for user preferences.
    // Lifetime: Config outlives Application (both on main() stack).
    txplay::config::Config& config_;

    // The order of these declarations is important for clean shutdown RAII.
    // AudioEngine must be stopped and destroyed before Library.
    library::Library library_;
    audio::AudioEngine audio_engine_;
};

} // namespace txplay::application
