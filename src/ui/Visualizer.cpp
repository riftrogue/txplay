#include "Visualizer.hpp"

#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace ftxui;

namespace txplay::ui {

// ---------------------------------------------------------------------------
// render_bars() — vertical gauge bar renderer
// ---------------------------------------------------------------------------
// Renders each magnitude value as a vertical gaugeUp bar colored green.
// Missing bars (when magnitudes < width) are filled with empty bars.
// This is the only renderer currently supported.
static Element render_bars(
    const std::vector<float>& magnitudes,
    int                       width,
    int                       height
) {
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

// ---------------------------------------------------------------------------
// render_visualizer() — public dispatch function
// ---------------------------------------------------------------------------
ftxui::Element render_visualizer(
    const std::vector<float>& magnitudes,
    const std::string&        style,
    int                       width,
    int                       height
) {
    if (style == "bars" || style.empty()) {
        return render_bars(magnitudes, width, height);
    }

    // Unknown style: fall back to bars with a one-time stderr warning.
    // Avoid logging every frame (30 FPS) by using a static flag.
    static bool warned = false;
    if (!warned) {
        std::cerr << "Warning: unknown visualizer style '"
                  << style << "' — falling back to 'bars'.\n";
        warned = true;
    }
    return render_bars(magnitudes, width, height);
}

} // namespace txplay::ui
