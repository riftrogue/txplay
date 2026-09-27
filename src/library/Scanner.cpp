#include "Scanner.hpp"
#include "MetadataReader.hpp"
#include "common/PathUtils.hpp"
#include <filesystem>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace txplay::library {
namespace fs = std::filesystem;

bool Scanner::is_supported_format(const std::string& extension) {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
    return ext == ".mp3" || ext == ".wav" || ext == ".flac";
}

Track Scanner::create_track_from_path(const std::string& canonical_path, const std::string& filename) {
    Track track;
    track.id = canonical_path;
    track.path = canonical_path;
    track.filename = filename;
    track.duration_ms = 0; // Deferred — obtained via AudioEngine at playback

    // Attempt to read embedded metadata (ID3, Vorbis comments, etc.) via TagLib.
    // MetadataReader returns empty strings for absent or unreadable tags.
    MetadataResult meta = MetadataReader::read(canonical_path);

    // title: embedded tag → filename stem fallback
    fs::path p(filename);
    track.title = meta.title.empty() ? p.stem().string() : meta.title;

    // artist: embedded tag → hardcoded fallback
    track.artist = meta.artist.empty() ? "Unknown Artist" : meta.artist;

    // album: embedded tag → hardcoded fallback
    track.album = meta.album.empty() ? "Unknown Album" : meta.album;

    return track;
}

// A-03: stop_requested is checked once per directory entry (after the iterator
// has advanced but before per-file stat/canonical work).  Individual syscalls
// (readdir, stat, realpath) run to completion and cannot be interrupted by a
// flag; cancellation is therefore best-effort between entries.
// If cancellation is detected, result.cancelled is set to true and the
// function returns immediately with whatever partial result has been built so
// far.  Library::scan_worker() inspects the flag and discards the partial
// result rather than committing it.
ScanResult Scanner::scan(const std::vector<std::string>& config_paths,
                         const std::atomic<bool>& stop_requested) {
    ScanResult result;
    std::unordered_set<std::string> seen_canonical_paths;

    for (const auto& raw_path : config_paths) {
        // A-03: check before starting traversal of each configured root.
        if (stop_requested.load(std::memory_order_relaxed)) {
            result.cancelled = true;
            return result;
        }

        std::string expanded = txplay::common::expand_tilde(raw_path);
        fs::path root_path(expanded);

        std::error_code ec;
        if (!fs::exists(root_path, ec)) {
            result.errors.push_back("Configured path does not exist: " + expanded);
            continue;
        }

        auto opts = fs::directory_options::skip_permission_denied;
        
        for (auto it = fs::recursive_directory_iterator(root_path, opts, ec); 
             it != fs::recursive_directory_iterator(); 
             it.increment(ec)) {
            
            // A-03: check once per directory entry, after iterator has advanced.
            // This is the finest-grained point where a check is cheap; it sits
            // before any per-entry stat() or realpath() work.
            if (stop_requested.load(std::memory_order_relaxed)) {
                result.cancelled = true;
                return result;
            }

            if (ec) {
                result.errors.push_back("Error traversing: " + ec.message());
                ec.clear();
                continue;
            }

            const auto& entry = *it;
            
            // Ignore symlinks entirely as per architecture decision
            if (entry.is_symlink(ec)) {
                continue;
            }
            if (ec) { ec.clear(); continue; }
            
            if (entry.is_regular_file(ec)) {
                std::string ext = entry.path().extension().string();
                if (is_supported_format(ext)) {
                    // Get canonical path
                    std::error_code canonical_ec;
                    fs::path canonical_p = fs::canonical(entry.path(), canonical_ec);
                    
                    if (canonical_ec) {
                        result.errors.push_back("Failed to canonicalize: " + entry.path().string());
                        continue;
                    }

                    std::string canonical_str = canonical_p.string();
                    
                    // Deduplicate
                    if (seen_canonical_paths.find(canonical_str) == seen_canonical_paths.end()) {
                        seen_canonical_paths.insert(canonical_str);
                        result.tracks.push_back(
                            create_track_from_path(canonical_str, entry.path().filename().string())
                        );
                    }
                }
            }
            if (ec) { ec.clear(); }
        }
    }

    // A-03: final check after traversal but before sort.  Avoids sorting a
    // result that is about to be discarded, and catches cancellations that
    // arrived while iterating the last directory root.
    if (stop_requested.load(std::memory_order_relaxed)) {
        result.cancelled = true;
        return result;
    }

    // Deterministic ordering by canonical path
    std::sort(result.tracks.begin(), result.tracks.end(), [](const Track& a, const Track& b) {
        return a.path < b.path;
    });

    return result;
}

} // namespace txplay::library

