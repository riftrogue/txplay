#pragma once

#include "../VisualizerEffect.hpp"
#include <vector>

namespace txplay::ui {

// ---------------------------------------------------------------------------
// SpectrumEffect — FFT frequency-spectrum bar visualizer.
//
// Visual design:
//
//     Low freq ──────────────────────────── High freq
//        █   █  ▇  ▅   ▄  ▃  ▂  ▁
//        █   █  █  ▇   ▅  ▄  ▃  ▂
//       (bars coloured by frequency: blue → cyan → magenta)
//
// Unlike BarsEffect (which operates on raw PCM samples), SpectrumEffect
// consumes the pre-computed fft_bands data from VisualizerContext.
// The Analyzer thread produces 32 logarithmically spaced, EMA-smoothed
// band magnitudes in [0, 1] covering ~40 Hz to ~18 kHz.
//
// Rendering strategy:
//   - Bands are distributed across the available terminal width by sampling
//     the 32 band array linearly.  Terminals narrower than 32 columns
//     subsample; wider terminals repeat/interpolate bands.
//   - Bar height is scaled to ctx.height using gaugeUp.
//   - A per-instance EMA is applied on top of the Analyzer's own smoothing,
//     giving the display a little extra inertia without modifying the shared
//     analysis data.
//   - Bar colour transitions from Blue → Cyan → Magenta as frequency
//     increases, making the spectral content immediately readable.
//
// State ownership:
//   smoothed_ and state_width_ are instance members and persist across
//   frames.  They are reinitialised when the drawable width changes.
//
// Graceful degradation:
//   - If fft_bands is empty (before the first analysis frame), all bars
//     render at zero height.
//   - Magnitudes outside [0, 1] are clamped before rendering.
// ---------------------------------------------------------------------------
class SpectrumEffect : public VisualizerEffect {
public:
    // EMA factor applied on the display side (0 = raw, 1 = frozen).
    // The Analyzer already applies its own EMA; this adds a second lighter
    // smoothing pass for a more fluid visual.
    static constexpr float kSmooth = 0.25f;

    std::string_view name() const noexcept override { return "spectrum"; }
    ftxui::Element   render(const VisualizerContext& ctx) override;

private:
    // Per-bar EMA state — reinitialised when the drawable width changes.
    int               state_width_{0};
    std::vector<float> smoothed_;  // display-side smoothed magnitude per bar
};

} // namespace txplay::ui
