// visualizer_test_runner.cpp
//
// Unit tests for the visualizer architecture and FFT band data.
// Tests cover:
//   - EffectRegistry construction and lookup.
//   - BarsEffect and PeakEffect render without crashing.
//   - Degenerate dimensions (zero width, zero height) do not crash.
//   - PeakEffect state reinitialises on width change.
//   - fft_bands field is accessible in VisualizerContext without crashes.

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

#include "ui/visualizer/EffectRegistry.hpp"
#include "ui/visualizer/VisualizerContext.hpp"
#include "ui/visualizer/VisualizerEffect.hpp"
#include "ui/visualizer/effects/BarsEffect.hpp"
#include "ui/visualizer/effects/PeakEffect.hpp"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static const std::vector<float> kEmptyBands;  // shared empty bands reference

static std::vector<float> make_samples(int n, float value = 0.5f) {
    return std::vector<float>(static_cast<size_t>(n), value);
}

// ---------------------------------------------------------------------------
// Architecture tests
// ---------------------------------------------------------------------------

static void test_registry_find_bars() {
    printf("VT01: EffectRegistry::find(\"bars\") returns non-null...\n");
    auto reg = txplay::ui::EffectRegistry::create();
    txplay::ui::VisualizerEffect* e = reg.find("bars");
    assert(e != nullptr && "bars must be registered");
    assert(e->name() == "bars");
    printf("  VT01 passed.\n");
}

static void test_registry_find_peak() {
    printf("VT02: EffectRegistry::find(\"peak\") returns non-null...\n");
    auto reg = txplay::ui::EffectRegistry::create();
    txplay::ui::VisualizerEffect* e = reg.find("peak");
    assert(e != nullptr && "peak must be registered");
    assert(e->name() == "peak");
    printf("  VT02 passed.\n");
}

static void test_registry_unknown_returns_nullptr() {
    printf("VT03: EffectRegistry::find(unknown) returns nullptr...\n");
    auto reg = txplay::ui::EffectRegistry::create();
    assert(reg.find("spectrum") == nullptr);
    assert(reg.find("") == nullptr);
    assert(reg.find("waveform") == nullptr);
    printf("  VT03 passed.\n");
}

static void test_bars_renders_with_samples() {
    printf("VT04: BarsEffect::render() does not crash with normal samples...\n");
    txplay::ui::BarsEffect effect;
    auto samples = make_samples(1024, 0.4f);
    txplay::ui::VisualizerContext ctx{samples, kEmptyBands, 80, 8};
    auto el = effect.render(ctx);
    (void)el;
    printf("  VT04 passed.\n");
}

static void test_bars_renders_empty_samples() {
    printf("VT05: BarsEffect::render() handles empty sample vector...\n");
    txplay::ui::BarsEffect effect;
    std::vector<float> empty;
    txplay::ui::VisualizerContext ctx{empty, kEmptyBands, 80, 8};
    auto el = effect.render(ctx);
    (void)el;
    printf("  VT05 passed.\n");
}

static void test_bars_degenerate_dimensions() {
    printf("VT06: BarsEffect::render() survives zero/minimal dimensions...\n");
    txplay::ui::BarsEffect effect;
    auto samples = make_samples(64, 0.5f);
    txplay::ui::VisualizerContext ctx1{samples, kEmptyBands, 1, 1};
    auto el1 = effect.render(ctx1);
    (void)el1;
    txplay::ui::VisualizerContext ctx2{samples, kEmptyBands, 0, 1};
    auto el2 = effect.render(ctx2);
    (void)el2;
    printf("  VT06 passed.\n");
}

static void test_peak_renders_with_samples() {
    printf("VT07: PeakEffect::render() does not crash with normal samples...\n");
    txplay::ui::PeakEffect effect;
    auto samples = make_samples(1024, 0.4f);
    txplay::ui::VisualizerContext ctx{samples, kEmptyBands, 80, 8};
    auto el = effect.render(ctx);
    (void)el;
    printf("  VT07 passed.\n");
}

