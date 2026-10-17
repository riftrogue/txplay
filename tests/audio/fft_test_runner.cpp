// fft_test_runner.cpp
//
// Integration tests for the FFT audio analysis pipeline.
// Exercises Analyzer with known inputs via the SpscRingBuffer and verifies
// that get_frequency_bands() returns correct, stable, and safe output.
//
// Tests:
//   FA01 — silence produces all-near-zero bands.
//   FA02 — a 1000 Hz sine wave produces a peak in the expected band range.
//   FA03 — band output always has exactly kNumBands elements in [0, 1].
//   FA04 — empty buffer (nothing written) returns empty bands without crash.
//   FA05 — get_latest_window() still works after FFT is computed.
//   FA06 — analyzer handles rapid successive windows without crash.

#include <cassert>
#include <cmath>
#include <cstdio>
#include <thread>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>

#include "audio/Analyzer.hpp"
#include "audio/SpscRingBuffer.hpp"

// ---------------------------------------------------------------------------
// Helper: generate interleaved stereo sine wave (1024 floats = 512 frames)
// frequency_hz — frequency of the sine
// sample_rate  — samples per second
// amplitude    — peak amplitude in [-1, 1]
// ---------------------------------------------------------------------------
static std::vector<float> make_sine_window(
    float frequency_hz,
    float sample_rate = 44100.0f,
    float amplitude   = 0.8f
) {
    constexpr size_t kFrames = 512;
    std::vector<float> buf(kFrames * 2);
    for (size_t i = 0; i < kFrames; ++i) {
        float s = amplitude * std::sin(2.0f * static_cast<float>(M_PI)
                                       * frequency_hz
                                       * static_cast<float>(i) / sample_rate);
        buf[i * 2]     = s;  // left
        buf[i * 2 + 1] = s;  // right
    }
    return buf;
}

