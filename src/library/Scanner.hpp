#pragma once

#include <vector>
#include <string>
#include <atomic>
#include "Track.hpp"

namespace txplay::library {

struct ScanResult {
    std::vector<Track> tracks;
    std::vector<std::string> errors;
    bool cancelled{false}; // A-03: true if scan was cut short by stop request
};

class Scanner {
public:
    // A-03: stop_requested is checked between directory entries so the caller
    // can cancel a long-running scan cooperatively. Passing a flag that is
    // always false gives the original non-cancellable behaviour.
    static ScanResult scan(const std::vector<std::string>& config_paths,
                           const std::atomic<bool>& stop_requested);

private:
    static bool is_supported_format(const std::string& extension);
    static Track create_track_from_path(const std::string& canonical_path, const std::string& filename);
};

} // namespace txplay::library
