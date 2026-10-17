// EffectRegistry.cpp — builds the registry of all known visualizer effects.
//
// ─────────────────────────────────────────────────────────────────────────
// HOW TO ADD A NEW EFFECT
// ─────────────────────────────────────────────────────────────────────────
// 1. Create MyEffect.hpp + MyEffect.cpp in src/ui/visualizer/effects/.
//    Inherit from VisualizerEffect and implement name() + render().
//
// 2. Add #include "effects/MyEffect.hpp" below.
//
// 3. Add one line inside EffectRegistry::create():
//      registry.register_effect(std::make_unique<MyEffect>());
//
// 4. Add MyEffect.cpp to the UI_SRCS list in CMakeLists.txt.
//
// No other file needs to change.  The manager (Visualizer.cpp), the
// interface (VisualizerEffect.hpp), and existing effects are untouched.
// ─────────────────────────────────────────────────────────────────────────

#include "EffectRegistry.hpp"

// ---- Register all known effects here ----
#include "effects/BarsEffect.hpp"
#include "effects/PeakEffect.hpp"
#include "effects/SpectrumEffect.hpp"

namespace txplay::ui {

namespace {

// Internal helper used only during create().
void register_effect(
    std::unordered_map<std::string, std::unique_ptr<VisualizerEffect>>& map,
    std::unique_ptr<VisualizerEffect>                                    effect
) {
    std::string id(effect->name());
    map.emplace(std::move(id), std::move(effect));
}

} // anonymous namespace

EffectRegistry EffectRegistry::create() {
    EffectRegistry registry;

    register_effect(registry.effects_, std::make_unique<BarsEffect>());
    register_effect(registry.effects_, std::make_unique<PeakEffect>());
    register_effect(registry.effects_, std::make_unique<SpectrumEffect>());

    return registry;
}

VisualizerEffect* EffectRegistry::find(const std::string& id) const {
    auto it = effects_.find(id);
    if (it == effects_.end()) return nullptr;
    return it->second.get();
}

} // namespace txplay::ui
