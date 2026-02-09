#!/usr/bin/env bash
# txplay installation script
# Supports both fresh install and updates

set -e

INSTALL_DIR="$HOME/.txplay"
BIN_DIR="$HOME/.local/bin"
REPO_URL="https://github.com/riftrogue/txplay.git"

# Allow branch override via environment variable or argument
# Usage: BRANCH=test bash install.sh  OR  bash install.sh test
BRANCH="${BRANCH:-${1:-main}}"

# Function to check and auto-install system dependencies
install_system_deps() {
    local deps_to_install=()
    
    echo "==> Checking system dependencies..."
    
    # Check each required package
    for cmd in git python mpv; do
        if ! command -v $cmd &> /dev/null; then
            echo "  ⚠ $cmd not found"
            deps_to_install+=("$cmd")
        else
            echo "  ✓ $cmd found"
        fi
    done
    
    # If missing packages, install them
    if [ ${#deps_to_install[@]} -gt 0 ]; then
        echo ""
        echo "Missing dependencies: ${deps_to_install[*]}"
        echo ""
        echo -n "Do you want to install these packages? (y/n): "
        read -r REPLY < /dev/tty
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Installation cancelled."
            exit 1
        fi
        echo "==> Installing: ${deps_to_install[*]}"
        pkg install "${deps_to_install[@]}"
        echo "  ✓ Dependencies installed successfully"
    else
        echo "  ✓ All system dependencies already installed"
    fi
}

echo "==> Installing txplay..."

# Install system dependencies first
install_system_deps

# Create directories
mkdir -p "$INSTALL_DIR" "$BIN_DIR"

# Clone or update repository
if [ -d "$INSTALL_DIR/.git" ]; then
    echo "==> Updating existing installation..."
    cd "$INSTALL_DIR"
    git fetch origin
    git checkout "$BRANCH"
    git pull origin "$BRANCH"
else
    echo "==> Cloning repository..."
    git clone -b "$BRANCH" "$REPO_URL" "$INSTALL_DIR"
    cd "$INSTALL_DIR"
fi

# Install Python dependencies
echo "==> Installing Python dependencies..."
if ! python -m pip install --user -r requirements.txt; then
    echo "Error: Failed to install Python dependencies"
    echo "Try manually: python -m pip install --user ytmusicapi yt-dlp"
    exit 1
fi
echo "  ✓ Python dependencies installed"

# Install launcher
echo "==> Installing launcher..."
cp "$INSTALL_DIR/txplay" "$BIN_DIR/txplay"
chmod +x "$BIN_DIR/txplay"

# Ensure ~/.local/bin is in PATH
if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
    echo "==> Adding ~/.local/bin to PATH..."
    
    # Termux primarily uses bash
    SHELL_CONFIG="$HOME/.bashrc"
    
    # Add to PATH if not already there
    if ! grep -q 'export PATH="$HOME/.local/bin:$PATH"' "$SHELL_CONFIG" 2>/dev/null; then
        echo '' >> "$SHELL_CONFIG"
        echo '# Added by txplay installer' >> "$SHELL_CONFIG"
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$SHELL_CONFIG"
        echo "   Added to $SHELL_CONFIG"
        echo "   Restart terminal or run: source $SHELL_CONFIG"
    fi
fi

echo ""
echo "✓ txplay installed successfully!"
echo ""
echo "To run txplay, type: txplay"
echo ""
echo "NOTE: If command not found, restart your terminal"
