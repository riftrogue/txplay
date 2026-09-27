# Txplay UI Redesign Progress

## Overall Status
**PHASE 4 COMPLETE** — Visual refinement done. Build clean. All tests pass.

---

## Completed Phases

### [x] Pre-requisite: Metadata Integration Committed
- Commit: `feat(metadata): integrate TagLib v2.0.2 for embedded audio metadata`
- TagLib integrated, all T1–T6 tests passing, real-file verified.

### [x] Phase 0 — Inspection + Phase Plan
- Read current `TxplayUI.cpp` (469 lines, one giant `build_ui()`)
- Read `ui_architecture_audit.md` (source-verified)
- Confirmed backend contracts (Application, Config APIs unchanged)
- Decided phase breakdown

### [x] Phase 1 — Core Utilities + Tests
- Extended `src/ui/Util.hpp`:
  - `format_track_row(title, artist)` → "Title - Artist" display string
  - `truncate_track_row(title, artist, max_cols)` → intelligent truncation
    that always tries to preserve the artist portion
- Created `src/ui/ui_util_test_runner.cpp` with tests for all utilities
- Added `ui_util_test_runner` CMake target
- **All tests pass. Zero compiler warnings.**

### [x] Phase 2 — Full UI Redesign Implementation
All redesign requirements implemented in one atomic phase:

**New files created:**
- `src/ui/components/Header.hpp` — minimal `TXPLAY ... LOCAL` header bar
- `src/ui/components/NowPlaying.hpp` — persistent now-playing bar component

**Files modified:**
- `src/ui/TxplayUI.hpp` — added `SmallScreenView` enum for small-screen nav
- `src/ui/TxplayUI.cpp` — complete redesign:
  - Header bar rendered at top of all layouts
  - Songs displayed as "Title - Artist" via `format_track_row()`
  - Queue entries displayed as "Title - Artist"
  - NowPlaying: "▶ Title - Artist   01:24 / 04:12" with seek bar
  - Cleaner nothing-playing state (no ugly "⏹ Nothing Playing" text)
  - Visualizer: when disabled, region removed entirely (no empty box)
  - Small-screen mode: tab bar shows [SONGS] / [QUEUE], Tab toggles
  - Layout breakpoints: kWideWidth=100, kMediumWidth=60 (same as before)
  - All existing keybindings preserved exactly

**Unchanged (verified):**
- `src/ui/Visualizer.hpp/.cpp` — untouched
- `src/ui/InputAdapter.hpp/.cpp` — untouched
- `src/ui/Widgets.hpp` — untouched
- All backend files — untouched

### [x] Phase 3 — Full Regression + Commit
- All 5 test runners passed with zero failures:
  - `config_test_runner`: 26/26
  - `input_adapter_test_runner`: 10/10
  - `ui_util_test_runner`: 3/3
  - `library_test_runner`: all pass (T1–T6 + cancellation)
  - `application_test_runner`: all pass (Q1–Q13, NP1–NP10, SN1–SN2)
- Full `txplay` binary builds clean, zero warnings
- Diff reviewed: only UI layer files changed, no backend regression
- Committed with message: `feat(ui): redesign UI with header, Title-Artist rows, intelligent truncation`

---

---

## Architecture (Current State)

```
TxplayUI.cpp (orchestration)
    ├── components/Header.hpp       (pure element: TXPLAY ... LOCAL)
    ├── components/NowPlaying.hpp   (pure element: track + progress)
    ├── Util.hpp                    (format_track_row, truncate_track_row)
    ├── Widgets.hpp                 (build_seek_bar)
    ├── Visualizer.hpp/.cpp         (render_visualizer)
    └── InputAdapter.hpp/.cpp       (key_from_event)
```

Backend boundary untouched:
```
TxplayUI → Application (public API only)
TxplayUI → Config (reads only)
```

---

## Files Changed (cumulative)

