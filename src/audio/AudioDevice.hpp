#pragma once
#include <atomic>
#include <memory>
#include "SpscRingBuffer.hpp"
#include "../../third_party/miniaudio/miniaudio.h"

namespace txplay::audio {

class AudioDevice {
public:
    AudioDevice(
        std::shared_ptr<SpscRingBuffer<float>> playback_buffer,
        std::shared_ptr<SpscRingBuffer<float>> analysis_buffer,
        std::atomic<bool>& is_playing,
        std::atomic<float>& volume,
        std::atomic<uint64_t>& flush_epoch,
        std::atomic<uint64_t>& ack_epoch
    );
    ~AudioDevice();

    bool start();
    void stop();
    
    // Check if the playback buffer is currently empty
    bool is_playback_buffer_empty() const;

private:
    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    std::shared_ptr<SpscRingBuffer<float>> playback_buffer_;
    std::shared_ptr<SpscRingBuffer<float>> analysis_buffer_;
    
    std::atomic<bool>& is_playing_;
    std::atomic<float>& volume_;
    std::atomic<uint64_t>& flush_epoch_;
    std::atomic<uint64_t>& ack_epoch_;

    ma_device device_;
    bool initialized_{false};
};

} // namespace txplay::audio
