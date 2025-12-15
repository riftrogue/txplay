#!/usr/bin/env bash
# txplay installation script
# Supports both fresh install and updates

set -e

INSTALL_DIR="$HOME/.txplay"
BIN_DIR="$HOME/.local/bin"
REPO_URL="https://github.com/riftrogue/txplay.git"
BRANCH="main"

echo "==> Installing txplay..."

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
if command -v pip3 &> /dev/null; then
    pip3 install --user -r requirements.txt
else
    python3 -m pip install --user -r requirements.txt
fi

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
