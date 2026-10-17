// PeakEffect.cpp — floating cyan peak-marker visualizer.
//
// Migrated from the working-tree render_peak() static function in
// the old Visualizer.cpp.
//
// Changes from the original:
//   - Function-local statics become instance members (s_width → state_width_,
//     s_smoothed → smoothed_, s_peak → peak_, s_hold → hold_).
//   - Dead code removed: the row_fill_threshold and unused in_bar variables
//     that were left over when the green bars were stripped from this effect.
//   - No visual or behavioural change.

#include "PeakEffect.hpp"

#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

using namespace ftxui;

namespace txplay::ui {

// Eighth-block characters, index 0 = ▁ (1/8), index 7 = █ (8/8 full).
// Defined at file scope so it is shared across calls without re-declaration.
static const char* const kBlocks[8] = {
    "\xe2\x96\x81",  // ▁
    "\xe2\x96\x82",  // ▂
    "\xe2\x96\x83",  // ▃
    "\xe2\x96\x84",  // ▄
    "\xe2\x96\x85",  // ▅
    "\xe2\x96\x86",  // ▆
    "\xe2\x96\x87",  // ▇
    "\xe2\x96\x88",  // █
};

ftxui::Element PeakEffect::render(const VisualizerContext& ctx) {
    const int width  = ctx.width;
    const int height = ctx.height;
    const auto& magnitudes = ctx.samples;

    const int max_bars = std::max(1, width);

    // ----- Reinitialise animation state if the terminal width changed -----
    if (state_width_ != max_bars) {
        state_width_ = max_bars;
        smoothed_.assign(max_bars, 0.0f);
        peak_    .assign(max_bars, 0.0f);
        hold_    .assign(max_bars, 0);
    }

    // ----- Update state for this frame -----
    const int bands = std::min(static_cast<int>(magnitudes.size()), max_bars);

    for (int i = 0; i < max_bars; i++) {
        float raw = (i < bands)
            ? std::min(1.0f, std::abs(magnitudes[i]) * 2.0f)
            : 0.0f;

        // Exponential moving average for the bar height.
        smoothed_[i] = smoothed_[i] * kSmooth + raw * (1.0f - kSmooth);

        // Peak detection: rise immediately, hold, then decay.
        if (smoothed_[i] >= peak_[i]) {
            peak_[i] = smoothed_[i];
            hold_[i] = kPeakHold;
        } else {
            if (hold_[i] > 0) {
                --hold_[i];
            } else {
                peak_[i] = std::max(smoothed_[i], peak_[i] - kPeakDecay);
            }
        }
    }

    // ----- Render -----
    // Each column is a vbox of `height` rows.
    // The peak marker appears in the row that corresponds to the peak height.
    // There are no filled bars — only the floating marker.
    //
    // Terminal-row coordinate: row 0 = top, row (height-1) = bottom.
    // A peak value of 1.0 means the marker sits at row 0 (top).
    // A peak value of 0.0 means no visible marker (sentinel → off-screen).
    //
    // Sub-row fractional position within the peak row is mapped to one of the
    // eight Unicode eighth-block characters for sub-row visual accuracy.

    // Guard against degenerate height that would cause divide-by-zero.
    if (height <= 0) {
        return ftxui::text("") | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, std::max(1, height));
    }

    Elements cols;
    cols.reserve(static_cast<size_t>(max_bars));

    for (int i = 0; i < max_bars; i++) {
        const float peak = peak_[i];

        // Only show if peak is meaningfully above zero.
        const bool show_peak = (peak > 0.02f);
        const int  peak_row  = show_peak
            ? static_cast<int>((1.0f - peak) * static_cast<float>(height - 1))
            : height; // off-screen sentinel

        // Sub-row fractional position → block character.
        int block_idx = 7;
        if (show_peak) {
            float frac = (1.0f - peak) * static_cast<float>(height - 1)
                         - static_cast<float>(peak_row);
            // frac in [0, 1): 0 = top of row (tall block), 1 = bottom (thin).
            block_idx = 7 - static_cast<int>(frac * 8.0f);
            block_idx = std::clamp(block_idx, 0, 7);
        }

        // Build the column as a vbox of `height` individual text cells.
        Elements rows;
        rows.reserve(static_cast<size_t>(height));

        for (int r = 0; r < height; r++) {
            if (r == peak_row && show_peak) {
                rows.push_back(text(kBlocks[block_idx]) | color(Color::Cyan));
            } else {
                rows.push_back(text(" "));
            }
        }

        cols.push_back(vbox(std::move(rows)) | flex);
    }

    return hbox(std::move(cols))
        | size(HEIGHT, EQUAL, height);
}

} // namespace txplay::ui
