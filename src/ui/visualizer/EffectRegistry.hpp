#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include "VisualizerEffect.hpp"

namespace txplay::ui {

// ---------------------------------------------------------------------------
// EffectRegistry — resolves configuration identifiers to effect instances.
//
// Effect instances are constructed once at startup via create() and reused
// for every frame.  This preserves each effect's animation state across
// frames without recreating it.
//
// Usage (from Visualizer.cpp):
//
//   static EffectRegistry registry = EffectRegistry::create();
//   VisualizerEffect* effect = registry.find("bars");
//   if (effect) ftxui::Element el = effect->render(ctx);
//
// The registry is constructed with a fixed set of known effects.  The list
// of effects lives entirely in EffectRegistry.cpp; no other file needs to
// change when a new effect is added.
//
// find() returns nullptr for unknown identifiers.  The caller is responsible
// for deciding the fallback behavior.
//
// Design: deterministic initialization via a factory function.
//   - No static-initialization-order dependency.
//   - No dynamic plugin loading.
//   - No heap allocation during rendering (ownership held in unique_ptr map).
// ---------------------------------------------------------------------------
class EffectRegistry {
public:
    // Build and return a registry containing all known effects.
    // Call this once at startup.
    static EffectRegistry create();

    // Look up an effect by its configuration identifier.
    // Returns nullptr if the identifier is not registered.
    VisualizerEffect* find(const std::string& id) const;

    // Prevent accidental copying; allow move for the factory return.
    EffectRegistry(const EffectRegistry&)            = delete;
    EffectRegistry& operator=(const EffectRegistry&) = delete;
    EffectRegistry(EffectRegistry&&)                 = default;
    EffectRegistry& operator=(EffectRegistry&&)      = default;

private:
    explicit EffectRegistry() = default;

    // Effect instances are owned by the registry for their lifetime.
    std::unordered_map<std::string, std::unique_ptr<VisualizerEffect>> effects_;
};

} // namespace txplay::ui
