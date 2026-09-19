#pragma once

// InputAdapter — the only FTXUI boundary for keyboard input.
//
// Translates an incoming ftxui::Event into our RIGHT-side Key representation
// (txplay::common::Key).  Nothing outside this file and InputAdapter.cpp
// should inspect FTXUI Event internals for the purpose of identifying which
// physical keyboard key was pressed.

#include "common/Key.hpp"
#include <ftxui/component/event.hpp>

namespace txplay::ui {

// Convert a FTXUI Event into our RIGHT-side keyboard Key.
// Mouse events, timer events, and any non-keyboard input return Key::unknown().
txplay::common::Key key_from_event(const ftxui::Event& event);

} // namespace txplay::ui
