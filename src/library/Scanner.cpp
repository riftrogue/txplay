#include "Scanner.hpp"
#include <filesystem>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace txplay::library {
namespace fs = std::filesystem;

std::string Scanner::expand_tilde(const std::string& path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }
    const char* home = std::getenv("HOME");
    if (!home) {
        return path; // Fallback if HOME is not set
    }
    if (path.length() == 1) {
        return home;
    }
    if (path[1] == '/') {
        return std::string(home) + path.substr(1);
    }
    return path;
}

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
    
    // Fallback metadata
    fs::path p(filename);
    track.title = p.stem().string();
    track.artist = "Unknown Artist";
    track.album = "Unknown Album";
    track.duration_ms = 0; // Deferred
    
    return track;
}

ScanResult Scanner::scan(const std::vector<std::string>& config_paths) {
    ScanResult result;
    std::unordered_set<std::string> seen_canonical_paths;

    for (const auto& raw_path : config_paths) {
        std::string expanded = expand_tilde(raw_path);
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

    // Deterministic ordering by canonical path
    std::sort(result.tracks.begin(), result.tracks.end(), [](const Track& a, const Track& b) {
        return a.path < b.path;
    });

    return result;
}

} // namespace txplay::library