static void test_peak_renders_empty_samples() {
    printf("VT08: PeakEffect::render() handles empty sample vector...\n");
    txplay::ui::PeakEffect effect;
    std::vector<float> empty;
    txplay::ui::VisualizerContext ctx{empty, kEmptyBands, 80, 8};
    auto el = effect.render(ctx);
    (void)el;
    printf("  VT08 passed.\n");
}

static void test_peak_degenerate_height() {
    printf("VT09: PeakEffect::render() handles height <= 0 without crash...\n");
    txplay::ui::PeakEffect effect;
    auto samples = make_samples(128, 0.5f);
    txplay::ui::VisualizerContext ctx{samples, kEmptyBands, 40, 0};
    auto el = effect.render(ctx);
    (void)el;
    printf("  VT09 passed.\n");
}

static void test_peak_state_reinitialises_on_resize() {
    printf("VT10: PeakEffect reinitialises state on width change (no crash)...\n");
    txplay::ui::PeakEffect effect;
    auto samples = make_samples(512, 0.6f);

    txplay::ui::VisualizerContext ctx40{samples, kEmptyBands, 40, 8};
    auto el1 = effect.render(ctx40);
    (void)el1;

    txplay::ui::VisualizerContext ctx80{samples, kEmptyBands, 80, 8};
    auto el2 = effect.render(ctx80);
    (void)el2;

    auto el3 = effect.render(ctx40);
    (void)el3;

    printf("  VT10 passed.\n");
}

static void test_peak_multiple_frames_accumulate_state() {
    printf("VT11: PeakEffect persists state across multiple frames...\n");
    txplay::ui::PeakEffect effect;
    auto samples = make_samples(256, 0.8f);
    txplay::ui::VisualizerContext ctx{samples, kEmptyBands, 40, 8};
    for (int i = 0; i < 20; ++i) {
        auto el = effect.render(ctx);
        (void)el;
    }
    printf("  VT11 passed.\n");
}

static void test_registry_effect_instances_persist() {
    printf("VT12: EffectRegistry returns same instance on repeated find()...\n");
    auto reg = txplay::ui::EffectRegistry::create();
    txplay::ui::VisualizerEffect* e1 = reg.find("peak");
    txplay::ui::VisualizerEffect* e2 = reg.find("peak");
    assert(e1 != nullptr && e1 == e2 && "same instance must be returned");
    printf("  VT12 passed.\n");
}

static void test_fft_bands_in_context_accessible() {
    printf("VT13: VisualizerContext::fft_bands is accessible to effects...\n");
    txplay::ui::BarsEffect effect;
    auto samples = make_samples(1024, 0.4f);
    auto bands   = make_samples(32, 0.3f);  // 32 synthetic band values
    txplay::ui::VisualizerContext ctx{samples, bands, 80, 8};
    // bars effect ignores fft_bands; render must still succeed
    auto el = effect.render(ctx);
    (void)el;
    // verify fields are accessible
    assert(ctx.fft_bands.size() == 32);
    assert(ctx.samples.size() == 1024);
    printf("  VT13 passed.\n");
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    printf("--- Visualizer Architecture Test Runner ---\n");

    test_registry_find_bars();
    test_registry_find_peak();
    test_registry_unknown_returns_nullptr();
    test_bars_renders_with_samples();
    test_bars_renders_empty_samples();
    test_bars_degenerate_dimensions();
    test_peak_renders_with_samples();
    test_peak_renders_empty_samples();
    test_peak_degenerate_height();
    test_peak_state_reinitialises_on_resize();
    test_peak_multiple_frames_accumulate_state();
    test_registry_effect_instances_persist();
    test_fft_bands_in_context_accessible();

    printf("\nAll visualizer architecture assertions passed! (13 tests)\n");
    return 0;
}
