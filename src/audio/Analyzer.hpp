#pragma once
#include <thread>
#include <atomic>
#include <memory>
#include <vector>
#include <complex>
#include <mutex>
#include "SpscRingBuffer.hpp"

namespace txplay::audio {

// ---------------------------------------------------------------------------
// Analyzer — runs on a dedicated background thread.
//
// Reads 1024-float (512 stereo-frame) PCM windows from the analysis ring
// buffer, applies a Hann window, runs a 512-point radix-2 FFT, maps the
// positive frequency bins into 32 logarithmically spaced bands, and
// exponentially smooths the result.
//
// Public accessors (all mutex-protected copies, safe to call from any thread):
//   get_latest_window()    — raw interleaved stereo PCM (1024 floats)
//   get_frequency_bands()  — 32 EMA-smoothed normalized band magnitudes [0,1]
//
// Both are updated at ~60 FPS on the analyzer thread.
// ---------------------------------------------------------------------------
class Analyzer {
public:
    static constexpr int   kNumBands   = 32;      // log-spaced frequency bands
    static constexpr float kFreqMin    = 40.0f;   // Hz — lowest band edge
    static constexpr float kFreqMax    = 18000.0f;// Hz — highest band edge
    static constexpr float kSampleRate = 44100.0f;
    static constexpr float kBandSmooth = 0.35f;   // EMA factor (0=raw, 1=frozen)

    explicit Analyzer(std::shared_ptr<SpscRingBuffer<float>> analysis_buffer);
    ~Analyzer();

    void stop();

    // Returns a copy of the latest raw PCM window (1024 interleaved stereo floats).
    std::vector<float> get_latest_window();

    // Returns a copy of the latest frequency band magnitudes.
    // kNumBands values in [0.0, 1.0], log-spaced from kFreqMin to kFreqMax.
    // Returns an empty vector until the first analysis window is processed.
    std::vector<float> get_frequency_bands();

private:
    void loop();

    // FFT helpers (used only on the analyzer thread — no locking needed).
    void build_hann_window(size_t n);
    void run_fft_and_update_bands(const std::vector<float>& stereo_pcm);
    static void fft_inplace(std::vector<std::complex<float>>& buf);

    std::shared_ptr<SpscRingBuffer<float>> analysis_buffer_;
    std::atomic<bool> stop_flag_{false};
    std::thread thread_;

    // Shared output — protected by data_mutex_.
    std::mutex data_mutex_;
    std::vector<float> latest_window_;
    std::vector<float> fft_bands_;   // length kNumBands, all in [0,1]

    // Working buffers — only accessed on the analyzer thread, no mutex needed.
    std::vector<float>                hann_window_;   // precomputed coefficients
    std::vector<std::complex<float>>  fft_buf_;       // reused every frame
    std::vector<float>                work_bands_;    // EMA state (thread-local to analyzer)
};

} // namespace txplay::audio
