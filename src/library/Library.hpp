#pragma once

#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include "Track.hpp"

namespace txplay::library {

class Library {
public:
    Library();
    ~Library();

    // Prevent copying
    Library(const Library&) = delete;
    Library& operator=(const Library&) = delete;

    // Asynchronously scan the provided paths
    // If a scan is already running, this rejects the new scan and returns false.
    bool scan_async(const std::vector<std::string>& config_paths);

    bool is_scanning() const;

    // Thread-safe access to results
    std::vector<Track> get_tracks() const;
    std::vector<std::string> get_last_errors() const;

private:
    void scan_worker(std::vector<std::string> paths);

    std::atomic<bool> is_scanning_{false};
    std::thread scanner_thread_;

    mutable std::mutex data_mutex_;
    std::vector<Track> tracks_;
    std::vector<std::string> last_errors_;
};

} // namespace txplay::library
