#include <iostream>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include "Library.hpp"

using namespace txplay::library;
using namespace std::chrono_literals;

int main() {
    std::cout << "--- Library Test Runner ---" << std::endl;

    std::vector<std::string> paths = {
        "experiments/library-test/root1",
        "experiments/library-test/root1", // Duplicate root
        "experiments/library-test/root2",
        "experiments/library-test/root1/nested" // Overlapping root
    };

    namespace fs = std::filesystem;
    if (fs::exists("experiments/library-test")) {
        fs::remove_all("experiments/library-test");
    }
    fs::create_directories("experiments/library-test/root1/nested");
    fs::create_directories("experiments/library-test/root2");
    
    // Create dummy files
    auto touch = [](const std::string& p) { std::ofstream(p) << "dummy"; };
    touch("experiments/library-test/root1/a.mp3");
    touch("experiments/library-test/root1/nested/b.flac");
    touch("experiments/library-test/root2/c.wav");
    touch("experiments/library-test/root1/unsupported.txt");
    
    // Create symlinks
    std::error_code ec;
    fs::create_symlink(fs::absolute("experiments/library-test/root1/a.mp3"), "experiments/library-test/root2/a_link.mp3", ec);
    fs::create_directory_symlink(fs::absolute("experiments/library-test/root1"), "experiments/library-test/root2/root1_link", ec);

    Library lib;
    
    std::cout << "Starting async scan..." << std::endl;
    bool started = lib.scan_async(paths);
    assert(started);
    
    std::cout << "Rejecting concurrent scan..." << std::endl;
    bool rejected = !lib.scan_async(paths);
    assert(rejected);

    std::cout << "Waiting for scan to complete..." << std::endl;
    while (lib.is_scanning()) {
        std::this_thread::sleep_for(10ms);
    }

    auto tracks = lib.get_tracks();
    auto errors = lib.get_last_errors();

    std::cout << "Scan finished. Found " << tracks.size() << " tracks." << std::endl;
    std::cout << "Errors encountered: " << errors.size() << std::endl;
    for (const auto& e : errors) {
        std::cout << "  Error: " << e << std::endl;
    }

    // Output tracks
    for (size_t i = 0; i < tracks.size(); ++i) {
        const auto& t = tracks[i];
        std::cout << "[" << i << "] " << t.path << "\n"
                  << "    Title: " << t.title << "\n"
                  << "    Artist: " << t.artist << "\n"
                  << "    Album: " << t.album << std::endl;
    }

    // Assertions
    // We expect 3 valid files: a.mp3, b.flac, c.wav
    // We expect duplicate roots and overlapping roots to NOT produce duplicates.
    // We expect unsupported files to be ignored.
    // We expect symlinks to be ignored.
    
    // Check deterministic ordering and deduplication
    assert(tracks.size() == 3);
    
    // Metadata fallback check
    assert(tracks[0].title == "a");
    assert(tracks[0].artist == "Unknown Artist");
    assert(tracks[0].album == "Unknown Album");
    assert(tracks[0].duration_ms == 0);

    // Re-scan to prove deterministic ordering doesn't change
    lib.scan_async(paths);
    while (lib.is_scanning()) std::this_thread::sleep_for(10ms);
    
    auto tracks2 = lib.get_tracks();
    assert(tracks.size() == tracks2.size());
    for (size_t i = 0; i < tracks.size(); ++i) {
        assert(tracks[i].path == tracks2[i].path);
    }

    std::cout << "All library assertions passed!" << std::endl;
    return 0;
}
