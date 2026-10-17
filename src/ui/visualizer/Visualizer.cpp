// Visualizer.cpp — visualizer manager; dispatches to the selected effect.
//
// This file owns the EffectRegistry singleton (initialised once on the first
// call to render_visualizer()) and delegates frame rendering to the
// appropriate VisualizerEffect instance.
//
// The manager contains no effect-specific rendering logic and no per-effect
// conditional branches.  All effect logic lives in the effects/ directory.
//
// Architecture:
//
//   render_visualizer()          ← called from TxplayUI at ~30 FPS
//       │
//       ├── EffectRegistry       ← resolves "bars" / "peak" / … → effect*
//       │
//       └── VisualizerEffect::render(VisualizerContext)
//               ├── BarsEffect   ← green bars
//               └── PeakEffect   ← cyan floating markers
//
// Adding a new effect: see EffectRegistry.cpp.

#include "Visualizer.hpp"
#include "EffectRegistry.hpp"
#include "VisualizerContext.hpp"

#include <iostream>

namespace txplay::ui {

ftxui::Element render_visualizer(
    const std::vector<float>& magnitudes,
    const std::vector<float>& fft_bands,
    const std::string&        style,
    int                       width,
    int                       height
) {
    // The registry is initialised once and reused for the application
    // lifetime.  Function-local static is safe here: C++11 guarantees that
    // the initialiser runs exactly once, even in the presence of concurrent
    // first calls (which cannot happen in practice because this is called
    // from the single UI thread).
    static EffectRegistry s_registry = EffectRegistry::create();

    VisualizerContext ctx{magnitudes, fft_bands, width, height};

    VisualizerEffect* effect = s_registry.find(style);

    if (effect) {
        return effect->render(ctx);
    }

    // Unknown style: warn once, then fall back to "bars".
    static bool s_warned = false;
    if (!s_warned) {
        std::cerr << "Warning: unknown visualizer style '"
                  << style << "' — falling back to 'bars'.\n";
        s_warned = true;
    }

    VisualizerEffect* fallback = s_registry.find("bars");
    // "bars" is always registered; this branch is only hit for unknown styles.
    if (fallback) {
        return fallback->render(ctx);
    }

    // Should be unreachable: "bars" is always in the registry.
    return ftxui::text("") | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, std::max(1, height));
}

} // namespace txplay::ui
