#pragma once

#include <vector>
#include <string>
#include "Track.hpp"

namespace txplay::library {

struct ScanResult {
    std::vector<Track> tracks;
    std::vector<std::string> errors;
};

class Scanner {
public:
    static ScanResult scan(const std::vector<std::string>& config_paths);

private:
    static bool is_supported_format(const std::string& extension);
    static Track create_track_from_path(const std::string& canonical_path, const std::string& filename);
};

} // namespace txplay::library
