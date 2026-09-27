#pragma once

#include <string>
#include <cstdint>

namespace txplay::library {

struct Track {
    // Identity
    // IMPORTANT: Track identity is the canonical absolute filesystem path.
    // 'id' and 'path' are always identical for the current implementation.
    // Consequence: renaming or moving a file changes its identity. Any queue
    // entries referencing the old path will be treated as missing tracks and
    // skipped on the next advance. This is an intentional MVP tradeoff.
    std::string id;       // Canonical absolute path (track identity)
    std::string path;     // Canonical absolute path (same as ID)
    std::string filename; // Base filename for display fallback

    // Metadata (Fallback for MVP)
    std::string title;
    std::string artist;
    std::string album;
    
    // Technical
    uint64_t duration_ms{0}; // Populated at playback
};

} // namespace txplay::library
