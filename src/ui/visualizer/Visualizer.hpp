#pragma once

#include <vector>
#include <string>
#include <ftxui/dom/elements.hpp>

namespace txplay::ui {

// ---------------------------------------------------------------------------
// render_visualizer() — public entry point for the visualizer subsystem.
//
// Produces one FTXUI Element representing the visualizer for the current
// frame.  Called from TxplayUI's renderer lambda at ~30 FPS.
//
// Parameters:
//   magnitudes — raw PCM sample window from the Analyzer thread.
//                May be empty when nothing is playing.
//   fft_bands  — 32 EMA-smoothed frequency-band magnitudes in [0, 1],
//                log-spaced from ~40 Hz to ~18 kHz.
//                May be empty until the first analysis frame is ready.
//   style      — the configuration identifier of the selected effect
//                (e.g. "bars", "peak").
//                Unknown identifiers fall back to "bars" with a one-time
//                warning to stderr.
//   width      — drawable terminal columns (border excluded).
//   height     — drawable terminal rows.
//
// Callers are responsible for:
//   - Checking Config::visualizer().enabled before calling.
//   - Adding any surrounding border or separator elements.
//
// Architecture:
//   This function owns an EffectRegistry (initialised on first call) and
//   delegates rendering to the selected VisualizerEffect instance.
//   Effect instances persist for the application lifetime; their animation
//   state survives across frames.
//
// Adding a new effect: see src/ui/visualizer/EffectRegistry.cpp.
// ---------------------------------------------------------------------------
ftxui::Element render_visualizer(
    const std::vector<float>& magnitudes,
    const std::vector<float>& fft_bands,
    const std::string&        style,
    int                       width,
    int                       height
);

} // namespace txplay::ui