| File | Status | Why |
|---|---|---|
| `src/ui/Util.hpp` | Modified | Added format_track_row, truncate_track_row |
| `src/ui/TxplayUI.hpp` | Modified | Added SmallScreenView enum |
| `src/ui/TxplayUI.cpp` | Modified | Full redesign |
| `src/ui/ui_util_test_runner.cpp` | New | Pure function tests |
| `src/ui/components/Header.hpp` | New | Header bar component |
| `src/ui/components/NowPlaying.hpp` | New | Now-playing bar component |
| `CMakeLists.txt` | Modified | ui_util_test_runner target |

---

## Tests Run

| Test Runner | Result |
|---|---|
| `config_test_runner` | ✅ 26/26 |
| `input_adapter_test_runner` | ✅ 10/10 |
| `ui_util_test_runner` | ✅ 3/3 |
| `library_test_runner` | ✅ All pass |
| `application_test_runner` | ✅ All pass |
| `txplay` build | ✅ Zero warnings |

---

## Decisions Made

1. **NowPlaying component**: renders both compact and wide via single function, `compact` bool param
2. **Visualizer off = no element at all**: `vis_enabled` check produces `text("")` placeholder (empty), the `vbox` simply doesn't include it
3. **Small screen**: Tab toggles `SmallScreenView` enum (Songs ↔ Queue); visible tab bar shows current pane  
4. **Song rows**: `format_track_row(title, artist)` — no width truncation in the list (FTXUI Menu clips naturally at container edge); truncation only in NowPlaying where space is precious
5. **Queue rows**: same `format_track_row()` as song rows
6. **Breakpoints unchanged**: kWideWidth=100, kMediumWidth=60 — same as original, no need to change

---

## Known Issues
- `truncate_track_row` counts bytes, not Unicode display width. For multi-byte characters (e.g. CJK), displayed width may differ from byte count. This is consistent with how FTXUI renders `text()` items. Acceptable for now.
- `format_time` uses `%lu` format specifiers which are correct on Linux/64-bit but may warn on some compilers. Not a new issue.

---

## Phase 4 — Visual / UX Refinement

### Visual Problems Identified
1. `window()` boxes around Songs, Queue created heavy rectangles consuming visual weight
2. Visualizer had `| border` — third heavy box
3. NowPlaying had `| border` — looked like another panel, not a footer
4. Header had no visual separation from content below
5. Search had no left-side decoration and no styled placeholder
6. FTXUI default Menu used full-width inverted highlight blocks
7. Songs/Queue took equal horizontal space (1:1 ratio) regardless of importance
8. Nothing-playing state used ugly hardcoded text
9. Idle progress bar used ASCII dashes, inconsistent with seek bar
10. Visualizer had no separator to anchor it visually in the layout

### Changes Made

#### `src/ui/components/Header.hpp`
- Lowercased app name: `TXPLAY → txplay` (calmer, more modern)
- Lowercased badge: `LOCAL → local`
- Added `separator()` below header hbox — single line that separates identity from content
- Removed color decoration; cyan for app name only

#### `src/ui/components/NowPlaying.hpp`
- Removed `| border` entirely — replaced with `separator()` above
- Component now feels like an anchored footer, not a framed panel
- Idle state: `·  ready` + `●────` (seek bar at 0) — subtle, not an error message
- Playing state: `▶  Title - Artist  (right-aligned)  01:24 / 04:24`
- Track label color: `Color::White` (was `Color::Cyan`) — calmer during playback
- Status icon when stopped: `·` (middle dot) instead of `⏹` — much less intrusive

#### `src/ui/TxplayUI.cpp`

**Song list:**
- Removed `window()` box — replaced with section label `songs` in `Color::GrayDark` + `separator()`
- Songs column gets `xflex_grow_factor(2)` on wide, `xflex_grow_factor(1)` on medium
- Queue column gets `xflex_grow_factor(1)` always
- Vertical separator `|` between columns replaces right-edge box borders

