#pragma once

#include <vector>
#include <string>
#include <ftxui/dom/elements.hpp>

namespace txplay::ui {

// ---------------------------------------------------------------------------
// render_visualizer() — produce an FTXUI Element from FFT magnitude data.
// ---------------------------------------------------------------------------
// style   — selects the renderer:
//             "bars"          → vertical bar visualizer (only current style)
//             anything else   → falls back to "bars" with a one-time warning
// width   — available terminal columns (border not included; caller adds border)
// height  — terminal rows (from Config::visualizer().height)
//
// Callers are responsible for adding the border and checking
// Config::visualizer().enabled before calling this function.
//
// To add a new visualizer style in the future, add a render_X() function in
// Visualizer.cpp and one if-branch in render_visualizer(). No other file needs
// to change.
ftxui::Element render_visualizer(
    const std::vector<float>& magnitudes,
    const std::string&        style,
    int                       width,
    int                       height
);

} // namespace txplay::ui
