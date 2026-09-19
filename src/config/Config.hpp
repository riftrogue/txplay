#pragma once

#include <string>
#include <vector>
#include "common/PathUtils.hpp"
#include "common/Key.hpp"

namespace txplay::config {

// ---------------------------------------------------------------------------
// Configuration Structs
// ---------------------------------------------------------------------------

struct LibraryConfig {
    std::vector<std::string> music_paths;   // from repeated music_path= keys
};

struct PlaybackConfig {
    bool autoplay{false};
    bool autoplay_limit{false};       // cap local autoplay track count
    int  autoplay_limit_value{10};    // max local tracks to auto-play (1–9999)
    int  seek_seconds{5};             // seconds per seek keypress (1–300)
};

struct VisualizerConfig {
    bool        enabled{true};
    std::string style{"bars"};  // currently only "bars" is supported
    int         height{6};      // terminal rows (1–20)
};

// ---------------------------------------------------------------------------
// KeybindConfig
//
// Each field name is the LEFT-side vocabulary (the Txplay name for the
// FTXUI/input event).  Each field value is the RIGHT-side Key (the physical
// keyboard key the user has configured).
//
// config.txt connects them:
//   LEFT=RIGHT   e.g.  navigation_up=ArrowUp
//
// Defaults match the canonical shipped config.txt.
// ---------------------------------------------------------------------------
struct KeybindConfig {
    // Playback
    common::Key play_pause   {common::Key::space()};
    common::Key next         {common::Key::character('n')};
    common::Key previous     {common::Key::character('b')};

    // Navigation (UI pane movement)
    common::Key navigation_up  {common::Key::arrow_up()};
    common::Key navigation_down{common::Key::arrow_down()};
    common::Key focus_next     {common::Key::tab()};
    common::Key focus_previous {common::Key::shift_tab()};
    common::Key play           {common::Key::enter()};
    common::Key back           {common::Key::escape()};

    // General
    common::Key search {common::Key::character('/')};
    common::Key refresh{common::Key::character('r')};
    common::Key quit   {common::Key::character('q')};

    // Seeking
    common::Key seek_forward {common::Key::arrow_right()};
    common::Key seek_backward{common::Key::arrow_left()};

    // Queue
    common::Key queue_add   {common::Key::character('a')};
    common::Key queue_remove{common::Key::character('d')};
    common::Key queue_clear {common::Key::character('c')};
};

// ---------------------------------------------------------------------------
// Config class
// ---------------------------------------------------------------------------

class Config {
public:
    // Construct from a file path. Unknown keys/sections are silently ignored.
    // Missing keys use the defaults defined in the structs above.
    explicit Config(const std::string& config_file_path);

    // Resolves the user config path in priority order:
    //   1. ~/.config/txplay/config.txt  (installed user config)
    //   2. ./config.txt                 (dev fallback)
    // The save path is always ~/.config/txplay/config.txt regardless of
    // which file was loaded.
    static std::string resolve_user_config_path();

    // Test-only factory. Identical to Config(config_file_path) but saves to
    // save_path instead of ~/.config/txplay/config.txt. Must NOT be called
    // from production code. Tests must never write to the real user config.
    static Config for_testing(const std::string& config_file_path,
                              const std::string& save_path);

    // ---- Read access (const references — no copies) ----
    const LibraryConfig&    library()    const { return library_; }
    const PlaybackConfig&   playback()   const { return playback_; }
    const VisualizerConfig& visualizer() const { return visualizer_; }
    const KeybindConfig&    keybinds()   const { return keybinds_; }

    // ---- Runtime mutation (called by Settings UI; all on UI thread) ----

    // Library
    void add_music_path(const std::string& path);
    void remove_music_path(std::size_t index);
    void set_music_paths(const std::vector<std::string>& paths);

    // Playback
    void set_autoplay(bool value);
    void set_autoplay_limit(bool value);
    void set_autoplay_limit_value(int value);  // clamped to [1, 9999]
    void set_seek_seconds(int value);          // clamped to [1, 300]

    // Visualizer
    void set_visualizer_enabled(bool value);
    void set_visualizer_height(int value);   // clamped to [1, 20]
    void set_visualizer_style(const std::string& value);

    // Keybindings — action is one of the LEFT-side names:
    //   play_pause, next, previous,
    //   navigation_up, navigation_down, focus_next, focus_previous, play, back,
    //   search, refresh, quit,
    //   seek_forward, seek_backward,
    //   queue_add, queue_remove, queue_clear
    // key_name uses the RIGHT-side canonical names (e.g. "ArrowUp", "n", "Space").
    // Empty key_name or unrecognized key_name is a no-op.
    void set_keybind(const std::string& action, const std::string& key_name);

    // ---- Persistence ----
    // Saves the current configuration to ~/.config/txplay/config.txt.
    // Creates the directory if it does not exist.
    // Uses a temp-file + rename strategy for atomic writes.
    // Clears the modified flag on success; leaves it set on failure.
    void save() const;

    // True if any value has been changed since construction or the last save().
    bool is_modified() const { return modified_; }

private:
    // Normalize all values to their valid ranges after parsing.
    void validate();

    LibraryConfig    library_;
    PlaybackConfig   playback_;
    VisualizerConfig visualizer_;
    KeybindConfig    keybinds_;

    // The canonical save path (always ~/.config/txplay/config.txt).
    std::string save_path_;

    // Tracks unsaved changes.
    mutable bool modified_{false};
};

} // namespace txplay::config
