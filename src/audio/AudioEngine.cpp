#include "AudioEngine.hpp"
#include "DecoderThread.hpp"
#include "AudioDevice.hpp"
#include "Analyzer.hpp"

namespace txplay::audio {

AudioEngine::AudioEngine() {
    playback_buffer_ = std::make_shared<SpscRingBuffer<float>>(44100);
    analysis_buffer_ = std::make_shared<SpscRingBuffer<float>>(8192);

    analyzer_ = std::make_shared<Analyzer>(analysis_buffer_);
    
    device_ = std::make_unique<AudioDevice>(
        playback_buffer_, 
        analysis_buffer_, 
        is_playing_, 
        volume_, 
        flush_epoch_, 
        ack_epoch_
    );
    
    device_->start();
}

AudioEngine::~AudioEngine() {
    stop_internal();
    if (device_) {
        device_->stop();
    }
    if (analyzer_) {
        analyzer_->stop();
    }
}

void AudioEngine::stop_internal() {
    if (decoder_thread_) {
        decoder_thread_->stop_and_join();
        decoder_thread_.reset();
    }
    
    // UI Thread safely requests a flush via epoch synchronization.
    // At this point, the DecoderThread is completely joined and dead.
    // We can safely act as the producer of the flush_epoch_ to flush the playback buffer.
    uint64_t next_epoch = flush_epoch_.load(std::memory_order_relaxed) + 1;
    flush_epoch_.store(next_epoch, std::memory_order_release);
    
    // Wait for the still-running callback to observe and ack the epoch.
    while (ack_epoch_.load(std::memory_order_acquire) != next_epoch) {
        std::this_thread::yield();
    }
    
    seek_request_ms_.store(UINT64_MAX, std::memory_order_relaxed);
    is_playing_.store(false, std::memory_order_release);
}

bool AudioEngine::play(const std::string& path) {
    stop_internal();
    
    decoder_thread_ = std::make_unique<DecoderThread>(
        path,
        playback_buffer_,
        is_playing_,
        seek_request_ms_,
        flush_epoch_,
        ack_epoch_
    );
    
    if (!decoder_thread_->is_valid()) {
        decoder_thread_.reset();
        return false;
    }
    
    is_playing_.store(true, std::memory_order_release);
    return true;
}

void AudioEngine::pause() {
    if (decoder_thread_ && is_playing_.load(std::memory_order_acquire)) {
        is_playing_.store(false, std::memory_order_release);
    }
}

void AudioEngine::resume() {
    if (decoder_thread_ && !is_playing_.load(std::memory_order_acquire)) {
        is_playing_.store(true, std::memory_order_release);
    }
}

void AudioEngine::stop() {
    stop_internal();
}

void AudioEngine::seek(uint64_t milliseconds) {
    if (decoder_thread_) {
        seek_request_ms_.store(milliseconds, std::memory_order_release);
    }
}

void AudioEngine::set_volume(float volume) {
    volume_.store(volume, std::memory_order_release);
}

PlaybackState AudioEngine::get_state() const {
    if (!decoder_thread_) return PlaybackState::Stopped;
    return is_playing_.load(std::memory_order_acquire) ? PlaybackState::Playing : PlaybackState::Paused;
}

uint64_t AudioEngine::get_position_ms() const {
    if (decoder_thread_) {
        return decoder_thread_->get_position_ms();
    }
    return 0;
}

uint64_t AudioEngine::get_duration_ms() const {
    if (decoder_thread_) {
        return decoder_thread_->get_duration_ms();
    }
    return 0;
}

bool AudioEngine::is_track_finished() const {
    if (!decoder_thread_) return false;
    return decoder_thread_->has_eof() && device_->is_playback_buffer_empty();
}

} // namespace txplay::audio
