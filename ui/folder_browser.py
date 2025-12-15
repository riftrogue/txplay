"""Interactive folder browser for custom scan path selection."""

import os
from core.terminal_utils import clear_screen
from .base_screen import Screen
from constants import AUDIO_EXTENSIONS


class FolderBrowserScreen(Screen):
    """Browse and select a folder for custom music scanning."""
    
    def __init__(self, app, start_path=None):
        super().__init__(app)
        self.current_path = start_path or os.path.expanduser("~")
        self.items = []  # Combined folders and files
        self.idx = 0
        self._load_items()
    
    def _load_items(self):
        """Load subdirectories and audio files in current path."""
        self.items = []
        
        try:
            entries = os.listdir(self.current_path)
            folders = []
            files = []
            
            for entry in sorted(entries, key=str.lower):
                full_path = os.path.join(self.current_path, entry)
                
                # Skip hidden items
                if entry.startswith('.'):
                    continue
                
                # Add directories
                if os.path.isdir(full_path):
                    folders.append({'name': entry, 'type': 'folder'})
                # Add audio files
                elif os.path.isfile(full_path):
                    _, ext = os.path.splitext(entry)
                    if ext.lower() in AUDIO_EXTENSIONS:
                        files.append({'name': entry, 'type': 'file'})
            
            # Folders first, then files
            self.items = folders + files
            
        except (PermissionError, OSError):
            pass  # Can't read this directory
        
        self.idx = 0  # Reset selection
    
    def render(self):
        """Draw the folder browser."""
        clear_screen()
        self.app.player_box.render()
        print()
        print(" Custom Folder Browser")
        print("-" * 50)
        print(f" Current: {self.current_path}")
        print("-" * 50)
        
        if not self.items:
            print("\n (No folders or audio files found)")
        else:
            for i, item in enumerate(self.items):
                prefix = "📁" if item['type'] == 'folder' else "🎵"
                suffix = "/" if item['type'] == 'folder' else ""
                
                if i == self.idx:
                    # Inverted colors for selected item
                    print(f"\033[7m {prefix} {item['name']}{suffix}\033[0m")
                else:
                    print(f" {prefix} {item['name']}{suffix}")
        
        print("\n[→] Open folder   [Enter] Select this path and scan")
        print("[←/b] Go back (cd ..)")
        print("[↑/↓] Navigate")
        print("[q] Cancel")
    
    def handle_input(self, key):
        """Handle keypresses."""
        # Navigate list up
        if key == "UP":
            if self.items:
                self.idx = max(0, self.idx - 1)
            return self
        
        # Navigate list down
        if key == "DOWN":
            if self.items:
                self.idx = min(len(self.items) - 1, self.idx + 1)
            return self
        
        # Enter folder (RIGHT arrow only) - only works on folders
        if key == "RIGHT" and self.items:
            selected = self.items[self.idx]
            
            # Only enter if it's a folder
            if selected['type'] == 'folder':
                new_path = os.path.join(self.current_path, selected['name'])
                
                # Block root directory
                if new_path == "/" or new_path == "":
                    return self  # Don't allow entering root
                
                self.current_path = new_path
                self._load_items()
            return self
        
        # Select current path and scan (ENTER key)
        if key == "ENTER":
            return self._select_current_path()
        
        # Go back
        if key == "LEFT" or key == "b":
            parent = os.path.dirname(self.current_path)
            
            # Block going above home or to root
            home = os.path.expanduser("~")
            if parent == "/" or len(parent) < len(home):
                return self  # Don't go above home
            
            self.current_path = parent
            self._load_items()
            return self
        
        if key == "q":
            # Cancel and go back
            from .scan_options import ScanOptionsScreen
            return ScanOptionsScreen(self.app)
        
        return self
    
    def _select_current_path(self):
        """Select current path and start scanning."""
        # Block root
        if self.current_path == "/":
            return self  # Don't allow selecting root
        
        # Save custom path to config
        from core.config import load_config, save_config
        config = load_config()
        config['scan_mode'] = 'custom'
        config['custom_scan_path'] = self.current_path
        save_config(config)
        
        # Start scanning
        from core.scanner import Scanner
        from constants import CUSTOM_CACHE
        
        def progress_callback(path, count):
            # Update status box during scan
            self.app.player_box.set_scanning(path, count)
        
        scanner = Scanner(status_callback=progress_callback)
        files = scanner.scan(self.current_path, CUSTOM_CACHE)
        
        # Update status to idle with song count
        self.app.player_box.set_idle(len(files))
        
        # Return to scan options
        from .scan_options import ScanOptionsScreen
        return ScanOptionsScreen(self.app)
