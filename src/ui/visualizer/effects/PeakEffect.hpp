#pragma once

#include "../VisualizerEffect.hpp"
#include <vector>

namespace txplay::ui {

// ---------------------------------------------------------------------------
// PeakEffect — floating cyan peak-marker visualizer.
//
// Visual design:
//
//     ▂     ▄     ▇     ← peak marker at highest recent position per bar
//
//     (no filled bars — markers only)
//
// Each peak marker:
//   - Rises immediately when the EMA-smoothed bar height exceeds it.
//   - Holds for kPeakHold frames before decaying.
//   - Decays downward at kPeakDecay per frame.
//   - Uses Unicode eighth-block characters (▁▂▃▄▅▆▇█) for sub-row accuracy.
//
// Tuning constants are declared in the class so they can be read and
// overridden by tests or a future settings UI without changing the ABI.
//
// State ownership:
//   Animation state (smoothed heights, peak heights, hold counters) is
//   owned by the instance and persists across frames.  It is reinitialised
//   whenever the drawable width changes (terminal resize).
//   State is NOT reset when the user switches away and back to this effect.
//
// This is the correct, non-static version of the "peak" effect that was
// implemented in the working tree.  The only changes from that version are:
//   - Function-local statics become instance members (proper encapsulation).
//   - Dead code (row_fill_threshold and unused in_bar variable) removed.
//   - No other visual or behavioural change.
// ---------------------------------------------------------------------------
class PeakEffect : public VisualizerEffect {
public:
    // ---- Tuning constants ----
    static constexpr float kSmooth    = 0.30f; // EMA factor (0 = raw, 1 = frozen)
    static constexpr int   kPeakHold  = 12;    // frames before peak starts decaying (~0.4 s @ 30 FPS)
    static constexpr float kPeakDecay = 0.06f; // fractional height drop per frame during decay

    std::string_view name() const noexcept override { return "peak"; }
    ftxui::Element   render(const VisualizerContext& ctx) override;

private:
    // Persistent animation state — reinitialised on terminal resize.
    int              state_width_{0};
    std::vector<float> smoothed_;   // EMA-smoothed bar height per bar [0, 1]
    std::vector<float> peak_;       // peak height per bar [0, 1]
    std::vector<int>   hold_;       // remaining hold frames per bar
};

} // namespace txplay::ui