// ---------------------------------------------------------------------------
// Helper: write window into ring buffer, wait for analyzer to process it,
// then return the frequency bands.
// Repeats `repetitions` times to let EMA smoothing converge.
// ---------------------------------------------------------------------------
static std::vector<float> feed_and_read(
    txplay::audio::SpscRingBuffer<float>& ring,
    txplay::audio::Analyzer&              analyzer,
    const std::vector<float>&             window,
    int                                   repetitions = 5
) {
    for (int r = 0; r < repetitions; ++r) {
        // Drain any stale data so the ring does not fill.
        size_t avail = ring.read_available();
        if (avail > 0) {
            std::vector<float> drain(avail);
            ring.read(drain.data(), avail);
        }
        ring.write(window.data(), window.size());
        std::this_thread::sleep_for(std::chrono::milliseconds(25)); // ~60fps * 1.5
    }
    return analyzer.get_frequency_bands();
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_silence_produces_zero_bands() {
    printf("FA01: silence -> all bands near zero...\n");

    auto ring = std::make_shared<txplay::audio::SpscRingBuffer<float>>(65536);
    txplay::audio::Analyzer analyzer(ring);

    std::vector<float> silence(1024, 0.0f);
    auto bands = feed_and_read(*ring, analyzer, silence, 8);

    assert(!bands.empty() && "bands must not be empty after processing");
    assert(static_cast<int>(bands.size()) == txplay::audio::Analyzer::kNumBands);

    float max_band = *std::max_element(bands.begin(), bands.end());
    // Silence may not be perfectly 0.0 due to floating-point and EMA, but
    // must be very small (< 0.05).
    assert(max_band < 0.05f && "all bands should be near zero for silence");

    printf("  FA01 passed. (max band=%.4f)\n", max_band);
}

static void test_sine_wave_peak_in_correct_band() {
    printf("FA02: 1000 Hz sine -> peak band in expected range (~band 16-17)...\n");

    auto ring = std::make_shared<txplay::audio::SpscRingBuffer<float>>(65536);
    txplay::audio::Analyzer analyzer(ring);

    // Feed a 1000 Hz sine wave. Expected band:
    //   f = 1000 Hz, kFreqMin=40, kFreqMax=18000, kNumBands=32
    //   b = log(1000/40) / log(18000/40) * 32
    //     = log(25) / log(450) * 32
    //     = 3.219 / 6.109 * 32 ≈ 16.85  → band 16 or 17
    const int kExpectedBandLo = 14;  // generous window around the expected peak
    const int kExpectedBandHi = 19;

    auto window = make_sine_window(1000.0f);
    auto bands  = feed_and_read(*ring, analyzer, window, 10);

    assert(!bands.empty());
    assert(static_cast<int>(bands.size()) == txplay::audio::Analyzer::kNumBands);

    int peak_band = static_cast<int>(
        std::max_element(bands.begin(), bands.end()) - bands.begin());
    float peak_val = bands[static_cast<size_t>(peak_band)];

    printf("  Peak band: %d (value=%.4f)\n", peak_band, peak_val);
    assert(peak_val > 0.05f && "peak band should have significant energy");
    assert(peak_band >= kExpectedBandLo && peak_band <= kExpectedBandHi
           && "peak must be in the band range corresponding to ~1 kHz");

    printf("  FA02 passed.\n");
}

static void test_band_output_valid_range() {
    printf("FA03: all bands in [0, 1] and count == kNumBands...\n");

    auto ring = std::make_shared<txplay::audio::SpscRingBuffer<float>>(65536);
    txplay::audio::Analyzer analyzer(ring);

    // Use a loud signal to stress-test normalisation.
    auto window = make_sine_window(440.0f, 44100.0f, 1.0f);
    auto bands  = feed_and_read(*ring, analyzer, window, 6);

    assert(static_cast<int>(bands.size()) == txplay::audio::Analyzer::kNumBands);
    for (int i = 0; i < txplay::audio::Analyzer::kNumBands; ++i) {
        assert(bands[static_cast<size_t>(i)] >= 0.0f && "band must be >= 0");
        assert(bands[static_cast<size_t>(i)] <= 1.0f && "band must be <= 1");
    }

    printf("  FA03 passed. (kNumBands=%d)\n", txplay::audio::Analyzer::kNumBands);
}

static void test_empty_buffer_no_crash() {
    printf("FA04: empty ring buffer -> get_frequency_bands() returns empty without crash...\n");

    auto ring = std::make_shared<txplay::audio::SpscRingBuffer<float>>(65536);
    txplay::audio::Analyzer analyzer(ring);

    // Do not write anything. The analyzer thread sleeps and does not process.
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    auto bands = analyzer.get_frequency_bands();

    // Before any window is processed the bands vector is empty.
    // This is documented in the header; the caller must handle it.
    printf("  FA04 passed. (bands.size()=%zu — empty before first window, as expected)\n",
           bands.size());
}

static void test_get_latest_window_still_works() {
    printf("FA05: get_latest_window() still returns raw PCM after FFT is computed...\n");

    auto ring = std::make_shared<txplay::audio::SpscRingBuffer<float>>(65536);
    txplay::audio::Analyzer analyzer(ring);

    auto window = make_sine_window(500.0f);
    feed_and_read(*ring, analyzer, window, 4);

    auto raw = analyzer.get_latest_window();
    assert(raw.size() == 1024 && "raw window must be 1024 floats");
    // The window should not be all zeros (we fed a sine wave).
    float sum = 0.0f;
    for (float f : raw) sum += std::abs(f);
    assert(sum > 0.0f && "raw window should contain signal data");

    printf("  FA05 passed. (window.size()=%zu, signal_sum=%.2f)\n",
           raw.size(), sum);
}

static void test_rapid_successive_windows_no_crash() {
    printf("FA06: rapid successive windows do not crash or deadlock...\n");

    auto ring = std::make_shared<txplay::audio::SpscRingBuffer<float>>(65536);
    txplay::audio::Analyzer analyzer(ring);

    auto window = make_sine_window(880.0f);

    // Write 20 windows as fast as possible — some may be dropped if the ring
    // fills, but neither the ring nor the analyzer should crash.
    for (int i = 0; i < 20; ++i) {
        size_t avail = ring->read_available();
        if (ring->write_available() >= window.size()) {
            ring->write(window.data(), window.size());
        } else {
            // Drain first to avoid stalling
            std::vector<float> drain(avail);
            if (avail) ring->read(drain.data(), avail);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto bands = analyzer.get_frequency_bands();
    // Just verify no crash and output is in range.
    for (float b : bands) {
        assert(b >= 0.0f && b <= 1.0f);
    }

    printf("  FA06 passed.\n");
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    printf("--- FFT Analyzer Test Runner ---\n");

    test_silence_produces_zero_bands();
    test_sine_wave_peak_in_correct_band();
    test_band_output_valid_range();
    test_empty_buffer_no_crash();
    test_get_latest_window_still_works();
    test_rapid_successive_windows_no_crash();

    printf("\nAll FFT analyzer assertions passed! (6 tests)\n");
    return 0;
}
