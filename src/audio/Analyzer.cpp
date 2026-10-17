#include "Analyzer.hpp"
#include <chrono>
#include <cmath>
#include <algorithm>

namespace txplay::audio {

// ---------------------------------------------------------------------------
// FFT — in-place Cooley-Tukey radix-2 DIT
// buf.size() must be a power of 2.
// ---------------------------------------------------------------------------
/*static*/ void Analyzer::fft_inplace(std::vector<std::complex<float>>& buf) {
    const size_t N = buf.size();

    // Bit-reversal permutation
    for (size_t i = 1, j = 0; i < N; ++i) {
        size_t bit = N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(buf[i], buf[j]);
    }

    // Butterfly stages
    for (size_t len = 2; len <= N; len <<= 1) {
        const float angle = -2.0f * static_cast<float>(M_PI) / static_cast<float>(len);
        const std::complex<float> w_step(std::cos(angle), std::sin(angle));
        for (size_t i = 0; i < N; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            const size_t half = len >> 1;
            for (size_t k = 0; k < half; ++k) {
                const std::complex<float> u = buf[i + k];
                const std::complex<float> v = buf[i + k + half] * w;
                buf[i + k]        = u + v;
                buf[i + k + half] = u - v;
                w *= w_step;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Precompute Hann window coefficients for n points.
// w[i] = 0.5 * (1 - cos(2*pi*i / (n-1)))
// ---------------------------------------------------------------------------
void Analyzer::build_hann_window(size_t n) {
    hann_window_.resize(n);
    for (size_t i = 0; i < n; ++i) {
        hann_window_[i] = 0.5f * (1.0f - std::cos(
            2.0f * static_cast<float>(M_PI) * static_cast<float>(i)
            / static_cast<float>(n > 1 ? n - 1 : 1)));
    }
}

// ---------------------------------------------------------------------------
// run_fft_and_update_bands
//
// 1. Mix stereo PCM to mono, apply Hann window.
// 2. Run in-place FFT.
// 3. Compute magnitude of each positive bin (bins 1 .. N/2-1; skip DC).
// 4. Map bins into kNumBands logarithmically spaced bands.
// 5. Normalise each band to [0, 1] and apply EMA smoothing.
// 6. Write result into fft_bands_ (caller holds no mutex — this runs on the
//    analyzer thread; the mutex is taken afterward before publishing).
//
// Called exclusively from loop() — no locking needed on working buffers.
// ---------------------------------------------------------------------------
void Analyzer::run_fft_and_update_bands(const std::vector<float>& stereo_pcm) {
    const size_t N = fft_buf_.size(); // 512 mono frames

    // Step 1: stereo-to-mono mix + Hann window into fft_buf_
    for (size_t i = 0; i < N; ++i) {
        const size_t l = i * 2;
        const size_t r = i * 2 + 1;
        float mono = 0.0f;
        if (l < stereo_pcm.size() && r < stereo_pcm.size()) {
            mono = (stereo_pcm[l] + stereo_pcm[r]) * 0.5f;
        }
        fft_buf_[i] = std::complex<float>(mono * hann_window_[i], 0.0f);
    }

    // Step 2: FFT
    fft_inplace(fft_buf_);

    // Step 3 + 4 + 5: map positive bins -> log bands -> normalise -> EMA
    const float bin_hz    = kSampleRate / static_cast<float>(N);
    const float log_ratio = std::log(kFreqMax / kFreqMin);
    // Normalisation divisor: full-amplitude Hann-windowed sine -> peak bin ~= N/4.
    const float norm_div  = static_cast<float>(N) * 0.25f;

    // work_bands_ holds the EMA state between frames (analyzer-thread-only).
    // Initialise on first call.
    if (work_bands_.size() != static_cast<size_t>(kNumBands)) {
        work_bands_.assign(kNumBands, 0.0f);
    }

    for (int b = 0; b < kNumBands; ++b) {
        const float f_lo = kFreqMin * std::exp(log_ratio * static_cast<float>(b)     / kNumBands);
        const float f_hi = kFreqMin * std::exp(log_ratio * static_cast<float>(b + 1) / kNumBands);

        int bin_lo = std::max(1, static_cast<int>(f_lo / bin_hz));
        int bin_hi = std::max(bin_lo + 1, static_cast<int>(f_hi / bin_hz) + 1);
        bin_hi     = std::min(bin_hi, static_cast<int>(N / 2));

        float sum_sq = 0.0f;
        for (int k = bin_lo; k < bin_hi; ++k) {
            const float mag = std::abs(fft_buf_[static_cast<size_t>(k)]);
            sum_sq += mag * mag;
        }

        const int   count = bin_hi - bin_lo;
        const float rms   = (count > 0)
            ? std::sqrt(sum_sq / static_cast<float>(count))
            : 0.0f;

        const float raw = std::min(1.0f, rms / norm_div);
        work_bands_[b]  = work_bands_[b] * kBandSmooth + raw * (1.0f - kBandSmooth);
    }
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
Analyzer::Analyzer(std::shared_ptr<SpscRingBuffer<float>> analysis_buffer)
    : analysis_buffer_(std::move(analysis_buffer))
{
    static constexpr size_t kFftSize = 512; // mono frames; matches WINDOW_SIZE/2

    build_hann_window(kFftSize);
    fft_buf_.resize(kFftSize);
    work_bands_.assign(kNumBands, 0.0f);  // initialise EMA state to silence

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

// ---------------------------------------------------------------------------
// Accessors (mutex-protected copies for cross-thread safety)
// ---------------------------------------------------------------------------
std::vector<float> Analyzer::get_latest_window() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return latest_window_;
}

std::vector<float> Analyzer::get_frequency_bands() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return fft_bands_;
}

// ---------------------------------------------------------------------------
// Analyzer thread loop
// ---------------------------------------------------------------------------
void Analyzer::loop() {
    const size_t WINDOW_SIZE = 1024; // 512 frames * 2 channels = 1024 floats
    std::vector<float> temp_buf(WINDOW_SIZE);

    while (!stop_flag_.load(std::memory_order_acquire)) {
        size_t available = analysis_buffer_->read_available();
        if (available >= WINDOW_SIZE) {
            analysis_buffer_->read(temp_buf.data(), WINDOW_SIZE);

            // Compute FFT + bands on the analyzer thread (no lock held).
            run_fft_and_update_bands(temp_buf);

            // Publish both outputs atomically under one lock.
            {
                std::lock_guard<std::mutex> lock(data_mutex_);
                latest_window_ = temp_buf;
                fft_bands_     = work_bands_; // copy smoothed result to shared output
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
        }
    }
}

} // namespace txplay::audio
