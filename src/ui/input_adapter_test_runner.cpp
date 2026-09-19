// InputAdapter test runner
//
// Verifies that key_from_event() correctly maps FTXUI Events to the
// RIGHT-side common::Key representation.
//
// This is the only test file that may link against FTXUI because it exercises
// the FTXUI boundary directly.  No other test runner should include FTXUI.

#include <iostream>
#include <cassert>

#include "ui/InputAdapter.hpp"

using namespace ftxui;
using txplay::common::Key;
using txplay::common::KeyCode;
using txplay::ui::key_from_event;

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------

static void pass(const char* name) {
    std::cout << "  " << name << " passed." << std::endl;
}

// ---------------------------------------------------------------------------
// IA01: Arrow keys
// ---------------------------------------------------------------------------
static void test_arrow_keys() {
    std::cout << "IA01: arrow keys..." << std::endl;
    assert(key_from_event(Event::ArrowUp)    == Key::arrow_up());
    assert(key_from_event(Event::ArrowDown)  == Key::arrow_down());
    assert(key_from_event(Event::ArrowLeft)  == Key::arrow_left());
    assert(key_from_event(Event::ArrowRight) == Key::arrow_right());
    pass("IA01");
}

// ---------------------------------------------------------------------------
// IA02: Enter / Return
// ---------------------------------------------------------------------------
static void test_return() {
    std::cout << "IA02: Enter/Return..." << std::endl;
    assert(key_from_event(Event::Return) == Key::enter());
    pass("IA02");
}

// ---------------------------------------------------------------------------
// IA03: Escape
// ---------------------------------------------------------------------------
static void test_escape() {
    std::cout << "IA03: Escape..." << std::endl;
    assert(key_from_event(Event::Escape) == Key::escape());
    pass("IA03");
}

// ---------------------------------------------------------------------------
// IA04: Tab and TabReverse
// ---------------------------------------------------------------------------
static void test_tab() {
    std::cout << "IA04: Tab and TabReverse..." << std::endl;
    assert(key_from_event(Event::Tab)        == Key::tab());
    assert(key_from_event(Event::TabReverse) == Key::shift_tab());
    pass("IA04");
}

// ---------------------------------------------------------------------------
// IA05: Backspace
// ---------------------------------------------------------------------------
static void test_backspace() {
    std::cout << "IA05: Backspace..." << std::endl;
    assert(key_from_event(Event::Backspace) == Key::backspace());
    pass("IA05");
}

// ---------------------------------------------------------------------------
// IA06: Space character → Key::space()
// ---------------------------------------------------------------------------
static void test_space_character() {
    std::cout << "IA06: Character(' ') -> Key::space()..." << std::endl;
    assert(key_from_event(Event::Character(" ")) == Key::space());
    // Must NOT produce Key::character(' ')
    assert(key_from_event(Event::Character(" ")).code == KeyCode::Space);
    pass("IA06");
}

// ---------------------------------------------------------------------------
// IA07: Printable characters
// ---------------------------------------------------------------------------
static void test_printable_characters() {
    std::cout << "IA07: printable characters..." << std::endl;
    assert(key_from_event(Event::Character("k")) == Key::character('k'));
    assert(key_from_event(Event::Character("n")) == Key::character('n'));
    assert(key_from_event(Event::Character("/")) == Key::character('/'));
    assert(key_from_event(Event::Character("q")) == Key::character('q'));
    assert(key_from_event(Event::Character("a")) == Key::character('a'));
    assert(key_from_event(Event::Character("?")) == Key::character('?'));
    pass("IA07");
}

// ---------------------------------------------------------------------------
// IA08: Custom (ticker) event → Key::unknown()
// ---------------------------------------------------------------------------
static void test_custom_event() {
    std::cout << "IA08: Custom (ticker) event -> Key::unknown()..." << std::endl;
    assert(key_from_event(Event::Custom).is_unknown());
    pass("IA08");
}

// ---------------------------------------------------------------------------
// IA09: Returned Key types are distinct — no cross-mapping
// ---------------------------------------------------------------------------
static void test_key_distinctness() {
    std::cout << "IA09: distinct keys are not equal..." << std::endl;
    assert(key_from_event(Event::ArrowUp)    != key_from_event(Event::ArrowDown));
    assert(key_from_event(Event::ArrowLeft)  != key_from_event(Event::ArrowRight));
    assert(key_from_event(Event::Return)     != key_from_event(Event::Escape));
    assert(key_from_event(Event::Tab)        != key_from_event(Event::TabReverse));
    assert(key_from_event(Event::Character("n")) != key_from_event(Event::Character("k")));
    assert(key_from_event(Event::Character(" ")) != key_from_event(Event::Character("a")));
    // Space (canonical) must not equal character('n')
    assert(key_from_event(Event::Character(" ")) != key_from_event(Event::Character("n")));
    pass("IA09");
}

// ---------------------------------------------------------------------------
// IA10: Input values map to the correct Key::parse() counterpart
//       (verifies the adapter and config vocabulary stay aligned)
// ---------------------------------------------------------------------------
static void test_adapter_config_alignment() {
    std::cout << "IA10: adapter output aligns with Key::parse()..." << std::endl;
    assert(key_from_event(Event::ArrowUp)        == Key::parse("ArrowUp"));
    assert(key_from_event(Event::ArrowDown)       == Key::parse("ArrowDown"));
    assert(key_from_event(Event::ArrowLeft)       == Key::parse("ArrowLeft"));
    assert(key_from_event(Event::ArrowRight)      == Key::parse("ArrowRight"));
    assert(key_from_event(Event::Return)          == Key::parse("Enter"));
    assert(key_from_event(Event::Escape)          == Key::parse("Escape"));
    assert(key_from_event(Event::Tab)             == Key::parse("Tab"));
    assert(key_from_event(Event::TabReverse)      == Key::parse("ShiftTab"));
    assert(key_from_event(Event::Backspace)       == Key::parse("Backspace"));
    assert(key_from_event(Event::Character(" "))  == Key::parse("Space"));
    assert(key_from_event(Event::Character("k"))  == Key::parse("k"));
    assert(key_from_event(Event::Character("/"))  == Key::parse("/"));
    pass("IA10");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "--- InputAdapter Test Runner ---" << std::endl;

    test_arrow_keys();
    test_return();
    test_escape();
    test_tab();
    test_backspace();
    test_space_character();
    test_printable_characters();
    test_custom_event();
    test_key_distinctness();
    test_adapter_config_alignment();

    std::cout << "\nAll InputAdapter assertions passed! (10 tests)" << std::endl;
    return 0;
}
