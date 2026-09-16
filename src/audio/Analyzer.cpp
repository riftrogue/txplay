#include "Analyzer.hpp"
#include <chrono>

namespace txplay::audio {

Analyzer::Analyzer(std::shared_ptr<SpscRingBuffer<float>> analysis_buffer) 
    : analysis_buffer_(std::move(analysis_buffer)) 
{
    thread_ = std::thread(&Analyzer::loop, this);
}

Analyzer::~Analyzer() {
    stop();
}

void Analyzer::stop() {
    stop_flag_.store(true, std::memory_order_release);
    if (thread_.joinable()) {
        thread_.join();
    }
}

std::vector<float> Analyzer::get_latest_window() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return latest_window_;
}

void Analyzer::loop() {
    const size_t WINDOW_SIZE = 1024; // 512 frames * 2 channels = 1024 floats
    std::vector<float> temp_buf(WINDOW_SIZE);

    while (!stop_flag_.load(std::memory_order_acquire)) {
        size_t available = analysis_buffer_->read_available();
        if (available >= WINDOW_SIZE) {
            analysis_buffer_->read(temp_buf.data(), WINDOW_SIZE);
            
            // In the future: FFT processing goes here!
            
            {
                std::lock_guard<std::mutex> lock(data_mutex_);
                latest_window_ = temp_buf;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
        }
    }
}

} // namespace txplay::audio
