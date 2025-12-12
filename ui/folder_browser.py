"""Interactive folder browser for custom scan path selection."""

import os
from core.terminal_utils import clear_screen
from .base_screen import Screen


class FolderBrowserScreen(Screen):
    """Browse and select a folder for custom music scanning."""
    
    def __init__(self, app, start_path=None):
        super().__init__(app)
        self.current_path = start_path or os.path.expanduser("~")
        self.folders = []
        self.idx = 0
        self._load_folders()
    
    def _load_folders(self):
        """Load subdirectories in current path."""
        self.folders = []
        
        try:
            items = os.listdir(self.current_path)
            for item in sorted(items):
                full_path = os.path.join(self.current_path, item)
                # Only show directories, skip hidden folders
                if os.path.isdir(full_path) and not item.startswith('.'):
                    self.folders.append(item)
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
        
        if not self.folders:
            print("\n (No subdirectories found)")
        else:
            for i, folder in enumerate(self.folders):
                prefix = ">" if i == self.idx else " "
                print(f"{prefix} 📁 {folder}/")
        
        print("\n[→] Open folder   [Enter] Select this path and scan")
        print("[←/b] Go back (cd ..)")
        print("[↑/↓] Navigate folders")
        print("[q] Cancel")
    
    def handle_input(self, key):
        """Handle keypresses."""
        # Navigate list up
        if key == "UP":
            if self.folders:
                self.idx = max(0, self.idx - 1)
            return self
        
        # Navigate list down
        if key == "DOWN":
            if self.folders:
                self.idx = min(len(self.folders) - 1, self.idx + 1)
            return self
        
        # Enter folder (RIGHT arrow only)
        if key == "RIGHT" and self.folders:
            selected = self.folders[self.idx]
            new_path = os.path.join(self.current_path, selected)
            
            # Block root directory
            if new_path == "/" or new_path == "":
                return self  # Don't allow entering root
            
            self.current_path = new_path
            self._load_folders()
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
            self._load_folders()
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
