#include "Config.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <system_error>

namespace txplay::config {
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(static_cast<unsigned char>(*start)))
        ++start;
    auto end = s.end();
    do { --end; } while (std::distance(start, end) > 0 &&
                         std::isspace(static_cast<unsigned char>(*end)));
    return std::string(start, end + 1);
}

static std::string to_lower_str(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

static bool parse_bool(const std::string& value, bool fallback) {
    std::string v = to_lower_str(trim(value));
    if (v == "true"  || v == "1") return true;
    if (v == "false" || v == "0") return false;
    return fallback;
}

static int parse_int(const std::string& value, int fallback) {
    try { return std::stoi(value); } catch (...) { return fallback; }
}

// ---------------------------------------------------------------------------
// resolve_user_config_path()
// ---------------------------------------------------------------------------

// static
std::string Config::resolve_user_config_path() {
    std::string user_config = common::expand_tilde("~/.config/txplay/config.txt");
    if (std::ifstream(user_config).good()) {
        return user_config;
    }
    return "config.txt"; // dev fallback
}

// ---------------------------------------------------------------------------
// Constructor — parse
// ---------------------------------------------------------------------------

Config::Config(const std::string& config_file_path) {
    // Canonical save target is always the user config directory,
    // regardless of which file was actually loaded (dev fallback or user).
    save_path_ = common::expand_tilde("~/.config/txplay/config.txt");

    std::ifstream file(config_file_path);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file: "
                  << config_file_path << std::endl;
        validate();
        return;
    }

    std::string current_section;
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line.front() == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            continue;
        }

        std::size_t pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key   = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));

        if (current_section == "Library") {
            if (key == "music_path")
                library_.music_paths.push_back(value);

        } else if (current_section == "Visualizer") {
            if      (key == "enabled") visualizer_.enabled = parse_bool(value, true);
            else if (key == "style")   visualizer_.style   = value;
            else if (key == "height")  visualizer_.height  = parse_int(value, 6);

        } else if (current_section == "Keybindings" ||
                   current_section == "Navigation") {
            // [Navigation] is accepted as a backward-compatible alias.
            // On save(), only [Keybindings] is written.
            if      (key == "pause")         keybinds_.pause         = value;
            else if (key == "search")        keybinds_.search        = value;
            else if (key == "refresh")       keybinds_.refresh       = value;
            else if (key == "quit")          keybinds_.quit          = value;
            else if (key == "seek_forward")  keybinds_.seek_forward  = value;
            else if (key == "seek_backward") keybinds_.seek_backward = value;
            else if (key == "queue_add")     keybinds_.queue_add     = value;
            else if (key == "queue_remove")  keybinds_.queue_remove  = value;
            else if (key == "queue_clear")   keybinds_.queue_clear   = value;
            // play=enter is intentionally ignored (not user-configurable)

        } else if (current_section == "Playback") {
            if      (key == "autoplay")             playback_.autoplay             = parse_bool(value, false);
            else if (key == "autoplay_limit")       playback_.autoplay_limit       = parse_bool(value, false);
            else if (key == "autoplay_limit_value") playback_.autoplay_limit_value = parse_int(value, 10);
            else if (key == "seek_seconds")         playback_.seek_seconds         = parse_int(value, 5);

        }
        // [Characters] and any other unknown sections are silently ignored.
    }

    validate();
}

// ---------------------------------------------------------------------------
// validate() — normalize after parse or before setters complete
// ---------------------------------------------------------------------------

