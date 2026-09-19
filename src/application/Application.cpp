#include "Application.hpp"
#include "audio/Analyzer.hpp"

#include <algorithm>

namespace txplay::application {

Application::Application(txplay::config::Config& config)
    : config_(config)
{
    // Start initial scan using paths from config
    library_.scan_async(config_.library().music_paths);
    state_ = ApplicationState::Ready;
}

Application::~Application() {
    // Stop playback explicitly before engines are destroyed by RAII
    audio_engine_.stop();
}

// ---------------------------------------------------------------------------
// play_track() — manual track selection
// ---------------------------------------------------------------------------
bool Application::play_track(const std::string& track_id) {
    // Manual selection resets the autoplay session counter so the limit
    // applies freshly to any subsequent autoplay sequence.
    reset_autoplay_counter();

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
    //    Record the previous track before overwriting current so play_previous()
    //    can return to it.
    previous_track_id_ = current_track_id_;
    current_track_id_  = track_id;
    eof_processed_ = false;
    status_message_ = "Playing: " + target_track->title;
    return true;
}

// ---------------------------------------------------------------------------
// play_next() — explicit user "next" command.
//
// Decision order:
//   1. Queue has items → consume the front entry (same as advance_queue, but
//      triggered manually rather than on EOF).
//   2. Queue empty, shuffle OFF → next track in library order after current.
//   3. Queue empty, shuffle ON → [future: random library track; not yet built]
//
// This is always an explicit user action. Autoplay settings are irrelevant.
// Resets the autoplay counter so a subsequent auto-advance starts fresh.
// ---------------------------------------------------------------------------
void Application::play_next() {
    // Step 1: consume the queue front (if any).
    if (!queue_.empty()) {
        // Reuse advance_queue() mechanics: pop-and-play, skip invalid entries.
        if (advance_queue()) {
            reset_autoplay_counter(); // manual skip resets autoplay context
            return;
        }
        // Queue was non-empty but all entries were invalid — fall through.
    }

    // Step 2: no usable queue entry — advance in library order.
    // Shuffle is not yet implemented; the library-order path is always used.
    auto tracks = library_.get_tracks();
    if (tracks.empty()) return;

    // Find the current track's position in the library.
    int current_index = -1;
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
        if (tracks[i].id == current_track_id_) {
            current_index = i;
            break;
        }
    }

    // If no current track, start from the first library track.
    int next_index = (current_index < 0) ? 0 : current_index + 1;

    if (next_index >= static_cast<int>(tracks.size())) {
        // Already at the end of the library — do nothing.
        return;
    }

    const auto& next_track = tracks[next_index];
    if (!audio_engine_.play(next_track.path)) {
        status_message_ = "Error: Failed to play: " + next_track.path;
        return;
    }

    previous_track_id_ = current_track_id_;
    current_track_id_  = next_track.id;
    eof_processed_    = false;
    reset_autoplay_counter();
    status_message_   = "Playing: " + next_track.title;
}

