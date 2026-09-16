#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <cstdint>
#include "SpscRingBuffer.hpp"
#include "../../third_party/miniaudio/miniaudio.h"

namespace txplay::audio {

class DecoderThread {
public:
    DecoderThread(
        const std::string& path,
        std::shared_ptr<SpscRingBuffer<float>> playback_buffer,
        std::atomic<bool>& is_playing,
        std::atomic<uint64_t>& seek_request_ms,
        std::atomic<uint64_t>& flush_epoch,
        std::atomic<uint64_t>& ack_epoch
    );
    ~DecoderThread();

    bool is_valid() const { return valid_; }
    void stop_and_join();

    bool has_eof() const { return eof_.load(std::memory_order_relaxed); }
    uint64_t get_position_ms() const;
    uint64_t get_duration_ms() const;

private:
    void loop();

    std::string path_;
    std::shared_ptr<SpscRingBuffer<float>> playback_buffer_;
    
    std::atomic<bool>& is_playing_;
    std::atomic<uint64_t>& seek_request_ms_;
    std::atomic<uint64_t>& flush_epoch_;
    std::atomic<uint64_t>& ack_epoch_;

    std::atomic<bool> stop_flag_{false};
    std::atomic<bool> eof_{false};
    std::atomic<uint64_t> current_frame_{0};
    bool valid_{false};

    ma_decoder decoder_;
    std::thread thread_;
};

} // namespace txplay::audio