void Config::validate() {
    // Playback — clamp to valid ranges (consistent with setter behavior)
    playback_.seek_seconds         = std::clamp(playback_.seek_seconds, 1, 300);
    playback_.autoplay_limit_value = std::clamp(playback_.autoplay_limit_value, 1, 9999);

    // Visualizer
    visualizer_.height = std::clamp(visualizer_.height, 1, 20);
    if (visualizer_.style.empty())
        visualizer_.style = "bars";

    // Keybindings — reject empty strings; revert to defaults
    const KeybindConfig defaults;
    auto fix_kb = [](std::string& val, const std::string& def) {
        if (val.empty()) val = def;
    };
    fix_kb(keybinds_.pause,         defaults.pause);
    fix_kb(keybinds_.search,        defaults.search);
    fix_kb(keybinds_.refresh,       defaults.refresh);
    fix_kb(keybinds_.quit,          defaults.quit);
    fix_kb(keybinds_.seek_forward,  defaults.seek_forward);
    fix_kb(keybinds_.seek_backward, defaults.seek_backward);
    fix_kb(keybinds_.queue_add,     defaults.queue_add);
    fix_kb(keybinds_.queue_remove,  defaults.queue_remove);
    fix_kb(keybinds_.queue_clear,   defaults.queue_clear);
}

// ---------------------------------------------------------------------------
// Runtime mutators
// ---------------------------------------------------------------------------

void Config::add_music_path(const std::string& path) {
    if (!path.empty()) {
        library_.music_paths.push_back(path);
        modified_ = true;
    }
}

void Config::remove_music_path(std::size_t index) {
    if (index < library_.music_paths.size()) {
        library_.music_paths.erase(library_.music_paths.begin() +
                                   static_cast<std::ptrdiff_t>(index));
        modified_ = true;
    }
}

void Config::set_music_paths(const std::vector<std::string>& paths) {
    if (library_.music_paths != paths) {
        library_.music_paths = paths;
        modified_ = true;
    }
}

void Config::set_autoplay(bool value) {
    if (playback_.autoplay != value) {
        playback_.autoplay = value;
        modified_ = true;
    }
}

void Config::set_autoplay_limit(bool value) {
    if (playback_.autoplay_limit != value) {
        playback_.autoplay_limit = value;
        modified_ = true;
    }
}

void Config::set_autoplay_limit_value(int value) {
    int clamped = std::clamp(value, 1, 9999);
    if (playback_.autoplay_limit_value != clamped) {
        playback_.autoplay_limit_value = clamped;
        modified_ = true;
    }
}

void Config::set_seek_seconds(int value) {
    int clamped = std::clamp(value, 1, 300);
    if (playback_.seek_seconds != clamped) {
        playback_.seek_seconds = clamped;
        modified_ = true;
    }
}

void Config::set_visualizer_enabled(bool value) {
    if (visualizer_.enabled != value) {
        visualizer_.enabled = value;
        modified_ = true;
    }
}

void Config::set_visualizer_height(int value) {
    int clamped = std::clamp(value, 1, 20);
    if (visualizer_.height != clamped) {
        visualizer_.height = clamped;
        modified_ = true;
    }
}

void Config::set_visualizer_style(const std::string& value) {
    if (!value.empty() && visualizer_.style != value) {
        visualizer_.style = value;
        modified_ = true;
    }
}

void Config::set_keybind(const std::string& action, const std::string& key) {
    if (key.empty()) return;

    bool changed = false;
    if      (action == "pause"         && keybinds_.pause         != key) { keybinds_.pause         = key; changed = true; }
    else if (action == "search"        && keybinds_.search        != key) { keybinds_.search        = key; changed = true; }
    else if (action == "refresh"       && keybinds_.refresh       != key) { keybinds_.refresh       = key; changed = true; }
    else if (action == "quit"          && keybinds_.quit          != key) { keybinds_.quit          = key; changed = true; }
    else if (action == "seek_forward"  && keybinds_.seek_forward  != key) { keybinds_.seek_forward  = key; changed = true; }
    else if (action == "seek_backward" && keybinds_.seek_backward != key) { keybinds_.seek_backward = key; changed = true; }
    else if (action == "queue_add"     && keybinds_.queue_add     != key) { keybinds_.queue_add     = key; changed = true; }
    else if (action == "queue_remove"  && keybinds_.queue_remove  != key) { keybinds_.queue_remove  = key; changed = true; }
    else if (action == "queue_clear"   && keybinds_.queue_clear   != key) { keybinds_.queue_clear   = key; changed = true; }

    if (changed) modified_ = true;
}

