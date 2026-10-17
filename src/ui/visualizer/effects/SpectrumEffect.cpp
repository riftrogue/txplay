// SpectrumEffect.cpp — FFT frequency-spectrum bar visualizer.
//
// Consumes fft_bands from VisualizerContext (32 log-spaced bands [0,1])
// and renders vertical spectrum bars coloured by frequency position.

#include "SpectrumEffect.hpp"

#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <cmath>

using namespace ftxui;

namespace txplay::ui {

// ---------------------------------------------------------------------------
// Colour palette — maps a bar's normalised position [0, 1] across the
// frequency range to an FTXUI Color.
//
// Low frequencies:  Blue
// Mid frequencies:  Cyan
// High frequencies: Magenta
//
// Simple three-stop linear interpolation; no heap allocation per bar.
// ---------------------------------------------------------------------------
static Color band_color(float t) {
    // t in [0, 1]: 0 = lowest band, 1 = highest band.
    if (t < 0.5f) {
        // Blue → Cyan
        // Blue  = (0,   0,   255)
        // Cyan  = (0,   255, 255)
        int g = static_cast<int>(t * 2.0f * 255.0f);
        return Color::RGB(0, static_cast<uint8_t>(g), 255);
    } else {
        // Cyan → Magenta
        // Cyan    = (0,   255, 255)
        // Magenta = (255, 0,   255)
        float u = (t - 0.5f) * 2.0f;
        int r = static_cast<int>(u * 255.0f);
        int g = static_cast<int>((1.0f - u) * 255.0f);
        return Color::RGB(static_cast<uint8_t>(r), static_cast<uint8_t>(g), 255);
    }
}

// ---------------------------------------------------------------------------
// SpectrumEffect::render
// ---------------------------------------------------------------------------
ftxui::Element SpectrumEffect::render(const VisualizerContext& ctx) {
    const int width  = ctx.width;
    const int height = ctx.height;

    // Guard degenerate dimensions.
    if (width <= 0 || height <= 0) {
        return text("") | size(HEIGHT, EQUAL, std::max(1, height));
    }

    // Reinitialise state on first call or whenever the terminal is resized.
    if (state_width_ != width) {
        state_width_ = width;
        smoothed_.assign(static_cast<size_t>(width), 0.0f);
    }

    const auto& bands    = ctx.fft_bands;
    const int   num_bands = static_cast<int>(bands.size()); // 32 when available

    Elements bars;
    bars.reserve(static_cast<size_t>(width));

    for (int col = 0; col < width; ++col) {
        // Normalised position of this column in [0, 1] — used both for band
        // interpolation and for the colour gradient.
        const float t = (width > 1)
            ? static_cast<float>(col) / static_cast<float>(width - 1)
            : 0.0f;

        // Map this terminal column to a band index via linear interpolation.
        // When width < num_bands  → subsample (multiple bands per column).
        // When width >= num_bands → upsample  (multiple columns per band).
        float raw = 0.0f;

        if (num_bands > 0) {
            // Corresponding floating-point band index.
            float band_f = t * static_cast<float>(num_bands - 1);
            int   b0     = static_cast<int>(band_f);
            int   b1     = std::min(b0 + 1, num_bands - 1);
            float frac   = band_f - static_cast<float>(b0);

            // Linearly interpolate between the two nearest bands.
            float v0 = std::clamp(bands[static_cast<size_t>(b0)], 0.0f, 1.0f);
            float v1 = std::clamp(bands[static_cast<size_t>(b1)], 0.0f, 1.0f);
            raw = v0 + frac * (v1 - v0);
        }

        // Display-side EMA — adds a second smoothing pass for a more fluid feel.
        smoothed_[static_cast<size_t>(col)] =
            smoothed_[static_cast<size_t>(col)] * kSmooth + raw * (1.0f - kSmooth);

        const float val = smoothed_[static_cast<size_t>(col)];

        bars.push_back(
            gaugeUp(val) | color(band_color(t)) | flex
        );
    }

    return hbox(std::move(bars))
        | size(HEIGHT, EQUAL, height);
}

} // namespace txplay::ui
