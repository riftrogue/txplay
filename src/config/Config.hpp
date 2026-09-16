#pragma once

#include <string>
#include <vector>
#include <map>
#include "common/PathUtils.hpp"

namespace txplay::config {

struct VisualizerConfig {
    bool enabled{true};
    std::string style{"bars"};
    int height{6};
};

struct NavigationConfig {
    std::string play{"enter"};
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

struct PlaybackConfig {
    bool autoplay{false};
};

struct CharacterConfig {
    std::string top_left{"╭"};
    std::string top_right{"╮"};
    std::string bottom_left{"╰"};
    std::string bottom_right{"╯"};
    std::string horizontal{"─"};
    std::string vertical{"│"};
};

class Config {
public:
    explicit Config(const std::string& config_file_path);

    // Resolves the user config path in priority order:
    //   1. ~/.config/txplay/config.txt  (installed user config)
    //   2. ./config.txt                 (dev fallback)
    static std::string resolve_user_config_path();

    std::vector<std::string> get_music_paths() const { return music_paths_; }
    VisualizerConfig get_visualizer() const { return visualizer_; }
    NavigationConfig get_navigation() const { return navigation_; }
    CharacterConfig get_characters() const { return characters_; }
    PlaybackConfig get_playback() const { return playback_; }

private:
    std::vector<std::string> music_paths_;
    VisualizerConfig visualizer_;
    NavigationConfig navigation_;
    CharacterConfig characters_;
    PlaybackConfig playback_;
};

} // namespace txplay::config
