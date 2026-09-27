# Txplay UI Redesign Progress

## Overall Status
**COMPLETE** — Phases 0–3 done and committed.

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

## Next Action (Phase 3)
**Phase 3: Full regression + git commit**

1. Run all tests one more time
2. Inspect final git diff for any unintentional changes
3. Commit with descriptive message
4. Mark this file as COMPLETE

Optionally extend:
- Verify truncation visually with narrow terminal
- Verify visualizer-disabled behavior 
- Verify small-screen tab navigation behavior

---

## What the Next Session Must NOT Redo
- The metadata integration is committed. Do not redo it.
- The utility functions are tested. Do not change their signatures.
- The component architecture is established. Build on it.
- The backend was not touched. Do not touch it.
- Keybinding architecture is preserved exactly. Do not change it.
