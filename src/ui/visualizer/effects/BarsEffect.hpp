#pragma once

#include "../VisualizerEffect.hpp"

namespace txplay::ui {

// ---------------------------------------------------------------------------
// BarsEffect — vertical green bar visualizer.
//
// Renders each sample position as a gaugeUp bar coloured green.
// This is the original default effect, preserved without visual change.
//
// State: none. The bars are recomputed from the raw PCM window each frame.
// ---------------------------------------------------------------------------
class BarsEffect : public VisualizerEffect {
public:
    std::string_view name() const noexcept override { return "bars"; }
    ftxui::Element   render(const VisualizerContext& ctx) override;
};

} // namespace txplay::ui
