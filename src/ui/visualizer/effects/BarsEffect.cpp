// BarsEffect.cpp — vertical green bar visualizer.
//
// This is the original "bars" effect, migrated verbatim from the monolithic
// Visualizer.cpp.  Its visual output and FTXUI rendering approach are
// intentionally unchanged.

#include "BarsEffect.hpp"

#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <cmath>

using namespace ftxui;

namespace txplay::ui {

ftxui::Element BarsEffect::render(const VisualizerContext& ctx) {
    const int width  = ctx.width;
    const int height = ctx.height;
    const auto& magnitudes = ctx.samples;

    Elements vis_elements;
    int max_bars = std::max(1, width);
    int bands    = std::min(static_cast<int>(magnitudes.size()), max_bars);

    for (int i = 0; i < bands; i++) {
        float val = std::min(1.0f, std::abs(magnitudes[i]) * 2.0f);
        vis_elements.push_back(gaugeUp(val) | color(Color::Green) | flex);
    }
    while (static_cast<int>(vis_elements.size()) < max_bars) {
        vis_elements.push_back(gaugeUp(0.0f) | color(Color::Green) | flex);
    }

    return hbox(std::move(vis_elements))
        | size(HEIGHT, EQUAL, height);
}

} // namespace txplay::ui
