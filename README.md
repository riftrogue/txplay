# txplay

A minimal TUI (Text User Interface) music player for Termux and Linux terminals.

## Features

- 🎵 **Real MPV playback** with IPC control
- 📂 **Local music browsing** with automatic scanning
- 🎼 **Universal queue system** supporting local files (YouTube ready)
- ⚡ **Lightweight** - runs smoothly in Termux on Android
- 🎹 **Simple keyboard controls** - no mouse needed
- 📱 **Termux-optimized** - designed for mobile terminals

## Installation

### Quick Install (One-liner)

```bash
curl -fsSL https://raw.githubusercontent.com/riftrogue/txplay/installation-test/install.sh | bash
```

### Manual Installation

```bash
# Clone repository
git clone -b installation-test https://github.com/riftrogue/txplay.git ~/.txplay
cd ~/.txplay

# Install dependencies
pip3 install --user -r requirements.txt

# Install launcher
mkdir -p ~/.local/bin
cp txplay ~/.local/bin/txplay
chmod +x ~/.local/bin/txplay

# Add to PATH (if needed)
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

## Requirements

- Python 3.8+
- MPV media player
- Git

### Install on Termux (Android)

```bash
pkg update
pkg install python mpv git
curl -fsSL https://raw.githubusercontent.com/riftrogue/txplay/installation-test/install.sh | bash
```

### Install on Linux

```bash
# Debian/Ubuntu
sudo apt install python3 python3-pip mpv git

# Arch Linux
sudo pacman -S python python-pip mpv git

# Then run installer
curl -fsSL https://raw.githubusercontent.com/riftrogue/txplay/installation-test/install.sh | bash
```

## Usage

Launch the player:
```bash
txplay
```

### First Run Setup

1. Navigate to **Scan Options** from the home menu
2. Select your scan mode:
   - **Termux**: Scans `/sdcard/Music` (Android)
   - **Phone**: Scans common Android music folders
   - **Custom**: Choose your own music directory

### Keyboard Controls

#### Global Controls
- **Arrow Keys** - Navigate menus
- **Enter** - Select/Play
- **q** - Quit or go back

#### Local Music Browser
- **Enter** - Play selected track
- **a** - Add track to queue
- **n** - Play next track in queue
- **s** - Stop playback
- **PgUp** - Seek backward 10 seconds
- **PgDn** - Seek forward 10 seconds

#### Player Status
The status bar at the top shows:
- Currently playing track
- Playback position and duration
- Queue count

## Updating

Re-run the installation command to update:

```bash
curl -fsSL https://raw.githubusercontent.com/riftrogue/txplay/installation-test/install.sh | bash
```

The installer will automatically pull the latest changes.

## Project Structure

```
txplay/
├── txplay/              # Main application directory
│   ├── app.py          # Entry point
│   ├── constants.py    # Configuration constants
│   ├── core/           # Core functionality
│   │   ├── player.py   # MPV IPC player
│   │   ├── queue.py    # Universal queue manager
│   │   ├── scanner.py  # Music file scanner
│   │   └── config.py   # Configuration management
│   ├── ui/             # User interface screens
│   └── data/           # User data (config, queue)
├── requirements.txt    # Python dependencies
├── install.sh         # Installation script
└── txplay             # Launcher script
```

## Configuration

Config file location: `~/.txplay/txplay/data/config.json`

Example:
```json
{
  "scan_mode": "custom",
  "custom_scan_path": "/sdcard/Music"
}
```

## Troubleshooting

### Command not found: txplay

Ensure `~/.local/bin` is in your PATH:
```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

### MPV not playing audio

Test MPV directly:
```bash
mpv --no-video /path/to/song.mp3
```

If MPV doesn't work, reinstall it:
```bash
# Termux
pkg reinstall mpv

# Linux
sudo apt reinstall mpv  # or equivalent for your distro
```

### No music files found

1. Check your scan path in Scan Options
2. Ensure music files exist in the directory
3. Supported formats: mp3, m4a, flac, wav, ogg, opus

## Development Status

**Phase 1 Complete:** ✅
- MPV IPC player with real playback
- Universal queue system
- Local music browsing
- Keyboard controls and seeking
- Auto-advance on track end

**Planned Features:**
- YouTube Music integration
- Queue management UI
- Playlist support
- Enhanced status display

## License

MIT License - See LICENSE file for details

## Contributing

This is a personal project under active development. Feel free to fork and customize for your needs.

## Author

Built for minimal, distraction-free music playback in terminal environments.
