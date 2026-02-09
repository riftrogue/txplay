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
    
    # If missing packages, try to install
    if [ ${#deps_to_install[@]} -gt 0 ]; then
        echo "==> Installing missing dependencies: ${deps_to_install[*]}"
        
        # Check if pkg command exists (Termux)
        if command -v pkg &> /dev/null; then
            pkg install -y "${deps_to_install[@]}"
        # Check if apt exists (Debian/Ubuntu)
        elif command -v apt &> /dev/null; then
            sudo apt update
            sudo apt install -y "${deps_to_install[@]}"
        # Check if yum exists (RHEL/CentOS)
        elif command -v yum &> /dev/null; then
            sudo yum install -y "${deps_to_install[@]}"
        else
            echo "Error: No supported package manager found (pkg, apt, yum)"
            echo "Please install manually: ${deps_to_install[*]}"
            exit 1
        fi
        
        echo "  ✓ Dependencies installed successfully"
    else
        echo "  ✓ All system dependencies already installed"
    fi
}

echo "==> Installing txplay..."

# Install system dependencies first
install_system_deps

# Create directories
mkdir -p "$INSTALL_DIR"
mkdir -p "$BIN_DIR"

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
if ! python3 -m pip install --user -r requirements.txt; then
    echo "Error: Failed to install Python dependencies"
    echo "Try manually: python3 -m pip install --user ytmusicapi yt-dlp"
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
    
    # Detect shell config file
    if [ -f "$HOME/.bashrc" ]; then
        SHELL_CONFIG="$HOME/.bashrc"
    elif [ -f "$HOME/.bash_profile" ]; then
        SHELL_CONFIG="$HOME/.bash_profile"
    elif [ -f "$HOME/.zshrc" ]; then
        SHELL_CONFIG="$HOME/.zshrc"
    else
        SHELL_CONFIG="$HOME/.profile"
    fi
    
    # Add to PATH if not already there
    if ! grep -q 'export PATH="$HOME/.local/bin:$PATH"' "$SHELL_CONFIG" 2>/dev/null; then
        echo '' >> "$SHELL_CONFIG"
        echo '# Added by txplay installer' >> "$SHELL_CONFIG"
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$SHELL_CONFIG"
        echo "   Added to $SHELL_CONFIG"
        echo "   Run: source $SHELL_CONFIG"
        echo "   Or restart your terminal"
    fi
fi

echo ""
echo "✓ txplay installed successfully!"
echo ""
echo "To run txplay, type: txplay"
echo ""
if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
    echo "NOTE: Restart your terminal or run:"
    echo "  source ~/.bashrc"
fi
