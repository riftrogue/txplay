#include "DecoderThread.hpp"
#include <iostream>
#include <chrono>

namespace txplay::audio {

DecoderThread::DecoderThread(
    const std::string& path,
    std::shared_ptr<SpscRingBuffer<float>> playback_buffer,
    std::atomic<bool>& is_playing,
    std::atomic<uint64_t>& seek_request_ms,
    std::atomic<uint64_t>& flush_epoch,
    std::atomic<uint64_t>& ack_epoch
) : path_(path),
    playback_buffer_(std::move(playback_buffer)),
    is_playing_(is_playing),
    seek_request_ms_(seek_request_ms),
    flush_epoch_(flush_epoch),
    ack_epoch_(ack_epoch)
{
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);
    if (ma_decoder_init_file(path_.c_str(), &decoderConfig, &decoder_) == MA_SUCCESS) {
        valid_ = true;
        thread_ = std::thread(&DecoderThread::loop, this);
    }
}

DecoderThread::~DecoderThread() {
    stop_and_join();
}

void DecoderThread::stop_and_join() {
    stop_flag_.store(true, std::memory_order_release);
    if (thread_.joinable()) {
        thread_.join();
    }
    if (valid_) {
        ma_decoder_uninit(&decoder_);
        valid_ = false;
    }
}

uint64_t DecoderThread::get_position_ms() const {
    uint64_t frames = current_frame_.load(std::memory_order_relaxed);
    return (frames * 1000) / 44100;
}

uint64_t DecoderThread::get_duration_ms() const {
    if (!valid_) return 0;
    ma_uint64 length_frames = 0;
    if (ma_decoder_get_length_in_pcm_frames(const_cast<ma_decoder*>(&decoder_), &length_frames) == MA_SUCCESS) {
        return (length_frames * 1000) / 44100;
    }
    return 0;
}

void DecoderThread::loop() {
    const size_t CHUNK_FRAMES = 1024; // 2048 floats
    const size_t CHUNK_FLOATS = CHUNK_FRAMES * 2;
    std::vector<float> decode_buf(CHUNK_FLOATS);

    while (!stop_flag_.load(std::memory_order_acquire)) {
        // 1. Check for seek request
        uint64_t target_ms = seek_request_ms_.exchange(UINT64_MAX, std::memory_order_acquire);
        if (target_ms != UINT64_MAX) {
            ma_uint64 target_frame = (target_ms * 44100) / 1000;

            ma_decoder_seek_to_pcm_frame(&decoder_, target_frame);
            current_frame_.store(target_frame, std::memory_order_relaxed);
            eof_.store(false, std::memory_order_relaxed);

            // Trigger flush epoch
            uint64_t next_epoch = flush_epoch_.load(std::memory_order_relaxed) + 1;
            flush_epoch_.store(next_epoch, std::memory_order_release);

            // Wait for callback to acknowledge
            while (ack_epoch_.load(std::memory_order_acquire) != next_epoch && !stop_flag_.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            // Note: If a new seek arrived while we were waiting for ack, it is perfectly preserved
            // in seek_request_ms_ and will be processed on the next loop iteration.
            continue; 
        }

        // 2. If EOF, just sleep
        if (eof_.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 3. Decode a chunk if there's space
        size_t available = playback_buffer_->write_available();
        if (available >= CHUNK_FLOATS) {
            ma_uint64 framesRead = 0;
            ma_result res = ma_decoder_read_pcm_frames(&decoder_, decode_buf.data(), CHUNK_FRAMES, &framesRead);
            
            if (framesRead > 0) {
                playback_buffer_->write(decode_buf.data(), framesRead * 2);
                current_frame_.fetch_add(framesRead, std::memory_order_relaxed);
            }

            if (framesRead < CHUNK_FRAMES) {
                eof_.store(true, std::memory_order_release);
            }
        } else {
            // Buffer full, yield
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

} // namespace txplay::audio
