#pragma once

#include <string>
#include <algorithm>
#include <cstdint>

#include <ftxui/dom/elements.hpp>

// Small reusable FTXUI element builders.

namespace txplay::ui {

inline ftxui::Element build_seek_bar(float progress, int width) {
    using namespace ftxui;
    if (width <= 0) return text("");
    int pos = std::clamp(static_cast<int>(progress * width), 0, std::max(0, width - 1));
    std::string bar;
    bar.reserve(width);
    for (int i = 0; i < width; i++) {
        if (i == pos) bar += "●";
        else          bar += "─";
    }
    return text(bar);
}

} // namespace txplay::ui
