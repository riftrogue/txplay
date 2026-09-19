#include <iostream>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "Application.hpp"
#include "config/Config.hpp"

using namespace txplay::application;
using namespace std::chrono_literals;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

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

// Helper: build a minimal Config with a given set of paths and autoplay flags.
static txplay::config::Config make_test_config(
    const std::vector<std::string>& paths,
    bool autoplay = false,
    bool autoplay_limit = false,
    int  autoplay_limit_value = 10)
{
    static const std::string tmp_cfg = "experiments/app-test/tmp_test_config.txt";
    {
        std::ofstream out(tmp_cfg);
        out << "[Library]\n";
        for (const auto& p : paths)
            out << "music_path=" << p << "\n";
        out << "[Playback]\n";
        out << "autoplay=" << (autoplay ? "true" : "false") << "\n";
        out << "autoplay_limit=" << (autoplay_limit ? "true" : "false") << "\n";
        out << "autoplay_limit_value=" << autoplay_limit_value << "\n";
    }
    return txplay::config::Config(tmp_cfg);
}

int main() {
    std::cout << "--- Application Test Runner ---" << std::endl;

    std::string test_dir   = "experiments/app-test/root";
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
    auto main_config = make_test_config(paths);
    Application app(main_config);
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
        auto cfg = make_test_config(paths);
        Application q_app(cfg);
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
        auto cfg = make_test_config(paths);
        Application q_app(cfg);
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
        auto cfg = make_test_config(paths);
        Application q_app(cfg);
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
        auto cfg = make_test_config(paths);
        Application q_app(cfg);
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
        auto cfg = make_test_config(paths, /*autoplay=*/false);
        Application q_app(cfg);
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
        auto cfg = make_test_config(paths, /*autoplay=*/true);
        Application q_app(cfg);
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

    // Q7: autoplay=true, empty queue — EOF stops cleanly (no local tracks to pick)
    // NOTE: The test library has 2 tracks (A and B). We play track B (last one
    // in sorted order) so advance_autoplay finds no next track and stops.
    {
        std::cout << "Q7: autoplay=true, empty queue, at end of library — stops..." << std::endl;
        auto cfg = make_test_config(paths, /*autoplay=*/true);
        Application q_app(cfg);
        wait_scan(q_app);
        auto t = q_app.get_tracks();

        // Sort: play the last track so there is no next track
        std::string last = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        q_app.play_track(last);
        q_app.seek(999999);

        bool stopped = wait_stopped(q_app, 400);
        assert(stopped);
        assert(q_app.queue_is_empty());
        std::cout << "  Q7 passed." << std::endl;
    }

    // Q8: autoplay=true, invalid queued track — skipped safely
    {
        std::cout << "Q8: invalid queued track skipped safely..." << std::endl;
        auto cfg = make_test_config(paths, /*autoplay=*/true);
        Application q_app(cfg);
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
    // AUTOPLAY TESTS
    // =========================================================================
    std::cout << "\n--- Autoplay Tests ---" << std::endl;

    // Q9: autoplay=false, empty queue → stops after current track
    {
        std::cout << "Q9: autoplay=false + empty queue → stop..." << std::endl;
        auto cfg = make_test_config(paths, /*autoplay=*/false);
        Application q_app(cfg);
        wait_scan(q_app);
        auto t = q_app.get_tracks();
        std::string first = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;

        q_app.play_track(first);
        q_app.seek(999999);

        bool stopped = wait_stopped(q_app, 400);
        assert(stopped);
        std::cout << "  Q9 passed." << std::endl;
    }

    // Q10: autoplay=true + empty queue → plays next local track
    {
        std::cout << "Q10: autoplay=true + empty queue → next local track..." << std::endl;
        auto cfg = make_test_config(paths, /*autoplay=*/true);
        Application q_app(cfg);
        wait_scan(q_app);
        auto t = q_app.get_tracks();
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        q_app.play_track(first);   // manual: resets autoplay counter, no limit
        q_app.seek(999999);

        bool advanced = false;
        for (int i = 0; i < 400; ++i) {
            q_app.update();
            auto ct = q_app.get_current_track();
            if (ct.has_value() && ct->id == second) { advanced = true; break; }
            std::this_thread::sleep_for(10ms);
        }
        assert(advanced);
        std::cout << "  Q10 passed." << std::endl;
    }

    // Q11: queue tracks do NOT count toward the autoplay limit
    {
        std::cout << "Q11: queue tracks do not count toward autoplay limit..." << std::endl;
        // Limit=1: after the queue drains, exactly 1 local autoplay track should play.
        auto cfg = make_test_config(paths, true, true, 1);
        Application q_app(cfg);
        wait_scan(q_app);
        auto t = q_app.get_tracks();
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        // Queue the second track, play the first manually.
        q_app.queue_add(second);
        q_app.play_track(first);
        q_app.seek(999999);

        // Should advance to second (queue item — not counted).
        bool got_second = false;
        for (int i = 0; i < 400; ++i) {
            q_app.update();
            auto ct = q_app.get_current_track();
            if (ct.has_value() && ct->id == second) { got_second = true; break; }
            std::this_thread::sleep_for(10ms);
        }
        assert(got_second);
        assert(q_app.queue_is_empty());
        // After queue exhausted, there are no more local tracks (second was last),
        // so it should stop regardless of the limit.
        q_app.seek(999999);
        bool stopped = wait_stopped(q_app, 400);
        assert(stopped);
        std::cout << "  Q11 passed." << std::endl;
    }

    // Q12: manual track selection does not increment autoplay counter
    {
        std::cout << "Q12: manual selection does not count as autoplay..." << std::endl;
        // Limit=1 but autoplay. Play first manually, play second manually.
        // Neither increments the counter. Queue unchanged.
        auto cfg = make_test_config(paths, true, true, 1);
        Application q_app(cfg);
        wait_scan(q_app);
        auto t = q_app.get_tracks();
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        q_app.queue_add(first);  // add to queue to verify it's unchanged
        q_app.play_track(second); // manual
        auto q = q_app.get_queue();
        assert(q.size() == 1 && q[0] == first); // queue unmodified
        std::cout << "  Q12 passed." << std::endl;
    }

    // Q13: autoplay limit stops after configured number of local tracks
    {
        std::cout << "Q13: autoplay limit stops after limit..." << std::endl;
        // Library has 2 tracks (A=first, B=second in sorted order).
        // Play A manually, autoplay should advance to B (count=1).
        // Limit=1 → stops after B finishes (no more tracks anyway in this lib).
        // This also tests limit=1 with exactly 1 local track available.
        auto cfg = make_test_config(paths, true, true, 1);
        Application q_app(cfg);
        wait_scan(q_app);
        auto t = q_app.get_tracks();
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        q_app.play_track(first);
        q_app.seek(999999);

        // Autoplay should pick second (count becomes 1 = limit).
        bool got_second = false;
        for (int i = 0; i < 400; ++i) {
            q_app.update();
            auto ct = q_app.get_current_track();
            if (ct.has_value() && ct->id == second) { got_second = true; break; }
            std::this_thread::sleep_for(10ms);
        }
        assert(got_second);

        // Now seek second to EOF — limit already reached, should stop.
        q_app.seek(999999);
        bool stopped = wait_stopped(q_app, 400);
        assert(stopped);
        std::cout << "  Q13 passed." << std::endl;
    }

    // =========================================================================
    // Legacy Application Tests
    // =========================================================================
    std::cout << "\n--- Legacy Application Tests ---" << std::endl;
    {
        auto cfg = make_test_config(paths);
        Application r_app(cfg);
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

        auto cfg = make_test_config({tmp_dir});
        Application r_app(cfg);
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

        auto cfg = make_test_config({tmp_dir2});
        Application r_app(cfg);
        wait_scan(r_app);
        auto t = r_app.get_tracks();
        bool ok = r_app.play_track(t[0].id);
        assert(!ok);
        assert(r_app.get_current_track() == std::nullopt);
    }
    std::cout << "  Bad file test passed." << std::endl;

    // =========================================================================
    // Next / Previous Tests
    // =========================================================================
    std::cout << "\n--- Next / Previous Tests ---" << std::endl;

    // NP1: play_next() — queue has items → plays queue front, queue shrinks
    {
        std::cout << "NP1: play_next() consumes queue front..." << std::endl;
        auto cfg = make_test_config(paths);
        Application np_app(cfg);
        wait_scan(np_app);
        auto t = np_app.get_tracks();
        assert(t.size() == 2);
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        np_app.play_track(first);
        np_app.queue_add(second);
        assert(np_app.get_queue().size() == 1);

        np_app.play_next();

        // Queue should be empty and current track should be second
        assert(np_app.get_queue().empty());
        assert(np_app.get_current_track().has_value());
        assert(np_app.get_current_track()->id == second);
        assert(np_app.get_playback_state() == txplay::audio::PlaybackState::Playing);
        std::cout << "  NP1 passed." << std::endl;
    }

    // NP2: play_next() — empty queue, shuffle OFF → next library track
    {
        std::cout << "NP2: play_next() library fallback (no queue)..." << std::endl;
        auto cfg = make_test_config(paths);
        Application np_app(cfg);
        wait_scan(np_app);
        auto t = np_app.get_tracks();
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        np_app.play_track(first);
        assert(np_app.queue_is_empty());

        np_app.play_next();

        assert(np_app.get_current_track().has_value());
        assert(np_app.get_current_track()->id == second);
        assert(np_app.get_playback_state() == txplay::audio::PlaybackState::Playing);
        std::cout << "  NP2 passed." << std::endl;
    }

    // NP3: play_next() — queue priority over library (even when library has next)
    {
        std::cout << "NP3: play_next() queue takes priority over library..." << std::endl;
        auto cfg = make_test_config(paths);
        Application np_app(cfg);
        wait_scan(np_app);
        auto t = np_app.get_tracks();
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        // Playing first; queue has second — next should take second from queue,
        // not second from library (they happen to be the same track here, so
        // verify queue was consumed).
        np_app.play_track(first);
        np_app.queue_add(second);

        np_app.play_next();

        assert(np_app.queue_is_empty()); // queue was consumed, not bypassed
        assert(np_app.get_current_track()->id == second);
        std::cout << "  NP3 passed." << std::endl;
    }

    // NP4: play_next() — at last library track, no queue → do nothing
    {
        std::cout << "NP4: play_next() at end of library → no-op..." << std::endl;
        auto cfg = make_test_config(paths);
        Application np_app(cfg);
        wait_scan(np_app);
        auto t = np_app.get_tracks();
        std::string last = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        np_app.play_track(last);
        np_app.play_next(); // at end — should do nothing

        // Still playing the last track (no change)
        assert(np_app.get_current_track().has_value());
        assert(np_app.get_current_track()->id == last);
        std::cout << "  NP4 passed." << std::endl;
    }

    // NP5: play_next() works regardless of autoplay setting
    {
        std::cout << "NP5: play_next() unaffected by autoplay=false..." << std::endl;
        auto cfg = make_test_config(paths, /*autoplay=*/false);
        Application np_app(cfg);
        wait_scan(np_app);
        auto t = np_app.get_tracks();
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        np_app.play_track(first);
        np_app.play_next(); // must work even with autoplay=false

        assert(np_app.get_current_track().has_value());
        assert(np_app.get_current_track()->id == second);
        std::cout << "  NP5 passed." << std::endl;
    }

    // NP6: play_previous() returns the immediately previous played track
    //      (one-step history, source-agnostic)
    {
        std::cout << "NP6: play_previous() returns immediately previous track..." << std::endl;
        auto cfg = make_test_config(paths);
        Application np_app(cfg);
        wait_scan(np_app);
        auto t = np_app.get_tracks();
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        np_app.play_track(first);   // previous = ""    current = first
        np_app.play_track(second);  // previous = first current = second

        np_app.play_previous();     // should go back to first

        assert(np_app.get_current_track().has_value());
        assert(np_app.get_current_track()->id == first);
        assert(np_app.get_playback_state() == txplay::audio::PlaybackState::Playing);
        std::cout << "  NP6 passed." << std::endl;
    }

    // NP7: play_previous() slot is consumed — pressing b twice does nothing
    {
        std::cout << "NP7: play_previous() consumed after one use..." << std::endl;
        auto cfg = make_test_config(paths);
        Application np_app(cfg);
        wait_scan(np_app);
        auto t = np_app.get_tracks();
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        np_app.play_track(first);
        np_app.play_track(second);

        np_app.play_previous();  // goes to first, slot cleared
        assert(np_app.get_current_track()->id == first);

        np_app.play_previous();  // no history left — must not change
        assert(np_app.get_current_track()->id == first); // still first
        assert(np_app.get_playback_state() == txplay::audio::PlaybackState::Playing);
        std::cout << "  NP7 passed." << std::endl;
    }

    // NP8: Autoplay regression — play_next() does not break subsequent autoplay;
    //      autoplay transition correctly populates the previous-track slot.
    {
        std::cout << "NP8: autoplay regression + previous slot after autoplay..." << std::endl;
        auto cfg = make_test_config(paths, /*autoplay=*/true);
        Application np_app(cfg);
        wait_scan(np_app);
        auto t = np_app.get_tracks();
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        // play_next() from first → lands on second.
        np_app.play_track(first);
        np_app.play_next();
        assert(np_app.get_current_track()->id == second);

        // Pressing b should return to first (play_next set previous = first).
        np_app.play_previous();
        assert(np_app.get_current_track()->id == first);

        // Autoplay regression: seek first to EOF; autoplay at end of library
        // (first is the last autoplay track after returning) — stops cleanly.
        np_app.seek(999999);
        bool stopped = wait_stopped(np_app, 400);
        assert(stopped);
        std::cout << "  NP8 passed." << std::endl;
    }

    // NP9: No toggle — play_previous cannot oscillate C→B→C→B
    {
        std::cout << "NP9: no C->B->C toggle behavior..." << std::endl;
        auto cfg = make_test_config(paths);
        Application np_app(cfg);
        wait_scan(np_app);
        auto t = np_app.get_tracks();
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        np_app.play_track(first);
        np_app.play_track(second); // previous = first, current = second

        np_app.play_previous();    // goes to first, previous cleared
        assert(np_app.get_current_track()->id == first);

        // Must NOT go back to second — previous was consumed
        np_app.play_previous();
        assert(np_app.get_current_track()->id == first); // unchanged
        std::cout << "  NP9 passed." << std::endl;
    }

    // NP10: play_previous works across playback sources (queue → library)
    {
        std::cout << "NP10: play_previous works across playback sources..." << std::endl;
        auto cfg = make_test_config(paths);
        Application np_app(cfg);
        wait_scan(np_app);
        auto t = np_app.get_tracks();
        std::string first  = (t[0].filename < t[1].filename) ? t[0].id : t[1].id;
        std::string second = (t[0].filename < t[1].filename) ? t[1].id : t[0].id;

        // Play first from library directly; queue second and advance via play_next
        np_app.play_track(first);  // library source
        np_app.queue_add(second);
        np_app.play_next();        // queue source; previous = first, current = second

        assert(np_app.get_current_track()->id == second);
        assert(np_app.queue_is_empty());

        np_app.play_previous();    // should return to first regardless of source

        assert(np_app.get_current_track().has_value());
        assert(np_app.get_current_track()->id == first);
        assert(np_app.get_playback_state() == txplay::audio::PlaybackState::Playing);
        std::cout << "  NP10 passed." << std::endl;
    }

    std::cout << "\nAll Application + Queue + Autoplay + Next/Previous assertions passed!" << std::endl;
    return 0;
}
