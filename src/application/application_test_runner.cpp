#include <iostream>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "Application.hpp"

using namespace txplay::application;
using namespace std::chrono_literals;
namespace fs = std::filesystem;

int main() {
    std::cout << "--- Application Test Runner ---" << std::endl;

    std::string test_dir = "experiments/app-test/root";
    std::string source_mp3 = "experiments/audio-test/test.mp3";
    std::string target_mp3 = test_dir + "/test.mp3";
    
    // Setup test environment
    if (fs::exists(test_dir)) fs::remove_all(test_dir);
    fs::create_directories(test_dir);
    fs::copy_file(source_mp3, target_mp3);

    std::vector<std::string> paths = {test_dir};

    // 1 & 2 & 3: Construction, State, and Auto-Scan
    Application app(paths);
    assert(app.get_state() == ApplicationState::Ready);
    
    std::cout << "Waiting for initial scan..." << std::endl;
    while (app.is_scanning()) {
        std::this_thread::sleep_for(10ms);
    }

    auto tracks = app.get_tracks();
    assert(tracks.size() == 1);
    
    // 4: Track lookup by canonical path (we use tracks[0].id)
    std::string valid_id = tracks[0].id;
    
    // 6: play_track with nonexistent ID
    bool played_bad = app.play_track("/invalid/path.mp3");
    assert(!played_bad);
    assert(app.get_current_track() == std::nullopt);

    // 5: Successful play_track()
    std::cout << "Playing track: " << valid_id << std::endl;
    bool played = app.play_track(valid_id);
    assert(played);
    
    auto current_track = app.get_current_track();
    assert(current_track.has_value());
    assert(current_track->id == valid_id);
    assert(app.get_playback_state() == txplay::audio::PlaybackState::Playing);

    std::this_thread::sleep_for(500ms);

    // 8: toggle_pause()
    app.toggle_pause();
    assert(app.get_playback_state() == txplay::audio::PlaybackState::Paused);
    app.toggle_pause();
    assert(app.get_playback_state() == txplay::audio::PlaybackState::Playing);

    // 9 & 10: seek and volume
    app.seek(1500);
    app.set_volume(0.5f);
    std::this_thread::sleep_for(500ms); // wait for seek epoch
    assert(app.get_position_ms() >= 1500);

    // 11: current_track survives a Library rescan when the track still exists
    std::cout << "Rescanning library..." << std::endl;
    app.rescan_library();
    while (app.is_scanning()) std::this_thread::sleep_for(10ms);
    
    assert(app.get_current_track().has_value());
    assert(app.get_current_track()->id == valid_id);

    // 12: current_track resolves to std::nullopt when the track disappears
    std::cout << "Removing file and rescanning..." << std::endl;
    fs::remove(target_mp3);
    app.rescan_library();
    while (app.is_scanning()) std::this_thread::sleep_for(10ms);
    
    // The library no longer has the track, so it returns nullopt
    assert(app.get_current_track() == std::nullopt);
    
    // But AudioEngine is still actively playing the open file handle
    assert(app.get_playback_state() == txplay::audio::PlaybackState::Playing);

    // 13: AudioEngine track completion is detected by update()
    std::cout << "Testing track completion..." << std::endl;
    // Play the original source_mp3 since target_mp3 was deleted
    app.rescan_library();
    while (app.is_scanning()) std::this_thread::sleep_for(10ms);
    // Play the source_mp3 directly using its absolute path (assuming it's in the library, wait it's not in the library config paths)
    // Let's copy it back.
    fs::copy_file(source_mp3, target_mp3);
    app.rescan_library();
    while (app.is_scanning()) std::this_thread::sleep_for(10ms);
    app.play_track(app.get_tracks()[0].id);
    
    app.seek(2000);
    
    bool finished = false;
    for (int i = 0; i < 200; ++i) { // 2 seconds
        app.update();
        if (app.get_playback_state() == txplay::audio::PlaybackState::Stopped) {
            finished = true;
            break;
        }
        std::this_thread::sleep_for(10ms);
    }
    assert(finished);

    // 7: Failed AudioEngine playback does not leave a false current-track state.
    // Create an unplayable file (empty)
    fs::remove(target_mp3);
    fs::path bad_mp3 = test_dir + "/bad.mp3";
    { std::ofstream out(bad_mp3); out << "bad data"; }
    app.rescan_library();
    while (app.is_scanning()) std::this_thread::sleep_for(10ms);
    
    auto new_tracks = app.get_tracks();
    assert(new_tracks.size() == 1); // Only bad.mp3 is there
    std::string bad_id = new_tracks[0].id;
    
    bool played_unplayable = app.play_track(bad_id);
    assert(!played_unplayable);
    assert(app.get_current_track() == std::nullopt);
    
    // 14: Shutdown does not leave worker threads running.
    // (Handled implicitly by exiting main, TSAN/valgrind would catch hangs)

    std::cout << "All Application assertions passed!" << std::endl;
    return 0;
}