// ---------------------------------------------------------------------------
// play_previous() — explicit user "previous" command.
//
// Returns to the immediately previous track using the one-step history slot
// (previous_track_id_).  The slot is CONSUMED on use so pressing b twice does
// nothing (no C→B→C toggle).
//
// The queue is a forward-only FIFO; there is no "previous queue item".
// This implementation is deliberately source-agnostic: it does not care
// whether the previous track came from the library, the queue, or future
// shuffle playback.
// ---------------------------------------------------------------------------
void Application::play_previous() {
    if (previous_track_id_.empty()) return; // no history — do nothing

    // Locate the previous track in the library.
    std::optional<library::Track> prev_track = std::nullopt;
    for (const auto& t : library_.get_tracks()) {
        if (t.id == previous_track_id_) { prev_track = t; break; }
    }

    if (!prev_track) {
        // Track was deleted from library since it last played — discard history.
        previous_track_id_.clear();
        return;
    }

    if (!audio_engine_.play(prev_track->path)) {
        status_message_ = "Error: Failed to play: " + prev_track->path;
        return;
    }

    // IMPORTANT: do NOT write previous_track_id_ = current_track_id_ here.
    // Doing so would allow C→B→C→B toggling.  Instead, consume the slot.
    current_track_id_  = previous_track_id_;
    previous_track_id_ = "";   // slot consumed
    eof_processed_     = false;
    reset_autoplay_counter();
    status_message_    = "Playing: " + prev_track->title;
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
    // Reads current paths from config_ so Settings-added paths are included.
    if (!library_.scan_async(config_.library().music_paths)) {
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
// Private helpers
// ---------------------------------------------------------------------------

// reset_autoplay_counter() — clears the autoplay session state.
// Called on every manual play_track() invocation so that a new autoplay
// sequence that starts after that track counts from zero.
void Application::reset_autoplay_counter() {
    autoplay_tracks_played_ = 0;
    autoplay_local_index_   = -1;
}

// advance_queue() — pops and plays the next valid FIFO queue entry.
// Skips entries that are no longer in the library (stale after a rescan).
// Does NOT call reset_autoplay_counter(); queue advancement is not manual.
// Returns true if a track was successfully started; false if queue exhausted.
bool Application::advance_queue() {
    while (!queue_.empty()) {
        std::string next_id = queue_.front();
        queue_.pop_front();

        // Locate the track without going through play_track() to avoid
        // resetting the autoplay counter mid-queue.
        std::optional<library::Track> target = std::nullopt;
        for (const auto& t : library_.get_tracks()) {
            if (t.id == next_id) { target = t; break; }
        }

        if (!target) {
            status_message_ = "Skipping missing track: " + next_id;
            continue;
        }

        if (!audio_engine_.play(target->path)) {
            status_message_ = "Error: Failed to play queued track: " + target->path;
            continue;
        }

        previous_track_id_ = current_track_id_;
        current_track_id_  = next_id;
        eof_processed_     = false;
        status_message_    = "Playing: " + target->title;
        return true;
    }
    return false;
}

// advance_autoplay() — selects the next local track in library order.
// Called only when the queue is empty and autoplay=true.
// Uses autoplay_local_index_ to maintain position across calls.
// Increments autoplay_tracks_played_ for each track started here.
// Returns true if a track was started; false if library exhausted or limit reached.
bool Application::advance_autoplay() {
    const auto& pb = config_.playback();

    // Check limit before selecting a track.
    if (pb.autoplay_limit &&
        autoplay_tracks_played_ >= static_cast<std::size_t>(pb.autoplay_limit_value)) {
        status_message_ = "Autoplay limit reached.";
        return false;
    }

    auto tracks = library_.get_tracks();
    if (tracks.empty()) return false;

    // Determine starting index: find current track in the library, then
    // advance by one. If not found, start from index 0.
    if (autoplay_local_index_ < 0) {
        // Establish position from current_track_id_ if available.
        autoplay_local_index_ = 0;
        for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
            if (tracks[i].id == current_track_id_) {
                autoplay_local_index_ = i;
                break;
            }
        }
    }

    // Pick next track (wrap around if at end — optional; here we stop at end).
    int next_index = autoplay_local_index_ + 1;
    if (next_index >= static_cast<int>(tracks.size())) {
        // Reached end of library; stop autoplay.
        return false;
    }

    const auto& next_track = tracks[next_index];

    if (!audio_engine_.play(next_track.path)) {
        status_message_ = "Error: Failed to auto-play: " + next_track.path;
        return false;
    }

    previous_track_id_        = current_track_id_;
    current_track_id_         = next_track.id;
    autoplay_local_index_     = next_index;
    eof_processed_            = false;
    ++autoplay_tracks_played_;
    status_message_           = "Playing: " + next_track.title;
    return true;
}

// ---------------------------------------------------------------------------
// update() — called once per frame (~30 FPS) by the UI loop
// ---------------------------------------------------------------------------
// EOF decision order:
//   1. Queue has a track? → advance_queue() (always, regardless of autoplay)
//   2. Queue empty + autoplay=true? → advance_autoplay() (respects limit)
//   3. Otherwise → stop cleanly
void Application::update() {
    // Guard: only act on EOF once per track lifetime.
    // eof_processed_ is reset in play_track()/advance_queue()/advance_autoplay()
    // whenever a new track starts.
    if (eof_processed_) return;

    if (!audio_engine_.is_track_finished()) return;

    // Mark processed immediately to prevent re-entry across frames.
    eof_processed_ = true;

    // Step 1: queue always has priority.
    if (advance_queue()) return;

    // Step 2: autoplay from local library.
    if (config_.playback().autoplay) {
        if (advance_autoplay()) return;
    }

    // Step 3: nothing to play — stop cleanly.
    audio_engine_.stop();
    current_track_id_.clear();
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

std::vector<float> Application::get_visualizer_magnitudes() const {
    auto analyzer = audio_engine_.get_analyzer();
    if (!analyzer) return {};
    return analyzer->get_latest_window();
}

} // namespace txplay::application
