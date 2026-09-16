#pragma once

#include <string>
#include <cstdint>

namespace txplay::library {

struct Track {
    // Identity
    std::string id;       // Canonical absolute path
    std::string path;     // Canonical absolute path (same as ID for MVP)
    std::string filename; // Base filename for display fallback

    // Metadata (Fallback for MVP)
    std::string title;
    std::string artist;
    std::string album;
    
    // Technical
    uint64_t duration_ms{0}; // Populated at playback
};

} // namespace txplay::library
