# Txplay

**Txplay v2.0**

Txplay is a fast, lightweight terminal music player written in C++17. It is designed entirely around local-first playback, low dependencies, and a highly responsive terminal UI.

## Current Status

**Stable Release (v2.0)**

Txplay has been completely rebuilt from its legacy Python/MPV prototype into a standalone C++17 application. The core architecture is solid, leveraging `miniaudio` for native decoding and `FTXUI` for a fluid dashboard.

Playback, seeking, background library scanning, visualization, queue, and autoplay are all fully implemented.

## Features

- **C++17 Architecture**: Multi-threaded and resource-efficient with strict RAII ownership.
- **Terminal UI**: Built on FTXUI for a robust, mouse-aware, and beautiful interface.
- **Local-first Playback**: Asynchronous, background scanning of configured music directories.
- **Audio Decoding**: Handled natively by miniaudio (no system audio daemons or heavy multimedia frameworks required).
- **Format Support**: MP3, WAV, and FLAC playback.
- **Responsive Controls**: Configurable keybindings, mouse click-to-play, hover highlighting, and seeking.
- **Queue**: Add tracks to a live FIFO queue. Queue always has priority over autoplay.
- **Autoplay**: Continue automatically into local tracks after the queue is exhausted.
- **Autoplay Limit**: Cap automatic local track playback at a configurable count.
- **Visualizer**: Real-time FFT bar visualizer running on a dedicated thread.
- **Portability**: Verified native support for Fedora/Linux and Termux/Android.

## Installation

Txplay is distributed with an `install.sh` script that automatically detects your platform (Linux or Termux), checks for minimal build dependencies (CMake/Compiler), compiles the C++ code, and installs the binary cleanly.

```bash
curl -fsSL https://raw.githubusercontent.com/riftrogue/txplay/main/install.sh | bash
```

**Binary Locations:**
- **Linux**: `~/.local/bin/txplay`
- **Termux**: `$PREFIX/bin/txplay`

## Configuration

Configuration is managed via a plaintext INI file at `~/.config/txplay/config.txt`. The installer creates a safe default if one doesn't exist.

**Example `config.txt`:**
```ini
[Library]
# Directories to scan. Repeat for multiple locations. ~ is supported.
music_path=~/Music
music_path=~/music
music_path=~/Songs
music_path=~/songs

[Playback]
# Continue automatically after queue empties (false = stop).
autoplay=true
# Limit local auto-plays per session.
autoplay_limit=false
# Max local tracks to auto-play (only active when autoplay_limit=true).
autoplay_limit_value=10
# Seek step in seconds (1-300).
seek_seconds=5

[Visualizer]
enabled=true
style=bars
height=6

[Keybindings]
pause=space
search=/
refresh=r
quit=q
seek_forward=right
seek_backward=left
queue_add=a
queue_remove=d
queue_clear=c
```

### Autoplay explained

`autoplay` controls what happens after your queue finishes, **not** queue behavior itself:

```
Queue empty + autoplay=false → stop
Queue empty + autoplay=true  → continue with next local track
```

With `autoplay_limit=true` and `autoplay_limit_value=10`, Txplay stops automatically after playing 10 local tracks post-queue. Queue tracks and manually-selected tracks are never counted toward this limit.

### Configurable seek keys

Seek keys can be mapped to any key, including vim-style:
```ini
seek_forward=j
seek_backward=k
```

## Controls

| Key | Action | Scope |
|---|---|---|
| `Space` | Toggle play/pause | Global |
| `/` | Focus the search box | Global |
| `r` | Rescan and refresh the library | Global |
| `q` | Quit the application | Global |
| `Left` / `Right` | Seek backward / forward (`seek_seconds`) | Global (not in search) |
| `Tab` / `Shift+Tab` | Cycle keyboard focus between UI zones | Global |
| `Up` / `Down` | Navigate library or queue rows | Focused pane |
| `Enter` | Play the currently selected track | Library pane |
| `Mouse Click` | Move selection and immediately play | Library / Queue |
| `a` | Add highlighted library track to queue | Global (not in search) |
| `d` | Remove selected queue entry | Queue pane focus only |
| `c` | Clear entire queue | Queue pane focus only |

All keys except `Enter`, `Tab`, and mouse are configurable in `[Keybindings]`.

## Architecture Overview

Txplay enforces a strict boundary between the UI, the Application state orchestrator, and the backend hardware engines. It relies on a lock-free SPSC Ring Buffer to pass audio from the decoder thread to the hardware callback safely. The visualizer runs on its own dedicated thread.

For detailed technical documentation consult the [docs/](./docs/) directory:
- [docs/architecture.md](./docs/architecture.md) — component map, data flow, ownership
- [docs/configuration.md](./docs/configuration.md) — authoritative settings reference
- [docs/features.md](./docs/features.md) — implemented features
- [docs/roadmap.md](./docs/roadmap.md) — planned and future work
- [docs/development.md](./docs/development.md) — build, test, contribute
- [docs/testing.md](./docs/testing.md) — test suites and coverage

## Building from Source (Developers)

```bash
git clone https://github.com/riftrogue/txplay.git
cd txplay

# Configure and build
cmake -S . -B build
cmake --build build -j$(nproc)

# Run Txplay
./build/txplay
```

Regression tests:
```bash
./build/config_test_runner
./build/library_test_runner
./build/audio_test_runner experiments/audio-test/test.mp3
./build/application_test_runner
```

## License
MIT License
