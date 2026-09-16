#include "Application.hpp"

namespace txplay::application {

Application::Application(const std::vector<std::string>& initial_music_paths, bool autoplay)
    : config_paths_(initial_music_paths), autoplay_(autoplay)
{
    // Start initial scan
    library_.scan_async(config_paths_);
    state_ = ApplicationState::Ready;
}

Application::~Application() {
    // Stop playback explicitly before engines are destroyed by RAII
    audio_engine_.stop();
}

bool Application::play_track(const std::string& track_id) {
    // 1. Find the Track in Library by its canonical path/ID.
    std::optional<library::Track> target_track = std::nullopt;
    for (const auto& track : library_.get_tracks()) {
        if (track.id == track_id) {
            target_track = track;
            break;
        }
    }

    // 2. If it doesn't exist
    if (!target_track) {
        status_message_ = "Error: Track not found in library: " + track_id;
        return false;
    }

    // 3. Call AudioEngine.play
    if (!audio_engine_.play(target_track->path)) {
        status_message_ = "Error: Failed to play track: " + target_track->path;
        current_track_id_.clear();
        return false;
    }

    // 4. Update current_track_id and reset EOF guard on every successful start.
    //    This covers both manual playback and autoplay advancement.
    current_track_id_ = track_id;
    eof_processed_ = false;
    status_message_ = "Playing: " + target_track->title;
    return true;
}

void Application::toggle_pause() {
    auto state = audio_engine_.get_state();
    if (state == audio::PlaybackState::Playing) {
        audio_engine_.pause();
    } else if (state == audio::PlaybackState::Paused) {
        audio_engine_.resume();
    }
}

void Application::seek(uint64_t ms) {
    audio_engine_.seek(ms);
}

void Application::set_volume(float volume) {
    audio_engine_.set_volume(volume);
}

void Application::rescan_library() {
    if (!library_.scan_async(config_paths_)) {
        status_message_ = "Scan already in progress.";
    } else {
        status_message_ = "Scanning library...";
    }
}

// ---------------------------------------------------------------------------
// Queue API
// ---------------------------------------------------------------------------

void Application::queue_add(const std::string& track_id) {
    queue_.push_back(track_id);
}

void Application::queue_remove(size_t index) {
    if (index < queue_.size()) {
        queue_.erase(queue_.begin() + index);
    }
}

void Application::queue_clear() {
    queue_.clear();
}

std::deque<std::string> Application::get_queue() const {
    return queue_;
}

bool Application::queue_is_empty() const {
    return queue_.empty();
}

// ---------------------------------------------------------------------------
// Private helper: advance_queue
// ---------------------------------------------------------------------------
// Pops entries from the front of the queue, skipping any track IDs that are
// no longer resolvable in the Library (e.g., deleted from disk after a rescan).
// Returns true if a track was successfully started; false if queue exhausted.
bool Application::advance_queue() {
    while (!queue_.empty()) {
        std::string next_id = queue_.front();
        queue_.pop_front();

        if (play_track(next_id)) {
            return true;
        }
        // play_track already updated status_message_ with the error;
        // we silently continue to the next entry.
    }
    return false;
}

// ---------------------------------------------------------------------------
// update() — called once per frame (~30 FPS) by the UI loop
// ---------------------------------------------------------------------------
void Application::update() {
    // Guard: only act on EOF once per track lifetime.
    // eof_processed_ is reset in play_track() whenever a new track starts.
    if (eof_processed_) return;

    if (!audio_engine_.is_track_finished()) return;

    // Mark as processed immediately to prevent re-entry across frames.
    eof_processed_ = true;

    // The queue always advances on EOF, regardless of the autoplay setting.
    // If the queue is empty and autoplay is off, stop cleanly.
    // If the queue is empty and autoplay is on, stop cleanly (future: repeat/shuffle).
    if (!advance_queue()) {
        audio_engine_.stop();
        current_track_id_.clear();
    }
}


// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

std::string Application::get_status_message() const {
    return status_message_;
}

void Application::clear_status_message() {
    status_message_.clear();
}

ApplicationState Application::get_state() const {
    return state_;
}

std::optional<library::Track> Application::get_current_track() const {
    if (current_track_id_.empty()) {
        return std::nullopt;
    }

    // Resolve against the active library vector
    auto tracks = library_.get_tracks();
    for (const auto& track : tracks) {
        if (track.id == current_track_id_) {
            return track;
        }
    }

    // Track disappeared from library (deleted + rescanned)
    return std::nullopt;
}

audio::PlaybackState Application::get_playback_state() const {
    return audio_engine_.get_state();
}

uint64_t Application::get_position_ms() const {
    return audio_engine_.get_position_ms();
}

uint64_t Application::get_duration_ms() const {
    return audio_engine_.get_duration_ms();
}

bool Application::is_scanning() const {
    return library_.is_scanning();
}

std::vector<library::Track> Application::get_tracks() const {
    return library_.get_tracks();
}

std::vector<std::string> Application::get_library_errors() const {
    return library_.get_last_errors();
}

std::shared_ptr<audio::Analyzer> Application::get_analyzer() const {
    return audio_engine_.get_analyzer();
}

} // namespace txplay::application
