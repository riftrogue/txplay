#pragma once
#include <string>

namespace txplay::common {

// ---------------------------------------------------------------------------
// KeyCode — discriminates special keys from printable character keys.
// ---------------------------------------------------------------------------
enum class KeyCode {
    Character,   // ch field contains the printable character
    Space,
    Enter,
    Escape,
    Tab,
    ShiftTab,
    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,
    Backspace,
    Unknown      // unrecognized or unsupported input
};

// ---------------------------------------------------------------------------
// Key — RIGHT-side keyboard key representation.
//
// Rules:
//   Special keys  → code is the specific KeyCode; ch is '\0'.
//   Character keys → code is KeyCode::Character; ch holds the character.
//   Unknown input  → code is KeyCode::Unknown; ch is '\0'.
//
// This type is the only place in the codebase that names keyboard keys.
// It must not depend on FTXUI.
// ---------------------------------------------------------------------------
struct Key {
    KeyCode code{KeyCode::Unknown};
    char    ch{'\0'};   // only meaningful when code == Character

    // ---- Static factories --------------------------------------------------
    static inline Key character(char c)  { return {KeyCode::Character, c}; }
    static inline Key space()            { return {KeyCode::Space};      }
    static inline Key enter()            { return {KeyCode::Enter};      }
    static inline Key escape()           { return {KeyCode::Escape};     }
    static inline Key tab()              { return {KeyCode::Tab};        }
    static inline Key shift_tab()        { return {KeyCode::ShiftTab};   }
    static inline Key arrow_up()         { return {KeyCode::ArrowUp};    }
    static inline Key arrow_down()       { return {KeyCode::ArrowDown};  }
    static inline Key arrow_left()       { return {KeyCode::ArrowLeft};  }
    static inline Key arrow_right()      { return {KeyCode::ArrowRight}; }
    static inline Key backspace()        { return {KeyCode::Backspace};  }
    static inline Key unknown()          { return {KeyCode::Unknown};    }

    // ---- Comparison --------------------------------------------------------
    inline bool operator==(const Key& o) const {
        if (code != o.code) return false;
        if (code == KeyCode::Character) return ch == o.ch;
        return true;
    }
    inline bool operator!=(const Key& o) const { return !(*this == o); }

    inline bool is_unknown() const { return code == KeyCode::Unknown; }

    // ---- Config round-trip -------------------------------------------------

    // parse() — convert a config.txt key name into a Key.
    //
    // Supported special key names (case-sensitive):
    //   "Space", "Enter", "Escape", "Tab", "ShiftTab",
    //   "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight",
    //   "Backspace"
    //
    // Character keys:
    //   Any single printable character (e.g. "n", "/", "?", "a").
    //
    // Anything else (including old aliases like "right", "space") → Key::unknown().
    static inline Key parse(const std::string& name) {
        if (name == "Space")     return space();
        if (name == "Enter")     return enter();
        if (name == "Escape")    return escape();
        if (name == "Tab")       return tab();
        if (name == "ShiftTab")  return shift_tab();
        if (name == "ArrowUp")   return arrow_up();
        if (name == "ArrowDown") return arrow_down();
        if (name == "ArrowLeft") return arrow_left();
        if (name == "ArrowRight")return arrow_right();
        if (name == "Backspace") return backspace();
        // Single printable character
        if (name.size() == 1 && name[0] >= 0x20 && name[0] < 0x7f)
            return character(name[0]);
        return unknown();
    }

    // name() — serialize back to the canonical config string (inverse of parse).
    // Used by Config::save().
    inline std::string name() const {
        switch (code) {
            case KeyCode::Space:     return "Space";
            case KeyCode::Enter:     return "Enter";
            case KeyCode::Escape:    return "Escape";
            case KeyCode::Tab:       return "Tab";
            case KeyCode::ShiftTab:  return "ShiftTab";
            case KeyCode::ArrowUp:   return "ArrowUp";
            case KeyCode::ArrowDown: return "ArrowDown";
            case KeyCode::ArrowLeft: return "ArrowLeft";
            case KeyCode::ArrowRight:return "ArrowRight";
            case KeyCode::Backspace: return "Backspace";
            case KeyCode::Character: return std::string(1, ch);
            default:                 return "Unknown";
        }
    }
};

} // namespace txplay::common
