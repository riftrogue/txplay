#pragma once

#include <string>
#include <atomic>
#include <memory>
#include <cstdint>
#include "SpscRingBuffer.hpp"

namespace txplay::audio {

class AudioDevice;
class DecoderThread;
class Analyzer;

enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool play(const std::string& path);
    void pause();
    void resume();
    void stop();
    void seek(uint64_t milliseconds);
    void set_volume(float volume);

    PlaybackState get_state() const;
    uint64_t get_position_ms() const;
    uint64_t get_duration_ms() const;
    bool is_track_finished() const;

    std::shared_ptr<Analyzer> get_analyzer() const { return analyzer_; }

private:
    void stop_internal();

    std::shared_ptr<SpscRingBuffer<float>> playback_buffer_;
    std::shared_ptr<SpscRingBuffer<float>> analysis_buffer_;

    std::unique_ptr<AudioDevice> device_;
    std::unique_ptr<DecoderThread> decoder_thread_;
    std::shared_ptr<Analyzer> analyzer_;

    std::atomic<bool> is_playing_{false};
    std::atomic<float> volume_{1.0f};
    
    std::atomic<uint64_t> seek_request_ms_{UINT64_MAX};
    std::atomic<uint64_t> flush_epoch_{0};
    std::atomic<uint64_t> ack_epoch_{0};
};

} // namespace txplay::audio
