#include "Library.hpp"
#include "Scanner.hpp"

namespace txplay::library {

Library::Library() = default;

Library::~Library() {
    // A-03: request cooperative cancellation before joining.
    // The scanner thread checks scan_stop_requested_ between directory entries
    // and exits as soon as it observes the flag.  For local storage this means
    // the join completes within the time of at most one readdir/stat call
    // rather than waiting for the full traversal to finish.
    //
    // LIFECYCLE NOTE: The join is still synchronous.  Each individual syscall
    // (readdir, stat, realpath) runs to completion; the flag can only interrupt
    // the loop between entries.  On network-mounted paths a single blocked
    // syscall can still delay shutdown briefly, but this is now bounded to one
    // entry rather than the full scan.
    scan_stop_requested_.store(true, std::memory_order_relaxed);
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

    // A-03: reset the cancellation flag before starting a new scan.
    // Safe: is_scanning_ was false (CAS succeeded above), so no scanner thread
    // is running at this point.  We are the only writer of this flag right now.
    scan_stop_requested_.store(false, std::memory_order_relaxed);

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
    // Perform the heavy filesystem traversal completely lock-free.
    // A-03: pass the cancellation flag so Scanner can exit early between entries.
    ScanResult result = Scanner::scan(paths, scan_stop_requested_);

    // A-03: if the scan was cancelled, discard the partial result entirely.
    // Do NOT modify tracks_, last_errors_, or version_.  The existing library
    // contents remain intact so the UI continues to show the previous scan.
    if (!result.cancelled) {
        // Commit the completed scan result
        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            tracks_ = std::move(result.tracks);
            last_errors_ = std::move(result.errors);
            ++version_; // A-01: bump version inside the same lock as tracks_ replacement
        }
    }

    // Mark scan as complete regardless of cancellation.
    is_scanning_.store(false, std::memory_order_release);
}

} // namespace txplay::library
