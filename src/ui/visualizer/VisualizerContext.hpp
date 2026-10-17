#pragma once

#include <vector>

namespace txplay::ui {

// ---------------------------------------------------------------------------
// VisualizerContext
//
// Carries everything an effect needs to render one frame.
// Passed by const reference into VisualizerEffect::render().
//
// Fields:
//   samples    — raw interleaved stereo PCM window from the Analyzer thread.
//                Currently 1024 f32 floats (512 frames × 2 channels).
//                Effects must not assume a particular size; always check .size().
//                May be empty when nothing is playing.
//
//   fft_bands  — Analyzer::kNumBands (32) EMA-smoothed frequency-band
//                magnitudes in [0.0, 1.0], logarithmically spaced from
//                ~40 Hz to ~18 kHz, derived from a 512-point Hann-windowed FFT.
//                May be empty until the first analysis frame completes.
//                Existing effects that use only `samples` are unaffected.
//
//   width / height — drawable terminal cells.
//                Both may be zero during terminal resizing; effects must
//                guard against divide-by-zero and degenerate dimensions.
//
// The struct is cheap to construct (const references + two ints; no copy).
// Both vector references point to caller-owned locals in render_visualizer().
//
// Future fields (e.g. raw mono samples, spectrogram history) can be added
// here without changing the effect interface or recompiling unaffected effects.
// ---------------------------------------------------------------------------
struct VisualizerContext {
    const std::vector<float>& samples;    // raw PCM window (1024 interleaved stereo f32)
    const std::vector<float>& fft_bands;  // 32 log-spaced frequency bands [0, 1]
    int                       width;      // drawable terminal columns
    int                       height;     // drawable terminal rows
};

} // namespace txplay::ui