**Queue:**
- Same treatment as songs — section label + separator, no box
- Queue items shown as `Title - Artist`; empty state shows `empty` (lowercase, subtle)

**Selection style (custom MenuOption transform):**
- Songs focused row: `▸ Title - Artist` (cyan `▸` + white bold text)
- Songs active row (focus elsewhere): `▸ Title - Artist` (gray `▸` + gray-light text)
- Songs normal row: `  Title - Artist` (gray-light text, no cursor)
- Queue focused row: `· Title - Artist` (cyan `·` + white text)
- Queue normal row: `  Title - Artist` (dark gray text — visually secondary)

**Search bar:**
- Custom `InputOption::transform`: placeholder=GrayDark, focused=White, unfocused=GrayLight
- Left decoration: `│` (box pipe) in Cyan when focused, `·` in GrayDark otherwise
- Always shows the decoration for visual anchor

**Visualizer:**
- Removed `| border`
- Added `separator()` above visualizer element so it doesn’t float free
- Renders flush in the layout with no extra framing
- Disabled = nothing in the layout (existing behavior preserved)

**Layout composition:**
- Wide (>= 100): `header | search | [songs 2:1 queue] | [vis] | now-playing`
- Medium (>= 60): `header | search | [songs 1:1 queue] | [vis] | now-playing`
- Small (< 60): `header | [songs]/[queue] tab | search | active-pane | [vis] | now-playing`

**Borders used (after refinement):**
- Header: none (separator line only)
- Search: none
- Songs: none (separator line only)
- Queue: none (separator line only)
- Visualizer: none (separator line only above)
- NowPlaying: none (separator line only above)
- Total rectangular borders: **0** (was 4)

### Tests
- `config_test_runner`: 26/26 ✅
- `input_adapter_test_runner`: 10/10 ✅
- `ui_util_test_runner`: 3/3 ✅
- `library_test_runner`: all pass ✅
- `application_test_runner`: all pass ✅
- `txplay` build: clean, zero errors ✅

### Files Changed
| File | Change |
|---|---|
| `src/ui/components/Header.hpp` | Lowercase, separator, color refinement |
| `src/ui/components/NowPlaying.hpp` | Remove border, separator footer, idle seek bar |
| `src/ui/TxplayUI.cpp` | Custom MenuOption, box-free layout, search style, vis separator |
| `src/ui/layout_preview.cpp` | New: offline visual inspection tool |
| `CMakeLists.txt` | Added `layout_preview` target |

### Functionality Preserved
- All keyboard shortcuts work identically
- Search: `/ ` key focuses, filter works
- Song selection + playback (Enter / click)
- Pause/resume, next, previous, seek
- Queue: add, remove, clear, navigate
- Small-screen Tab toggle (Songs ⇔ Queue)
- Visualizer on/off (controlled by config)
- Visualizer disabled = no empty region
- Now playing: playing/paused/stopped states all correct

### Remaining Visual Issues
1. FTXUI’s `vscroll_indicator` draws a small scrollbar on the right edge of the menu frame — may look odd in very narrow terminals. Acceptable for now.
2. The `frame` decorator on menu content adds a small implicit margin. Removing it may cause cursor-follow to stop working. Left in place.
3. Wide layout does not reserve a fixed height for the visualizer — its height comes from `config.visualizer.height` (default 6). Large values may crowd the song list. User-configurable; not a code bug.
4. On terminals with no true-color support, some `Color::GrayDark` / `Color::GrayLight` differences may collapse. This is a terminal capability issue, not a code issue.
5. The `layout_preview.cpp` tool is not shipped but lives in `src/ui/` and `CMakeLists.txt`. Should be moved to a `tools/` or `dev/` directory in a future cleanup.

## What the Next Session Must NOT Redo
- The metadata integration is committed. Do not redo it.
- The utility functions are tested. Do not change their signatures.
- The component architecture is established. Build on it.
- The backend was not touched. Do not touch it.
- Keybinding architecture is preserved exactly. Do not change it.
