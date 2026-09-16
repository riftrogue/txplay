#include <iostream>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "Application.hpp"

using namespace txplay::application;
using namespace std::chrono_literals;
namespace fs = std::filesystem;

// Helper: wait for app to finish scanning
static void wait_scan(Application& app) {
    while (app.is_scanning()) std::this_thread::sleep_for(10ms);
}

// Helper: drive update() for up to N * 10ms, return true when Stopped
static bool wait_stopped(Application& app, int iterations = 400) {
    for (int i = 0; i < iterations; ++i) {
        app.update();
        if (app.get_playback_state() == txplay::audio::PlaybackState::Stopped) return true;
        std::this_thread::sleep_for(10ms);
    }
    return false;
}

int main() {
    std::cout << "--- Application Test Runner ---" << std::endl;

    std::string test_dir = "experiments/app-test/root";
    std::string source_mp3 = "experiments/audio-test/test.mp3";
    std::string target_mp3  = test_dir + "/test.mp3";
    std::string target_mp3b = test_dir + "/test_b.mp3";

    // Setup test environment
    if (fs::exists(test_dir)) fs::remove_all(test_dir);
    fs::create_directories(test_dir);
    fs::copy_file(source_mp3, target_mp3);
    fs::copy_file(source_mp3, target_mp3b);

    std::vector<std::string> paths = {test_dir};

    // =========================================================================
    // 1-3. Construction, State, and Auto-Scan
    // =========================================================================
    Application app(paths);
    assert(app.get_state() == ApplicationState::Ready);

    std::cout << "Waiting for initial scan..." << std::endl;
    wait_scan(app);

    auto tracks = app.get_tracks();
    assert(tracks.size() == 2);

    // Sort by filename for stable ordering in tests
    std::string id_a = (tracks[0].filename < tracks[1].filename) ? tracks[0].id : tracks[1].id;
    std::string id_b = (tracks[0].filename < tracks[1].filename) ? tracks[1].id : tracks[0].id;

    // =========================================================================
    // 4. play_track with nonexistent ID fails cleanly
    // =========================================================================
    bool played_bad = app.play_track("/invalid/path.mp3");
    assert(!played_bad);
    assert(app.get_current_track() == std::nullopt);

    // =========================================================================
    // 5. Successful play_track
    // =========================================================================
    std::cout << "Playing track A: " << id_a << std::endl;
    bool played = app.play_track(id_a);
    assert(played);
    assert(app.get_current_track().has_value());
    assert(app.get_current_track()->id == id_a);
    assert(app.get_playback_state() == txplay::audio::PlaybackState::Playing);
    std::this_thread::sleep_for(200ms);

    // =========================================================================
    // 6. toggle_pause
    // =========================================================================
    app.toggle_pause();
    assert(app.get_playback_state() == txplay::audio::PlaybackState::Paused);
    app.toggle_pause();
    assert(app.get_playback_state() == txplay::audio::PlaybackState::Playing);

    // =========================================================================
    // 7. seek and volume
    // =========================================================================
    app.seek(1500);
    app.set_volume(0.5f);
    std::this_thread::sleep_for(500ms);
    assert(app.get_position_ms() >= 1500);

    // =========================================================================
    // QUEUE TESTS — begin
    // =========================================================================
    std::cout << "\n--- Queue Tests ---" << std::endl;

    // Q1: Queue API basics (add/next/empty)
    {
        std::cout << "Q1: basic FIFO add/get..." << std::endl;
        Application q_app(paths);
        wait_scan(q_app);

        assert(q_app.queue_is_empty());
        q_app.queue_add("B");
        q_app.queue_add("C");
        q_app.queue_add("D");
        assert(!q_app.queue_is_empty());

        auto q = q_app.get_queue();
        assert(q.size() == 3);
        assert(q[0] == "B");
        assert(q[1] == "C");
        assert(q[2] == "D");
        std::cout << "  Q1 passed." << std::endl;
    }

    // Q2: queue_remove
    {
        std::cout << "Q2: queue_remove..." << std::endl;
        Application q_app(paths);
        wait_scan(q_app);

        q_app.queue_add("B");
        q_app.queue_add("C");
        q_app.queue_add("D");
        q_app.queue_remove(1); // remove C (index 1)

        auto q = q_app.get_queue();
        assert(q.size() == 2);
        assert(q[0] == "B");
        assert(q[1] == "D");
        std::cout << "  Q2 passed." << std::endl;
    }

    // Q3: queue_clear
    {
        std::cout << "Q3: queue_clear..." << std::endl;
        Application q_app(paths);
        wait_scan(q_app);

        q_app.queue_add("B");
        q_app.queue_add("C");
        q_app.queue_clear();
        assert(q_app.queue_is_empty());
        std::cout << "  Q3 passed." << std::endl;
    }

    // Q4: Manual playback independence — queue must NOT change
    {
        std::cout << "Q4: manual play does not modify queue..." << std::endl;
        Application q_app(paths);
        wait_scan(q_app);
        auto t = q_app.get_tracks();
        assert(t.size() == 2);

        q_app.queue_add(t[0].id);
        q_app.queue_add(t[1].id);
        std::string before_q0 = q_app.get_queue()[0];
        std::string before_q1 = q_app.get_queue()[1];

        // Manually play a track — queue must remain identical
        q_app.play_track(t[0].id);
        auto q = q_app.get_queue();
        assert(q.size() == 2);
        assert(q[0] == before_q0);
        assert(q[1] == before_q1);
        std::cout << "  Q4 passed." << std::endl;
    }

    // Q5: queue always advances on EOF, even when autoplay=false
    {
        std::cout << "Q5: queue advances on EOF regardless of autoplay flag..." << std::endl;
        Application q_app(paths, /*autoplay=*/false);
        wait_scan(q_app);
        auto t = q_app.get_tracks();

        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        q_app.queue_add(second);
        q_app.play_track(first);
        q_app.seek(999999);

        // Queue must still advance to second even though autoplay=false
        bool advanced = false;
        for (int i = 0; i < 400; ++i) {
            q_app.update();
            auto ct = q_app.get_current_track();
            if (ct.has_value() && ct->id == second) {
                advanced = true;
                break;
            }
            std::this_thread::sleep_for(10ms);
        }
        assert(advanced);
        assert(q_app.queue_is_empty());
        std::cout << "  Q5 passed." << std::endl;
    }


    // Q6: autoplay=true — EOF consumes next track from queue
    {
        std::cout << "Q6: autoplay=true — EOF advances queue..." << std::endl;
        Application q_app(paths, /*autoplay=*/true);
        wait_scan(q_app);
        auto t = q_app.get_tracks();

        // Sort for stability
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        q_app.queue_add(second);

        q_app.play_track(first);
        q_app.seek(999999); // race to EOF

        // Wait for autoplay to start the next track
        bool advanced = false;
        for (int i = 0; i < 400; ++i) {
            q_app.update();
            auto ct = q_app.get_current_track();
            if (ct.has_value() && ct->id == second) {
                advanced = true;
                break;
            }
            std::this_thread::sleep_for(10ms);
        }
        assert(advanced);
        assert(q_app.queue_is_empty()); // queue was consumed
        std::cout << "  Q6 passed." << std::endl;
    }

    // Q7: autoplay=true, empty queue — EOF stops cleanly
    {
        std::cout << "Q7: autoplay=true, empty queue — stops cleanly..." << std::endl;
        Application q_app(paths, /*autoplay=*/true);
        wait_scan(q_app);
        auto t = q_app.get_tracks();

        q_app.play_track(t[0].id);
        q_app.seek(999999);

        bool stopped = wait_stopped(q_app, 400);
        assert(stopped);
        assert(q_app.queue_is_empty());
        std::cout << "  Q7 passed." << std::endl;
    }

    // Q8: autoplay=true, invalid queued track — skipped safely
    {
        std::cout << "Q8: invalid queued track skipped safely..." << std::endl;
        Application q_app(paths, /*autoplay=*/true);
        wait_scan(q_app);
        auto t = q_app.get_tracks();

        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        // Push a totally invalid ID first — it should be skipped
        q_app.queue_add("/nonexistent/track.mp3");
        q_app.queue_add(second);

        q_app.play_track(first);
        q_app.seek(999999);

        bool advanced = false;
        for (int i = 0; i < 400; ++i) {
            q_app.update();
            auto ct = q_app.get_current_track();
            if (ct.has_value() && ct->id == second) {
                advanced = true;
                break;
            }
            std::this_thread::sleep_for(10ms);
        }
        assert(advanced);
        assert(q_app.queue_is_empty());
        std::cout << "  Q8 passed." << std::endl;
    }

    // =========================================================================
    // 11. current_track survives a Library rescan when track still exists
    // =========================================================================
    std::cout << "\n--- Legacy Application Tests ---" << std::endl;
    {
        Application r_app(paths);
        wait_scan(r_app);
        auto t = r_app.get_tracks();
        r_app.play_track(t[0].id);
        r_app.rescan_library();
        wait_scan(r_app);
        assert(r_app.get_current_track().has_value());
    }
    std::cout << "  Legacy rescan test passed." << std::endl;

    // =========================================================================
    // 12. Track disappeared from Library resolves to nullopt
    // =========================================================================
    {
        // Copy fresh file in its own sub-dir
        std::string tmp_dir = "experiments/app-test/tmp";
        fs::create_directories(tmp_dir);
        std::string tmp_mp3 = tmp_dir + "/tmp.mp3";
        fs::copy_file(source_mp3, tmp_mp3);

        Application r_app({tmp_dir});
        wait_scan(r_app);
        auto t = r_app.get_tracks();
        r_app.play_track(t[0].id);

        fs::remove(tmp_mp3);
        r_app.rescan_library();
        wait_scan(r_app);
        assert(r_app.get_current_track() == std::nullopt);
    }
    std::cout << "  Disappeared track test passed." << std::endl;

    // =========================================================================
    // 13. Bad/unplayable file
    // =========================================================================
    {
        std::string tmp_dir2 = "experiments/app-test/bad";
        fs::create_directories(tmp_dir2);
        std::string bad_mp3 = tmp_dir2 + "/bad.mp3";
        { std::ofstream out(bad_mp3); out << "bad data"; }

        Application r_app({tmp_dir2});
        wait_scan(r_app);
        auto t = r_app.get_tracks();
        bool ok = r_app.play_track(t[0].id);
        assert(!ok);
        assert(r_app.get_current_track() == std::nullopt);
    }
    std::cout << "  Bad file test passed." << std::endl;

    std::cout << "\nAll Application + Queue assertions passed!" << std::endl;
    return 0;
}
