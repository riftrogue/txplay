#pragma once

#include <string>
#include <vector>
#include <map>

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

    std::vector<std::string> get_music_paths() const { return music_paths_; }
    VisualizerConfig get_visualizer() const { return visualizer_; }
    NavigationConfig get_navigation() const { return navigation_; }
    CharacterConfig get_characters() const { return characters_; }

private:
    std::vector<std::string> music_paths_;
    VisualizerConfig visualizer_;
    NavigationConfig navigation_;
    CharacterConfig characters_;
};

} // namespace txplay::config
