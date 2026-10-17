#pragma once

#include <string_view>
#include <ftxui/dom/elements.hpp>
#include "VisualizerContext.hpp"

namespace txplay::ui {

// ---------------------------------------------------------------------------
// VisualizerEffect — abstract base for all visualizer effects.
//
// Every effect must implement:
//
//   name()   — the stable configuration identifier (e.g. "bars", "peak").
//              Used by EffectRegistry to match the user's config value.
//              Must be stable across releases because it is stored in
//              config.txt.  Do not rename an effect once shipped.
//
//   render() — produce an FTXUI Element for one frame.
//              Called at ~30 FPS from the UI thread.
//              May retain and mutate per-instance animation state.
//              Must handle ctx.width == 0 and ctx.height == 0 gracefully.
//              Must not block or perform I/O.
//
// Lifetime:
//   Effect instances are constructed once at application startup inside
//   EffectRegistry and reused for every frame.  This allows effects to hold
//   smoothing buffers, peak accumulators, and other per-bar state that must
//   persist across frames.  State is NOT reset when the user switches away
//   from an effect and later switches back.
//
// Adding a new effect:
//   1. Create MyEffect.hpp + MyEffect.cpp in src/ui/visualizer/effects/.
//   2. Inherit from VisualizerEffect and implement name() + render().
//   3. Register the effect in EffectRegistry.cpp — that is the ONLY file
//      that needs to change.
//   4. Add the new .cpp to CMakeLists.txt under UI_SRCS.
//   No other file needs to be modified.
// ---------------------------------------------------------------------------
class VisualizerEffect {
public:
    virtual ~VisualizerEffect() = default;

    // Returns the stable configuration identifier for this effect.
    // The returned string_view must remain valid for the lifetime of the
    // effect object (typically the application lifetime).
    virtual std::string_view name() const noexcept = 0;

    // Render one frame and return an FTXUI Element.
    // ctx.samples may be empty (nothing playing); effects should render
    // a sensible idle state (silence / blank) in that case.
    virtual ftxui::Element render(const VisualizerContext& ctx) = 0;
};

} // namespace txplay::ui
