# Txplay

**Txplay v2.0**

Txplay is a fast, lightweight terminal music player written in C++17. It is designed entirely around local-first playback, low dependencies, and a highly responsive terminal UI.

## Current Status

**Stable Release (v2.0)**

Txplay has been completely rebuilt from its legacy Python/MPV prototype into a standalone, statically-compiled C++17 application. The core architecture is solid, leveraging `miniaudio` for native decoding and `FTXUI` for a fluid dashboard. 

Development is currently active. Playback, seeking, background library scanning, and visualization are fully implemented. The Queue and Auto-Next functionalities are currently under active development.

## Features

- **C++17 Architecture**: Multi-threaded and resource-efficient with strict RAII ownership.
- **Terminal UI**: Built on FTXUI for a robust, mouse-aware, and beautiful interface.
- **Local-first Playback**: Asynchronous, background scanning of configured music directories.
- **Audio Decoding**: Handled natively by miniaudio (no system audio daemons or heavy multimedia frameworks required).
- **Format Support**: MP3, WAV, and FLAC playback.
- **Responsive Controls**: Global keybindings, mouse click-to-play, hover highlighting, and seeking.
- **Queue & Autoplay**: Add tracks to a live FIFO queue. Enable `autoplay=true` in config for continuous, uninterrupted playback. Missing queued tracks are skipped automatically.
- **Visualizer**: Real-time FFT terminal audio visualizer running on a dedicated thread.
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

Configuration is managed via a plaintext INI file located at `~/.config/txplay/config.txt`. The installer creates a safe default if one doesn't exist.

**Example `config.txt`:**
```ini
[Library]
music_path=~/Music
music_path=~/Downloads

[Visualizer]
enabled=true
style=bars
height=6

[Navigation]
play=enter
pause=space
search=/
refresh=r
quit=q
seek_forward=right
seek_backward=left
```

## Controls

- **Space** or **p**: Toggle play/pause (Global)
- **/**: Focus the Search box (Global)
- **r**: Rescan and refresh the library (Global)
- **q**: Quit the application (Global)
- **Tab** / **Shift+Tab**: Cycle keyboard focus between UI zones
- **Up / Down**: Navigate library or queue rows
- **Enter**: Play the currently selected track
- **Left / Right**: Seek playback (-5s / +5s)
- **Mouse Click**: Moves selection and immediately plays the clicked track
- **a**: Add highlighted Library track to Queue (configurable via `queue_add`)
- **d**: Remove selected Queue entry — Queue pane must have focus (configurable via `queue_remove`)
- **c**: Clear entire Queue — Queue pane must have focus (configurable via `queue_clear`)

## Architecture Overview

Txplay enforces a strict boundary between the UI, the Application state orchestrator, and the backend hardware engines. It relies on a lock-free SPSC Ring Buffer to pass audio from the decoder thread to the hardware callback safely. 

For detailed technical documentation regarding Threading, Architecture, and Development, consult the [docs/](./docs/) directory.

## Building from source (Developers)

```bash
git clone https://github.com/riftrogue/txplay.git
cd txplay

# Configure and build
cmake -S . -B build
cmake --build build -j$(nproc)

# Run Txplay
./build/txplay
```

Regression tests can be run via:
```bash
./build/audio_test_runner experiments/audio-test/test.mp3
./build/library_test_runner
./build/application_test_runner
```

## License
MIT License
