#include "Config.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

namespace txplay::config {

static std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }
    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    return std::string(start, end + 1);
}

Config::Config(const std::string& config_file_path) {
    std::ifstream file(config_file_path);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file: " << config_file_path << std::endl;
        return;
    }

    std::string current_section = "";
    std::string line;
    
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            continue;
        }

        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));

            if (current_section == "Library") {
                if (key == "music_path") {
                    music_paths_.push_back(value);
                }
            } else if (current_section == "Visualizer") {
                if (key == "enabled") visualizer_.enabled = (value == "true" || value == "1");
                else if (key == "style") visualizer_.style = value;
                else if (key == "height") {
                    try { visualizer_.height = std::stoi(value); } catch(...) {}
                }
            } else if (current_section == "Navigation") {
                if (key == "play") navigation_.play = value;
                else if (key == "pause") navigation_.pause = value;
                else if (key == "search") navigation_.search = value;
                else if (key == "refresh") navigation_.refresh = value;
                else if (key == "quit") navigation_.quit = value;
                else if (key == "seek_forward") navigation_.seek_forward = value;
                else if (key == "seek_backward") navigation_.seek_backward = value;
                else if (key == "queue_add") navigation_.queue_add = value;
                else if (key == "queue_remove") navigation_.queue_remove = value;
                else if (key == "queue_clear") navigation_.queue_clear = value;
            } else if (current_section == "Characters") {
                if (key == "top_left") characters_.top_left = value;
                else if (key == "top_right") characters_.top_right = value;
                else if (key == "bottom_left") characters_.bottom_left = value;
                else if (key == "bottom_right") characters_.bottom_right = value;
                else if (key == "horizontal") characters_.horizontal = value;
                else if (key == "vertical") characters_.vertical = value;
            } else if (current_section == "Playback") {
                if (key == "autoplay") playback_.autoplay = (value == "true" || value == "1");
            }
        }
    }
}

} // namespace txplay::config
