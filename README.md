# Txplay

Txplay v2.0

Txplay is a lightweight terminal music player written in C++17, designed around local-first playback, low dependencies, and a responsive terminal UI.

## Features

- **C++17 Architecture**: Fast, multi-threaded, and resource-efficient.
- **Terminal UI**: Built on FTXUI for a robust, responsive, and beautiful interface.
- **Local-first Playback**: Configurable local music directories with asynchronous scanning.
- **Audio Decoding**: Handled natively by miniaudio (no system audio daemons or heavy multimedia frameworks required).
- **Format Support**: MP3, WAV, FLAC playback.
- **Responsive Controls**: Mouse interaction, keyboard navigation, seeking, and playback state display.
- **Visualizer**: Real-time terminal audio visualizer with configurable styles.
- **Portability**: Verified support for Fedora/Linux and Termux/Android.

## Requirements

- A modern terminal emulator
- Linux (e.g. Fedora, Ubuntu, Arch) or Android (via Termux)

For building from source:
- `cmake` (>= 3.11)
- `clang` or `gcc` (C++17 support)
- `make`
- `git`

## Installation

You can install or update Txplay directly using the provided install script. It will detect your environment, configure the build, compile from source, and install the binary cleanly.

```bash
curl -fsSL https://raw.githubusercontent.com/riftrogue/txplay/main/install.sh | bash
```

**Where does it install?**
- **Linux**: `~/.local/bin/txplay`
- **Termux**: `$PREFIX/bin/txplay`

The source code is cloned and built locally in `~/.txplay`. Running the installer again will safely update your installation from the latest source without overwriting your configuration.

## Configuration

Txplay uses a simple `config.txt` file for configuration. 

**Location:**
- **Linux**: `~/.config/txplay/config.txt`
- **Termux**: `$HOME/.config/txplay/config.txt`

The installer will create a default configuration if one does not exist. Txplay plays files directly from their configured locations and does not copy your music files into its own directory.

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

Txplay provides a focused and intuitive interaction model.

### Global Shortcuts
*(Active when the Search input is not focused)*
- **Space** or **p**: Toggle play/pause
- **/**: Focus the Search box
- **r**: Rescan and refresh the library
- **q**: Quit the application
- **Tab** / **Shift+Tab**: Cycle keyboard focus between UI zones (Search -> Library -> Queue)

### Library Navigation
- **Up / Down**: Move the selection arrow
- **Enter**: Play the currently selected track
- **Left / Right**: Seek playback (-5s / +5s)

### Search Context
- When the Search box holds focus, it securely owns text-entry events. `Space`, `/`, `r`, `Left`, and `Right` will act as normal alphanumeric typing and cursor movement.

### Mouse
- **Hover**: Passively highlights library rows
- **Click**: Moves selection and immediately plays the clicked track

## Supported Formats

- MP3
- WAV
- FLAC

## Building from source

If you prefer to build manually instead of using the installer:

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

**Dependencies:**
- `miniaudio` is vendored directly into the repository.
- `FTXUI` is automatically fetched and built via CMake.

## Project Structure
```
txplay/
├── CMakeLists.txt        # Build configuration
├── docs/                 # Detailed architectural documentation
├── experiments/          # Regression test fixtures
├── src/                  # Core application source code
└── third_party/          # Vendored dependencies (miniaudio)
```

## Current Status

**Txplay v2.0**

This release marks the completion of the C++17 rebuild from the legacy Python/MPV implementation. The core playback engine, terminal integration, and library structure are stable.

*Unfinished/Planned Features:*
- **Queue**: Planned (UI placeholder exists, but not implemented)
- **Lyrics**: Not implemented
- **Online playback**: Not implemented
- **Metadata parsing**: Not implemented (Currently falls back to filename extraction)

## Architecture

Txplay is structured around explicit ownership, zero bloat, and responsive threading:

```
FTXUI (Terminal UI)
  ↓
Application (State Orchestration)
  ├── Config (INI Parsing)
  ├── Library (Async File Scanning)
  └── AudioEngine (Playback Management)
          ↓
      miniaudio (Decoding & Output)
```

- **Library** discovers configured local files asynchronously.
- **Application** orchestrates UI and backend state cleanly.
- **AudioEngine** handles threaded playback, seeking, and visualizer bridging.
- **miniaudio** handles all raw decoding and hardware output.
- **FTXUI** manages the terminal DOM and event loops.

For deeper technical details, see the `docs/` directory.

## License
MIT License
