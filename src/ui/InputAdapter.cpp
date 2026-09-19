#include "InputAdapter.hpp"

using namespace ftxui;
using txplay::common::Key;

namespace txplay::ui {

// ---------------------------------------------------------------------------
// key_from_event()
//
// Maps a FTXUI Event to our RIGHT-side Key representation.
//
// This is the ONLY place in Txplay that reads FTXUI Event internals for the
// purpose of identifying which physical key was pressed.
//
// Mapping:
//   Arrow keys    → Key::arrow_{up,down,left,right}()
//   Return        → Key::enter()
//   Escape        → Key::escape()
//   Tab           → Key::tab()
//   TabReverse    → Key::shift_tab()
//   Backspace     → Key::backspace()
//   Character(' ')→ Key::space()
//   Character('x')→ Key::character('x')   for any single printable char
//   Mouse / Custom/ anything else → Key::unknown()
// ---------------------------------------------------------------------------
Key key_from_event(const Event& event) {
    if (event == Event::ArrowUp)        return Key::arrow_up();
    if (event == Event::ArrowDown)      return Key::arrow_down();
    if (event == Event::ArrowLeft)      return Key::arrow_left();
    if (event == Event::ArrowRight)     return Key::arrow_right();
    if (event == Event::Return)         return Key::enter();
    if (event == Event::Escape)         return Key::escape();
    if (event == Event::Tab)            return Key::tab();
    if (event == Event::TabReverse)     return Key::shift_tab();
    if (event == Event::Backspace)      return Key::backspace();

    // Single-character events
    if (event.is_character()) {
        const std::string& s = event.character();
        if (s.size() == 1) {
            const char c = s[0];
            if (c == ' ') return Key::space();
            return Key::character(c);
        }
    }

    // Mouse, Custom (ticker), and everything else → unknown
    return Key::unknown();
}

} // namespace txplay::ui
