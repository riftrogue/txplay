#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include "Config.hpp"
#include "common/Key.hpp"

using namespace txplay::config;
using txplay::common::Key;
using txplay::common::KeyCode;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static const std::string TEST_ROOT = "experiments/config-test";

// Write a config file to the test directory and return its path.
static std::string write_tmp(const std::string& name, const std::string& content) {
    std::string path = TEST_ROOT + "/" + name;
    fs::create_directories(TEST_ROOT);
    std::ofstream out(path);
    out << content;
    return path;
}

// ---------------------------------------------------------------------------
// T01: Defaults (missing/empty config)
// ---------------------------------------------------------------------------
static void test_defaults() {
    std::cout << "T01: defaults from missing file..." << std::endl;
    Config cfg(TEST_ROOT + "/nonexistent.txt");

    assert(cfg.library().music_paths.empty());
    assert(cfg.playback().autoplay == false);
    assert(cfg.playback().autoplay_limit == false);
    assert(cfg.playback().autoplay_limit_value == 10);
    assert(cfg.playback().seek_seconds == 5);
    assert(cfg.visualizer().enabled == true);
    assert(cfg.visualizer().style == "bars");
    assert(cfg.visualizer().height == 6);

    // KeybindConfig defaults — RIGHT-side Key values
    assert(cfg.keybinds().play_pause      == Key::space());
    assert(cfg.keybinds().next            == Key::character('n'));
    assert(cfg.keybinds().previous        == Key::character('b'));
    assert(cfg.keybinds().navigation_up   == Key::arrow_up());
    assert(cfg.keybinds().navigation_down == Key::arrow_down());
    assert(cfg.keybinds().focus_next      == Key::tab());
    assert(cfg.keybinds().focus_previous  == Key::shift_tab());
    assert(cfg.keybinds().play            == Key::enter());
    assert(cfg.keybinds().back            == Key::escape());
    assert(cfg.keybinds().search          == Key::character('/'));
    assert(cfg.keybinds().refresh         == Key::character('r'));
    assert(cfg.keybinds().quit            == Key::character('q'));
    assert(cfg.keybinds().seek_forward    == Key::arrow_right());
    assert(cfg.keybinds().seek_backward   == Key::arrow_left());
    assert(cfg.keybinds().queue_add       == Key::character('a'));
    assert(cfg.keybinds().queue_remove    == Key::character('d'));
    assert(cfg.keybinds().queue_clear     == Key::character('c'));

    assert(!cfg.is_modified());
    std::cout << "  T01 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T02: Full valid config parsing
// ---------------------------------------------------------------------------
static void test_parse_valid() {
    std::cout << "T02: full valid config parsing..." << std::endl;
    auto path = write_tmp("valid.txt", R"(
[Library]
music_path=~/Music
music_path=/mnt/usb

[Playback]
autoplay=true
autoplay_limit=true
autoplay_limit_value=5
seek_seconds=10

[Visualizer]
enabled=false
style=bars
height=12

[Keybindings]
play_pause=Space
next=n
previous=b
navigation_up=ArrowUp
navigation_down=ArrowDown
focus_next=Tab
focus_previous=ShiftTab
play=Enter
back=Escape
search=/
refresh=r
quit=q
seek_forward=ArrowRight
seek_backward=ArrowLeft
queue_add=a
queue_remove=d
queue_clear=c
)");

    Config cfg(path);
    assert(cfg.library().music_paths.size() == 2);
    assert(cfg.library().music_paths[0] == "~/Music");
    assert(cfg.library().music_paths[1] == "/mnt/usb");
    assert(cfg.playback().autoplay == true);
    assert(cfg.playback().autoplay_limit == true);
    assert(cfg.playback().autoplay_limit_value == 5);
    assert(cfg.playback().seek_seconds == 10);
    assert(cfg.visualizer().enabled == false);
    assert(cfg.visualizer().height == 12);

    // All keybindings parsed correctly
    assert(cfg.keybinds().play_pause      == Key::space());
    assert(cfg.keybinds().next            == Key::character('n'));
    assert(cfg.keybinds().previous        == Key::character('b'));
    assert(cfg.keybinds().navigation_up   == Key::arrow_up());
    assert(cfg.keybinds().navigation_down == Key::arrow_down());
    assert(cfg.keybinds().focus_next      == Key::tab());
    assert(cfg.keybinds().focus_previous  == Key::shift_tab());
    assert(cfg.keybinds().play            == Key::enter());
    assert(cfg.keybinds().back            == Key::escape());
    assert(cfg.keybinds().search          == Key::character('/'));
    assert(cfg.keybinds().refresh         == Key::character('r'));
    assert(cfg.keybinds().quit            == Key::character('q'));
    assert(cfg.keybinds().seek_forward    == Key::arrow_right());
    assert(cfg.keybinds().seek_backward   == Key::arrow_left());
    assert(cfg.keybinds().queue_add       == Key::character('a'));
    assert(cfg.keybinds().queue_remove    == Key::character('d'));
    assert(cfg.keybinds().queue_clear     == Key::character('c'));

    std::cout << "  T02 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T03: [Navigation] backward-compat alias
// ---------------------------------------------------------------------------
static void test_navigation_alias() {
    std::cout << "T03: [Navigation] backward-compat..." << std::endl;
    // [Navigation] is a backward-compatible section-name alias.
    // New LEFT-side key names work inside it.
    // Old key names (pause=, seek_forward=right) are silently ignored.
    auto path = write_tmp("nav_alias.txt", R"(
[Navigation]
play_pause=p
search=s
refresh=u
quit=x
seek_forward=ArrowRight
seek_backward=ArrowLeft
queue_add=a
queue_remove=d
queue_clear=c
)");
    Config cfg(path);
    assert(cfg.keybinds().play_pause   == Key::character('p'));
    assert(cfg.keybinds().search       == Key::character('s'));
    assert(cfg.keybinds().refresh      == Key::character('u'));
    assert(cfg.keybinds().quit         == Key::character('x'));
    assert(cfg.keybinds().seek_forward == Key::arrow_right());
    std::cout << "  T03 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T04: [Characters] section silently ignored
// ---------------------------------------------------------------------------
static void test_characters_ignored() {
    std::cout << "T04: [Characters] ignored safely..." << std::endl;
    auto path = write_tmp("chars.txt", R"(
[Characters]
border_horizontal=-
border_vertical=|
)");
    Config cfg(path);
    assert(cfg.playback().seek_seconds == 5);
    std::cout << "  T04 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T05: Validation — clamp seek_seconds
// ---------------------------------------------------------------------------
static void test_validation_seek() {
    std::cout << "T05: validation clamping seek_seconds..." << std::endl;

    // Below minimum → clamped to 1
    auto path_low = write_tmp("seek_low.txt", "[Playback]\nseek_seconds=0\n");
    Config cfg_low(path_low);
    assert(cfg_low.playback().seek_seconds == 1);

    // Above maximum → clamped to 300
    auto path_high = write_tmp("seek_high.txt", "[Playback]\nseek_seconds=999\n");
    Config cfg_high(path_high);
    assert(cfg_high.playback().seek_seconds == 300);

    // Boundary values are preserved
    auto path_min = write_tmp("seek_min.txt", "[Playback]\nseek_seconds=1\n");
    Config cfg_min(path_min);
    assert(cfg_min.playback().seek_seconds == 1);

    auto path_max = write_tmp("seek_max.txt", "[Playback]\nseek_seconds=300\n");
    Config cfg_max(path_max);
    assert(cfg_max.playback().seek_seconds == 300);

    // Valid mid-range value
    auto path_ok = write_tmp("seek_ok.txt", "[Playback]\nseek_seconds=30\n");
    Config cfg_ok(path_ok);
    assert(cfg_ok.playback().seek_seconds == 30);

    std::cout << "  T05 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T06: Validation — clamp visualizer height
// ---------------------------------------------------------------------------
static void test_validation_height() {
    std::cout << "T06: validation clamping visualizer height..." << std::endl;

    // Below minimum → clamped to 1
    auto path_low = write_tmp("vis_height_low.txt", "[Visualizer]\nheight=0\n");
    Config cfg_low(path_low);
    assert(cfg_low.visualizer().height == 1);

    // Above maximum → clamped to 20
    auto path_high = write_tmp("vis_height_high.txt", "[Visualizer]\nheight=500\n");
    Config cfg_high(path_high);
    assert(cfg_high.visualizer().height == 20);

    // Boundary values preserved
    auto path_min = write_tmp("vis_height_min.txt", "[Visualizer]\nheight=1\n");
    Config cfg_min(path_min);
    assert(cfg_min.visualizer().height == 1);

    auto path_max = write_tmp("vis_height_max.txt", "[Visualizer]\nheight=20\n");
    Config cfg_max(path_max);
    assert(cfg_max.visualizer().height == 20);

    std::cout << "  T06 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T07: Validation — unknown keybind value reverts to default
// ---------------------------------------------------------------------------
static void test_validation_keybind() {
    std::cout << "T07: unknown keybind value reverts to default..." << std::endl;
    // Old format alias "space" is no longer recognized → Unknown → reverts to Space.
    auto path_old = write_tmp("kb_old.txt", "[Keybindings]\nplay_pause=space\n");
    Config cfg_old(path_old);
    assert(cfg_old.keybinds().play_pause == Key::space()); // reverts to default

    // Genuinely unknown value
    auto path_bad = write_tmp("kb_bad.txt", "[Keybindings]\nnavigation_up=NotAKey\n");
    Config cfg_bad(path_bad);
    assert(cfg_bad.keybinds().navigation_up == Key::arrow_up()); // default

    std::cout << "  T07 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T08: Runtime mutation
// ---------------------------------------------------------------------------
static void test_mutation() {
    std::cout << "T08: runtime mutation..." << std::endl;
    Config cfg(TEST_ROOT + "/nonexistent.txt");

    assert(!cfg.is_modified());

    cfg.set_autoplay(true);
    assert(cfg.playback().autoplay == true);
    assert(cfg.is_modified());

    cfg.set_autoplay_limit(true);
    assert(cfg.playback().autoplay_limit == true);

    cfg.set_autoplay_limit_value(7);
    assert(cfg.playback().autoplay_limit_value == 7);

    cfg.set_seek_seconds(15);
    assert(cfg.playback().seek_seconds == 15);

    cfg.set_visualizer_enabled(false);
    assert(cfg.visualizer().enabled == false);

    cfg.set_visualizer_height(10);
    assert(cfg.visualizer().height == 10);

    cfg.set_visualizer_style("bars");

    // Clamping via setters
    cfg.set_visualizer_height(999);
    assert(cfg.visualizer().height == 20);
    cfg.set_visualizer_height(-5);
    assert(cfg.visualizer().height == 1);

    cfg.set_seek_seconds(500);
    assert(cfg.playback().seek_seconds == 300);

    cfg.set_autoplay_limit_value(99999);
    assert(cfg.playback().autoplay_limit_value == 9999);
    cfg.set_autoplay_limit_value(0);
    assert(cfg.playback().autoplay_limit_value == 1);

    // Keybind mutation — new LEFT name, RIGHT-side canonical name
    cfg.set_keybind("play_pause", "p");
    assert(cfg.keybinds().play_pause == Key::character('p'));

    cfg.set_keybind("navigation_up", "k");
    assert(cfg.keybinds().navigation_up == Key::character('k'));

    cfg.set_keybind("seek_forward", "ArrowRight");
    assert(cfg.keybinds().seek_forward == Key::arrow_right());

    cfg.add_music_path("~/Music");
    assert(cfg.library().music_paths.size() == 1);

    cfg.remove_music_path(0);
    assert(cfg.library().music_paths.empty());

    // Setting same value should not mark modified
    Config cfg2(TEST_ROOT + "/nonexistent.txt");
    assert(!cfg2.is_modified());
    cfg2.set_autoplay(false); // already false
    assert(!cfg2.is_modified());

    std::cout << "  T08 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T09: Save / Load round-trip (using for_testing)
// ---------------------------------------------------------------------------
static void test_save_roundtrip() {
    std::cout << "T09: save/load round-trip..." << std::endl;

    std::string src = write_tmp("roundtrip_src.txt", R"(
[Library]
music_path=~/Music
music_path=/mnt/external

[Playback]
autoplay=true
autoplay_limit=true
autoplay_limit_value=8
seek_seconds=20

[Visualizer]
enabled=false
style=bars
height=8

[Keybindings]
play_pause=Space
next=n
previous=b
navigation_up=ArrowUp
navigation_down=ArrowDown
focus_next=Tab
focus_previous=ShiftTab
play=Enter
back=Escape
search=/
refresh=r
quit=q
seek_forward=ArrowRight
seek_backward=ArrowLeft
queue_add=a
queue_remove=d
queue_clear=c
)");

    Config cfg = Config::for_testing(src, TEST_ROOT + "/roundtrip_saved.txt");
    assert(cfg.playback().autoplay == true);
    assert(cfg.playback().autoplay_limit == true);
    assert(cfg.playback().autoplay_limit_value == 8);
    assert(cfg.playback().seek_seconds == 20);

    cfg.set_music_paths({"~/Music", "/mnt/external", "~/Podcasts"});
    assert(cfg.is_modified());
    cfg.save();
    assert(!cfg.is_modified());

    // Reload from saved file
    Config reloaded(TEST_ROOT + "/roundtrip_saved.txt");
    assert(reloaded.library().music_paths.size() == 3);
    assert(reloaded.library().music_paths[2] == "~/Podcasts");
    assert(reloaded.playback().seek_seconds == 20);
    assert(reloaded.playback().autoplay == true);
    assert(reloaded.playback().autoplay_limit == true);
    assert(reloaded.playback().autoplay_limit_value == 8);
    assert(reloaded.visualizer().enabled == false);
    assert(reloaded.visualizer().height == 8);
    assert(reloaded.keybinds().play_pause == Key::space());    // default round-trip

    std::cout << "  T09 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T10: modified flag — save() to isolated path clears flag
// ---------------------------------------------------------------------------
static void test_modified_flag() {
    std::cout << "T10: modified flag + isolated save..." << std::endl;

    std::string save_dir  = TEST_ROOT + "/save_isolated";
    std::string save_path = save_dir  + "/config.txt";

    // Ensure the save directory does not pre-exist (tests directory creation).
    if (fs::exists(save_dir)) fs::remove_all(save_dir);

    Config cfg = Config::for_testing(TEST_ROOT + "/nonexistent.txt", save_path);
    assert(!cfg.is_modified());

    cfg.set_autoplay(true);
    assert(cfg.is_modified());

    cfg.save();
    assert(!cfg.is_modified());           // flag cleared after successful save
    assert(fs::exists(save_path));        // file was created at the isolated path

    assert(save_path.find("experiments/") != std::string::npos);

    std::cout << "  T10 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T11: Multiple music_paths accumulate
// ---------------------------------------------------------------------------
static void test_multiple_paths() {
    std::cout << "T11: multiple music_path keys..." << std::endl;
    auto path = write_tmp("multi_paths.txt", R"(
[Library]
music_path=~/Music
music_path=/mnt/external
music_path=~/Podcasts
)");
    Config cfg(path);
    assert(cfg.library().music_paths.size() == 3);
    assert(cfg.library().music_paths[0] == "~/Music");
    assert(cfg.library().music_paths[1] == "/mnt/external");
    assert(cfg.library().music_paths[2] == "~/Podcasts");
    std::cout << "  T11 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T12: parse_bool case-insensitive
// ---------------------------------------------------------------------------
static void test_parse_bool() {
    std::cout << "T12: parse_bool case-insensitive..." << std::endl;
    auto path = write_tmp("bool_upper.txt", "[Playback]\nautoplay=True\n");
    Config cfg(path);
    assert(cfg.playback().autoplay == true);

    auto path2 = write_tmp("bool_false.txt", "[Playback]\nautoplay=FALSE\n");
    Config cfg2(path2);
    assert(cfg2.playback().autoplay == false);
    std::cout << "  T12 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T13: Comments and blank lines ignored
// ---------------------------------------------------------------------------
static void test_comments_ignored() {
    std::cout << "T13: comments and blank lines ignored..." << std::endl;
    auto path = write_tmp("comments.txt", R"(
# This is a comment
[Playback]
# autoplay is disabled by default
autoplay=true

# seek step
seek_seconds=7
)");
    Config cfg(path);
    assert(cfg.playback().autoplay == true);
    assert(cfg.playback().seek_seconds == 7);
    std::cout << "  T13 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T14: for_testing() does not affect production Config construction
// ---------------------------------------------------------------------------
static void test_for_testing_isolation() {
    std::cout << "T14: Config::for_testing isolates save path..." << std::endl;

    std::string isolated_save = TEST_ROOT + "/for_testing_out/config.txt";
    if (fs::exists(TEST_ROOT + "/for_testing_out"))
        fs::remove_all(TEST_ROOT + "/for_testing_out");

    Config cfg = Config::for_testing(TEST_ROOT + "/nonexistent.txt", isolated_save);
    cfg.set_autoplay(true);
    cfg.save();
    assert(fs::exists(isolated_save));

    Config normal(TEST_ROOT + "/nonexistent.txt");
    assert(!normal.is_modified());

    std::cout << "  T14 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T15: save() creates intermediate directories
// ---------------------------------------------------------------------------
static void test_save_creates_dirs() {
    std::cout << "T15: save() creates missing directories..." << std::endl;

    std::string deep_dir  = TEST_ROOT + "/deep/nested/dir";
    std::string deep_path = deep_dir  + "/config.txt";
    if (fs::exists(TEST_ROOT + "/deep")) fs::remove_all(TEST_ROOT + "/deep");

    Config cfg = Config::for_testing(TEST_ROOT + "/nonexistent.txt", deep_path);
    cfg.set_seek_seconds(42);
    cfg.save();
    assert(fs::exists(deep_path));

    Config reloaded(deep_path);
    assert(reloaded.playback().seek_seconds == 42);

    std::cout << "  T15 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T16: Empty config file loads all defaults
// ---------------------------------------------------------------------------
static void test_empty_config() {
    std::cout << "T16: empty config file loads defaults..." << std::endl;
    auto path = write_tmp("empty.txt", "");
    Config cfg(path);
    assert(cfg.playback().autoplay == false);
    assert(cfg.playback().autoplay_limit == false);
    assert(cfg.playback().autoplay_limit_value == 10);
    assert(cfg.playback().seek_seconds == 5);
    assert(cfg.visualizer().enabled == true);
    assert(cfg.visualizer().height == 6);
    assert(cfg.keybinds().play_pause == Key::space());       // renamed from pause
    assert(cfg.keybinds().navigation_up == Key::arrow_up()); // new default
    std::cout << "  T16 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T17: autoplay_limit_value validation (clamping)
// ---------------------------------------------------------------------------
static void test_autoplay_limit_value_validation() {
    std::cout << "T17: autoplay_limit_value clamping..." << std::endl;

    // Parse: below minimum (0 is invalid) → clamped to 1
    auto p_low = write_tmp("alv_low.txt", "[Playback]\nautoplay_limit_value=0\n");
    Config cfg_low(p_low);
    assert(cfg_low.playback().autoplay_limit_value == 1);

    // Parse: valid value preserved
    auto p_ok = write_tmp("alv_ok.txt", "[Playback]\nautoplay_limit_value=25\n");
    Config cfg_ok(p_ok);
    assert(cfg_ok.playback().autoplay_limit_value == 25);

    // Parse: above maximum → clamped to 9999
    auto p_high = write_tmp("alv_high.txt", "[Playback]\nautoplay_limit_value=99999\n");
    Config cfg_high(p_high);
    assert(cfg_high.playback().autoplay_limit_value == 9999);

    std::cout << "  T17 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T18: [Keybindings] section — new LEFT names and RIGHT key names
// ---------------------------------------------------------------------------
static void test_keybindings_section() {
    std::cout << "T18: [Keybindings] section — all LEFT=RIGHT..." << std::endl;
    auto path = write_tmp("kb_direct.txt", R"(
[Keybindings]
play_pause=p
next=m
previous=z
navigation_up=k
navigation_down=j
focus_next=Tab
focus_previous=ShiftTab
play=Enter
back=Escape
search=s
refresh=u
quit=x
seek_forward=ArrowRight
seek_backward=ArrowLeft
queue_add=e
queue_remove=w
queue_clear=y
)");
    Config cfg(path);
    assert(cfg.keybinds().play_pause      == Key::character('p'));
    assert(cfg.keybinds().next            == Key::character('m'));
    assert(cfg.keybinds().previous        == Key::character('z'));
    assert(cfg.keybinds().navigation_up   == Key::character('k'));
    assert(cfg.keybinds().navigation_down == Key::character('j'));
    assert(cfg.keybinds().focus_next      == Key::tab());
    assert(cfg.keybinds().focus_previous  == Key::shift_tab());
    assert(cfg.keybinds().play            == Key::enter());
    assert(cfg.keybinds().back            == Key::escape());
    assert(cfg.keybinds().search          == Key::character('s'));
    assert(cfg.keybinds().refresh         == Key::character('u'));
    assert(cfg.keybinds().quit            == Key::character('x'));
    assert(cfg.keybinds().seek_forward    == Key::arrow_right());
    assert(cfg.keybinds().seek_backward   == Key::arrow_left());
    assert(cfg.keybinds().queue_add       == Key::character('e'));
    assert(cfg.keybinds().queue_remove    == Key::character('w'));
    assert(cfg.keybinds().queue_clear     == Key::character('y'));
    std::cout << "  T18 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T19: autoplay and autoplay_limit parsed independently
// ---------------------------------------------------------------------------
static void test_autoplay_parsing() {
    std::cout << "T19: autoplay fields parsed independently..." << std::endl;

    // autoplay=false, autoplay_limit=true — limit field has no effect when
    // autoplay=false, but must still parse correctly.
    auto p1 = write_tmp("ap1.txt",
        "[Playback]\nautoplay=false\nautoplay_limit=true\nautoplay_limit_value=3\n");
    Config cfg1(p1);
    assert(cfg1.playback().autoplay == false);
    assert(cfg1.playback().autoplay_limit == true);
    assert(cfg1.playback().autoplay_limit_value == 3);

    // autoplay=true, autoplay_limit=false — unlimited autoplay
    auto p2 = write_tmp("ap2.txt",
        "[Playback]\nautoplay=true\nautoplay_limit=false\nautoplay_limit_value=50\n");
    Config cfg2(p2);
    assert(cfg2.playback().autoplay == true);
    assert(cfg2.playback().autoplay_limit == false);
    assert(cfg2.playback().autoplay_limit_value == 50);

    std::cout << "  T19 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T20: set_keybind with empty key is a no-op (does not clear)
// ---------------------------------------------------------------------------
static void test_set_keybind_empty_noop() {
    std::cout << "T20: set_keybind with empty key is no-op..." << std::endl;
    Config cfg(TEST_ROOT + "/nonexistent.txt");
    assert(cfg.keybinds().play_pause == Key::space());
    cfg.set_keybind("play_pause", "");              // empty key: must be ignored
    assert(cfg.keybinds().play_pause == Key::space()); // unchanged
    assert(!cfg.is_modified());
    std::cout << "  T20 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T21: Key::parse() — special key names
// ---------------------------------------------------------------------------
static void test_key_parse_special() {
    std::cout << "T21: Key::parse() special key names..." << std::endl;
    assert(Key::parse("Space")     == Key::space());
    assert(Key::parse("Enter")     == Key::enter());
    assert(Key::parse("Escape")    == Key::escape());
    assert(Key::parse("Tab")       == Key::tab());
    assert(Key::parse("ShiftTab")  == Key::shift_tab());
    assert(Key::parse("ArrowUp")   == Key::arrow_up());
    assert(Key::parse("ArrowDown") == Key::arrow_down());
    assert(Key::parse("ArrowLeft") == Key::arrow_left());
    assert(Key::parse("ArrowRight")== Key::arrow_right());
    assert(Key::parse("Backspace") == Key::backspace());
    std::cout << "  T21 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T22: Key::parse() — character keys
// ---------------------------------------------------------------------------
static void test_key_parse_characters() {
    std::cout << "T22: Key::parse() character keys..." << std::endl;
    assert(Key::parse("n")  == Key::character('n'));
    assert(Key::parse("/")  == Key::character('/'));
    assert(Key::parse("?")  == Key::character('?'));
    assert(Key::parse("a")  == Key::character('a'));
    assert(Key::parse("z")  == Key::character('z'));
    assert(Key::parse(" ")  == Key::character(' ')); // single space → Character, not Space
    // (Space the canonical name produces Space; " " the single character produces Character(' '))
    // These are distinct Keys because their KeyCode differs.
    assert(Key::parse(" ") != Key::space());
    std::cout << "  T22 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T23: Key::parse() — unknown / old-format names produce Unknown
// ---------------------------------------------------------------------------
static void test_key_parse_unknown() {
    std::cout << "T23: Key::parse() unknown / old-format names..." << std::endl;
    // Old aliases (not supported)
    assert(Key::parse("right").is_unknown());
    assert(Key::parse("left").is_unknown());
    assert(Key::parse("up").is_unknown());
    assert(Key::parse("down").is_unknown());
    assert(Key::parse("space").is_unknown());   // "space" ≠ "Space"
    assert(Key::parse("enter").is_unknown());   // "enter" ≠ "Enter"
    assert(Key::parse("escape").is_unknown());  // "escape" ≠ "Escape"
    // Genuinely garbage values
    assert(Key::parse("NotAKey").is_unknown());
    assert(Key::parse("garbage").is_unknown());
    assert(Key::parse("").is_unknown());
    assert(Key::parse("ArrowUp2").is_unknown());
    assert(Key::parse("SPACE").is_unknown());   // wrong case
    // Removed key codes — no longer supported
    assert(Key::parse("Delete").is_unknown());
    assert(Key::parse("Home").is_unknown());
    assert(Key::parse("End").is_unknown());
    assert(Key::parse("PageUp").is_unknown());
    assert(Key::parse("PageDown").is_unknown());
    assert(Key::parse("F1").is_unknown());
    assert(Key::parse("F6").is_unknown());
    assert(Key::parse("F12").is_unknown());
    std::cout << "  T23 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T24: Key::name() round-trip — parse(key.name()) == key
// ---------------------------------------------------------------------------
static void test_key_name_roundtrip() {
    std::cout << "T24: Key::name() round-trip..." << std::endl;
    auto rt = [](const Key& k) {
        return Key::parse(k.name()) == k;
    };
    assert(rt(Key::space()));
    assert(rt(Key::enter()));
    assert(rt(Key::escape()));
    assert(rt(Key::tab()));
    assert(rt(Key::shift_tab()));
    assert(rt(Key::arrow_up()));
    assert(rt(Key::arrow_down()));
    assert(rt(Key::arrow_left()));
    assert(rt(Key::arrow_right()));
    assert(rt(Key::backspace()));
    assert(rt(Key::character('n')));
    assert(rt(Key::character('/')));
    assert(rt(Key::character('?')));
    std::cout << "  T24 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T25: Config with remapped navigation key (navigation_up=k)
// ---------------------------------------------------------------------------
static void test_remapped_navigation() {
    std::cout << "T25: navigation_up=k remaps correctly..." << std::endl;
    auto path = write_tmp("remap_nav.txt", "[Keybindings]\nnavigation_up=k\nnavigation_down=j\n");
    Config cfg(path);
    assert(cfg.keybinds().navigation_up   == Key::character('k'));
    assert(cfg.keybinds().navigation_down == Key::character('j'));
    // Others still default
    assert(cfg.keybinds().play_pause      == Key::space());
    assert(cfg.keybinds().quit            == Key::character('q'));
    std::cout << "  T25 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// T26: set_keybind with unknown key_name is a no-op
// ---------------------------------------------------------------------------
static void test_set_keybind_unknown_noop() {
    std::cout << "T26: set_keybind with unknown key_name is no-op..." << std::endl;
    Config cfg(TEST_ROOT + "/nonexistent.txt");
    // "right" is old vocab — not recognized
    cfg.set_keybind("seek_forward", "right");
    assert(cfg.keybinds().seek_forward == Key::arrow_right()); // unchanged default
    assert(!cfg.is_modified());
    // Garbage value
    cfg.set_keybind("quit", "NotAKey");
    assert(cfg.keybinds().quit == Key::character('q'));
    assert(!cfg.is_modified());
    std::cout << "  T26 passed." << std::endl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "--- Config Test Runner ---" << std::endl;
    fs::create_directories(TEST_ROOT);

    test_defaults();
    test_parse_valid();
    test_navigation_alias();
    test_characters_ignored();
    test_validation_seek();
    test_validation_height();
    test_validation_keybind();
    test_mutation();
    test_save_roundtrip();
    test_modified_flag();
    test_multiple_paths();
    test_parse_bool();
    test_comments_ignored();
    test_for_testing_isolation();
    test_save_creates_dirs();
    test_empty_config();
    test_autoplay_limit_value_validation();
    test_keybindings_section();
    test_autoplay_parsing();
    test_set_keybind_empty_noop();
    test_key_parse_special();
    test_key_parse_characters();
    test_key_parse_unknown();
    test_key_name_roundtrip();
    test_remapped_navigation();
    test_set_keybind_unknown_noop();

    std::cout << "\nAll Config assertions passed! (26 tests)" << std::endl;
    return 0;
}