// ---------------------------------------------------------------------------
// save() — atomic write via temp file + rename
// ---------------------------------------------------------------------------

void Config::save() const {
    fs::path target(save_path_);
    std::error_code ec;

    // Ensure directory exists
    fs::create_directories(target.parent_path(), ec);
    if (ec) {
        std::cerr << "Warning: Could not create config directory: "
                  << target.parent_path() << " — " << ec.message() << std::endl;
        return;
    }

    fs::path tmp_path = target.parent_path() / ".txplay_config.tmp";

    // Write to temp file
    {
        std::ofstream out(tmp_path);
        if (!out.is_open()) {
            std::cerr << "Warning: Could not write config to: " << tmp_path << std::endl;
            return;
        }

        out << "# Txplay Configuration\n";
        out << "# File: ~/.config/txplay/config.txt\n\n";

        out << "[Library]\n";
        out << "# Directories scanned for audio files (MP3, WAV, FLAC).\n";
        out << "# Repeat this key for multiple locations. Tilde expansion is supported.\n";
        if (library_.music_paths.empty()) {
            out << "# music_path=~/Music\n";
        } else {
            for (const auto& p : library_.music_paths)
                out << "music_path=" << p << "\n";
        }
        out << "\n";

        out << "[Playback]\n";
        out << "# When true: automatically selects the next local track after the queue\n";
        out << "# empties. When false: playback stops when the queue is empty.\n";
        out << "# Queue tracks always play regardless of this setting.\n";
        out << "autoplay=" << (playback_.autoplay ? "true" : "false") << "\n";
        out << "# Limit the number of local tracks auto-played after queue exhaustion.\n";
        out << "autoplay_limit=" << (playback_.autoplay_limit ? "true" : "false") << "\n";
        out << "# Max local tracks to auto-play when autoplay_limit=true (1-9999).\n";
        out << "autoplay_limit_value=" << playback_.autoplay_limit_value << "\n";
        out << "# How many seconds each seek keypress moves (1-300).\n";
        out << "seek_seconds=" << playback_.seek_seconds << "\n";
        out << "\n";

        out << "[Visualizer]\n";
        out << "# Show the real-time FFT visualizer panel.\n";
        out << "enabled=" << (visualizer_.enabled ? "true" : "false") << "\n";
        out << "# Rendering style. Currently only \"bars\" is supported.\n";
        out << "style=" << visualizer_.style << "\n";
        out << "# Height of the visualizer in terminal rows (1-20).\n";
        out << "height=" << visualizer_.height << "\n";
        out << "\n";

        out << "[Keybindings]\n";
        out << "pause="         << keybinds_.pause         << "\n";
        out << "search="        << keybinds_.search        << "\n";
        out << "refresh="       << keybinds_.refresh       << "\n";
        out << "quit="          << keybinds_.quit          << "\n";
        out << "seek_forward="  << keybinds_.seek_forward  << "\n";
        out << "seek_backward=" << keybinds_.seek_backward << "\n";
        out << "queue_add="     << keybinds_.queue_add     << "\n";
        out << "queue_remove="  << keybinds_.queue_remove  << "\n";
        out << "queue_clear="   << keybinds_.queue_clear   << "\n";

        if (!out.good()) {
            std::cerr << "Warning: Error while writing config to: " << tmp_path << std::endl;
            return;
        }
    }

    // Atomic rename
    fs::rename(tmp_path, target, ec);
    if (ec) {
        std::cerr << "Warning: Could not finalize config save: " << ec.message() << std::endl;
        fs::remove(tmp_path, ec); // clean up temp on failure
        return;
    }

    modified_ = false;
}

// ---------------------------------------------------------------------------
// for_testing() — test-only factory that overrides the save path
// ---------------------------------------------------------------------------

// static
Config Config::for_testing(const std::string& config_file_path,
                           const std::string& save_path) {
    Config cfg(config_file_path);
    cfg.save_path_ = save_path;   // override before any save() call
    return cfg;
}

} // namespace txplay::config
