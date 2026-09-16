#include "AudioDevice.hpp"

namespace txplay::audio {

AudioDevice::AudioDevice(
    std::shared_ptr<SpscRingBuffer<float>> playback_buffer,
    std::shared_ptr<SpscRingBuffer<float>> analysis_buffer,
    std::atomic<bool>& is_playing,
    std::atomic<float>& volume,
    std::atomic<uint64_t>& flush_epoch,
    std::atomic<uint64_t>& ack_epoch
) : playback_buffer_(std::move(playback_buffer)),
    analysis_buffer_(std::move(analysis_buffer)),
    is_playing_(is_playing),
    volume_(volume),
    flush_epoch_(flush_epoch),
    ack_epoch_(ack_epoch)
{
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = ma_format_f32; 
    deviceConfig.playback.channels = 2;
    deviceConfig.sampleRate        = 44100;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = this;

    if (ma_device_init(NULL, &deviceConfig, &device_) == MA_SUCCESS) {
        initialized_ = true;
    }
}

AudioDevice::~AudioDevice() {
    stop();
    if (initialized_) {
        ma_device_uninit(&device_);
    }
}

bool AudioDevice::start() {
    if (initialized_) {
        return ma_device_start(&device_) == MA_SUCCESS;
    }
    return false;
}

void AudioDevice::stop() {
    if (initialized_) {
        ma_device_stop(&device_);
    }
}

bool AudioDevice::is_playback_buffer_empty() const {
    return playback_buffer_->read_available() == 0;
}

void AudioDevice::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    AudioDevice* self = static_cast<AudioDevice*>(pDevice->pUserData);
    if (!self) return;

    // 1. Process sync epochs for lock-free seek and track replacement flush
    uint64_t flush = self->flush_epoch_.load(std::memory_order_acquire);
    if (flush != self->ack_epoch_.load(std::memory_order_relaxed)) {
        // Safe: AudioDevice callback is the ONLY consumer of the Playback Buffer.
        self->playback_buffer_->flush_consumer_only();
        self->ack_epoch_.store(flush, std::memory_order_release);
    }

    // 2. Set volume
    float vol = self->volume_.load(std::memory_order_relaxed);
    ma_device_set_master_volume(pDevice, vol);

    // 3. Output Silence if paused or empty
    ma_uint32 floatsNeeded = frameCount * 2; // Stereo
    
    if (!self->is_playing_.load(std::memory_order_acquire)) {
        ma_silence_pcm_frames(pOutput, frameCount, ma_format_f32, 2);
        return;
    }

    size_t available = self->playback_buffer_->read_available();
    if (available == 0) {
        ma_silence_pcm_frames(pOutput, frameCount, ma_format_f32, 2);
        return;
    }

    // 4. Read from Playback Buffer
    size_t floatsToRead = std::min(static_cast<size_t>(floatsNeeded), available);
    float* outBuf = static_cast<float*>(pOutput);
    
    self->playback_buffer_->read(outBuf, floatsToRead);

    // Fill the remainder with silence if underrun
    if (floatsToRead < floatsNeeded) {
        ma_uint32 framesRead = floatsToRead / 2;
        ma_uint32 framesRemaining = frameCount - framesRead;
        float* pTail = outBuf + floatsToRead;
        ma_silence_pcm_frames(pTail, framesRemaining, ma_format_f32, 2);
    }

    // 5. Tap into Analysis Buffer
    size_t ana_avail = self->analysis_buffer_->write_available();
    
    // Ensure we only write complete stereo frames (multiples of 2 floats)
    ana_avail = (ana_avail / 2) * 2;
    size_t ana_to_write = std::min(floatsToRead, ana_avail);
    
    if (ana_to_write > 0) {
        self->analysis_buffer_->write(outBuf, ana_to_write);
    }
    // Note: If floatsToRead > ana_avail, the extra frames are simply dropped from analysis, 
    // satisfying SPSC constraints (we never modify the read index).

    (void)pInput;
}

} // namespace txplay::audio
