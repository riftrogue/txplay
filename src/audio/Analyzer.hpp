#pragma once
#include <thread>
#include <atomic>
#include <memory>
#include <vector>
#include <mutex>
#include "SpscRingBuffer.hpp"

namespace txplay::audio {

class Analyzer {
public:
    explicit Analyzer(std::shared_ptr<SpscRingBuffer<float>> analysis_buffer);
    ~Analyzer();

    void stop();
    
    // Returns a copy of the latest window. 
    std::vector<float> get_latest_window();

private:
    void loop();

    std::shared_ptr<SpscRingBuffer<float>> analysis_buffer_;
    std::atomic<bool> stop_flag_{false};
    std::thread thread_;

    std::mutex data_mutex_;
    std::vector<float> latest_window_;
};

} // namespace txplay::audio
