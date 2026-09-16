#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "AudioEngine.hpp"
#include "Analyzer.hpp"

using namespace txplay::audio;
using namespace std::chrono_literals;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <test.mp3|wav|flac>" << std::endl;
        return 1;
    }

    std::string file_path = argv[1];
    AudioEngine engine;

    std::cout << "Loading " << file_path << "..." << std::endl;
    if (!engine.play(file_path)) {
        std::cerr << "Failed to play." << std::endl;
        return 1;
    }

    std::cout << "Playing. Wait 2 seconds..." << std::endl;
    std::this_thread::sleep_for(2s);
    
    std::cout << "Position: " << engine.get_position_ms() << " ms" << std::endl;
    
    auto analyzer = engine.get_analyzer();
    auto window = analyzer->get_latest_window();
    std::cout << "Analyzer window size: " << window.size() << std::endl;
    if (!window.empty()) {
        std::cout << "First float sample: " << window[0] << std::endl;
    }

    std::cout << "Pausing..." << std::endl;
    engine.pause();
    std::this_thread::sleep_for(1s);

    std::cout << "Seeking while paused to 1500 ms..." << std::endl;
    engine.seek(1500);
    std::this_thread::sleep_for(500ms); // let seek process
    std::cout << "Position after seek: " << engine.get_position_ms() << " ms" << std::endl;

    std::cout << "Resuming..." << std::endl;
    engine.resume();
    std::this_thread::sleep_for(1s);

    std::cout << "Rapid seeks..." << std::endl;
    engine.seek(500);
    std::this_thread::sleep_for(10ms);
    engine.seek(2000);
    std::this_thread::sleep_for(10ms);
    engine.seek(1000);
    std::this_thread::sleep_for(1s);
    std::cout << "Position after rapid seeks: " << engine.get_position_ms() << " ms" << std::endl;

    std::cout << "Track replacement: Loading test.flac over current track..." << std::endl;
    if (!engine.play("experiments/audio-test/test.flac")) {
        std::cerr << "Failed to play FLAC." << std::endl;
        return 1;
    }
    std::cout << "Playing FLAC for 2 seconds..." << std::endl;
    std::this_thread::sleep_for(2s);
    
    std::cout << "Waiting for EOF of FLAC..." << std::endl;
    while (!engine.is_track_finished()) {
        std::this_thread::sleep_for(100ms);
    }

    std::cout << "Track completed naturally." << std::endl;
    engine.stop();
    
    std::cout << "Tests passed for " << file_path << " and FLAC replacement!" << std::endl;
    return 0;
}
