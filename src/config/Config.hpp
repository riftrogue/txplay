#pragma once

#include <string>
#include <vector>
#include <map>
#include "common/PathUtils.hpp"

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

struct KeybindConfig {
    std::string pause{"space"};
    std::string search{"/"};
    std::string refresh{"r"};
    std::string quit{"q"};
    std::string seek_forward{"right"};
    std::string seek_backward{"left"};
    std::string queue_add{"a"};
    std::string queue_remove{"d"};
    std::string queue_clear{"c"};
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

    // Keybindings
    // action is one of: "pause", "search", "refresh", "quit",
    //                   "seek_forward", "seek_backward",
    //                   "queue_add", "queue_remove", "queue_clear"
    void set_keybind(const std::string& action, const std::string& key);

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
