#!/usr/bin/env bash
set -e

echo "======================================"
echo "    Txplay v2.0 Installer             "
echo "======================================"
echo ""

# 1. Detect operating environment
IS_TERMUX=false
if [[ -n "$PREFIX" ]] && [[ "$PREFIX" == *"/com.termux/"* ]]; then
    IS_TERMUX=true
    echo "Environment: Termux/Android detected."
else
    echo "Environment: Standard Linux detected."
fi

# 2. Check required build tools
echo "Checking dependencies..."
REQUIRED_CMDS=("git" "cmake" "make")
if [ "$IS_TERMUX" = true ]; then
    REQUIRED_CMDS+=("clang")
else
    # Allow gcc or clang on standard linux
    if ! command -v gcc >/dev/null 2>&1 && ! command -v clang >/dev/null 2>&1; then
        echo "Error: Neither gcc nor clang is installed. A C++ compiler is required."
        exit 1
    fi
fi

for cmd in "${REQUIRED_CMDS[@]}"; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "Error: Required command '$cmd' is not installed."
        if [ "$IS_TERMUX" = true ]; then
            echo "Run: pkg install git cmake make clang"
        else
            echo "Please install $cmd via your package manager."
        fi
        exit 1
    fi
done
echo "All dependencies met."

# 3. Clone or update repository
SRC_DIR="$HOME/.txplay"
REPO_URL="https://github.com/riftrogue/txplay.git"

if [ -d "$SRC_DIR/.git" ]; then
    echo "Updating existing Txplay source at $SRC_DIR..."
    cd "$SRC_DIR"
    git fetch origin
    git reset --hard origin/main
else
    echo "Cloning Txplay source to $SRC_DIR..."
    git clone "$REPO_URL" "$SRC_DIR"
    cd "$SRC_DIR"
fi

# 4. Configure CMake & Build
echo "Configuring build with CMake..."
rm -rf build
cmake -S . -B build

echo "Building Txplay..."
# Use all available cores
CORES=$(nproc 2>/dev/null || echo 1)
cmake --build build -j"$CORES"

if [ ! -f "build/txplay" ]; then
    echo "Error: Build failed. Binary 'build/txplay' not found."
    exit 1
fi

# 6. Install the binary
if [ "$IS_TERMUX" = true ]; then
    BIN_DIR="$PREFIX/bin"
else
    BIN_DIR="$HOME/.local/bin"
fi

echo "Installing binary to $BIN_DIR..."
mkdir -p "$BIN_DIR"
cp build/txplay "$BIN_DIR/txplay"
chmod +x "$BIN_DIR/txplay"

# 7. Setup Configuration
if [ "$IS_TERMUX" = true ]; then
    CONFIG_DIR="$HOME/.config/txplay"
else
    CONFIG_DIR="$HOME/.config/txplay"
fi

echo "Setting up configuration at $CONFIG_DIR..."
mkdir -p "$CONFIG_DIR"

if [ ! -f "$CONFIG_DIR/config.txt" ]; then
    echo "Creating default configuration..."
    cp config.txt "$CONFIG_DIR/config.txt"
else
    echo "Existing configuration found. Keeping user config."
fi

# 8. Success message
echo ""
echo "======================================"
echo "    Txplay v2.0 Installed!            "
echo "======================================"
echo "Executable: $BIN_DIR/txplay"
echo "Config:     $CONFIG_DIR/config.txt"
echo ""

# Make sure the user's PATH includes the installation directory if on Linux
if [ "$IS_TERMUX" = false ]; then
    if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
        echo "NOTE: $BIN_DIR is not in your PATH."
        echo "Please add it to your ~/.bashrc or ~/.zshrc:"
        echo "    export PATH=\"$BIN_DIR:\$PATH\""
        echo ""
    fi
fi

echo "You can now run Txplay by typing: txplay"
