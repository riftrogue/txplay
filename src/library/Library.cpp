#include "Library.hpp"
#include "Scanner.hpp"

namespace txplay::library {

Library::Library() = default;

Library::~Library() {
    // Join the scanner thread before destruction.
    // LIFECYCLE NOTE: This join is intentionally synchronous and has no
    // cooperative cancellation mechanism. The wait duration is bounded by
    // the filesystem traversal time of the configured music paths.
    // For typical local directories this completes in under one second.
    // Callers using very large network-mounted paths should be aware that
    // exit may block until the scan finishes.
    if (scanner_thread_.joinable()) {
        scanner_thread_.join();
    }
}

bool Library::scan_async(const std::vector<std::string>& config_paths) {
    bool expected = false;
    // Atomically claim the scanning state. If true, a scan is already running.
    if (!is_scanning_.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
        return false;
    }

    // Clean up previous thread if it finished but wasn't joined
    if (scanner_thread_.joinable()) {
        scanner_thread_.join();
    }

    scanner_thread_ = std::thread(&Library::scan_worker, this, config_paths);
    return true;
}

bool Library::is_scanning() const {
    return is_scanning_.load(std::memory_order_acquire);
}

std::vector<Track> Library::get_tracks() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return tracks_;
}

std::vector<std::string> Library::get_last_errors() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return last_errors_;
}

// A-01: returns the current version counter under data_mutex_.
// Application compares this against its cached library_version_ to detect
// whether library_snapshot_ needs refreshing.
uint64_t Library::get_version() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return version_;
}

void Library::scan_worker(std::vector<std::string> paths) {
    // Perform the heavy filesystem traversal completely lock-free
    ScanResult result = Scanner::scan(paths);

    // Swap the newly built data into the active library state
    {
        std::lock_guard<std::mutex> lock(data_mutex_);
        tracks_ = std::move(result.tracks);
        last_errors_ = std::move(result.errors);
        ++version_; // A-01: bump version inside the same lock as tracks_ replacement
    }

    // Mark scan as complete
    is_scanning_.store(false, std::memory_order_release);
}

} // namespace txplay::library
