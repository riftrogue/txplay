"""Scan options screen - configure where to scan for music."""

from core.terminal_utils import clear_screen
from .base_screen import Screen
from core.scanner import Scanner
from core.config import load_config, save_config
from constants import (
    PHONE_CACHE, TERMUX_CACHE, CUSTOM_CACHE,
    PHONE_MUSIC_PATH, PHONE_DOWNLOAD_PATH, HOME_PATH
)


class ScanOptionsScreen(Screen):
    """Configure music scanning options."""
    
    def __init__(self, app):
        super().__init__(app)
        self.idx = 0
        self.options = [
            "Phone Storage (Music + Downloads)",
            "Termux Home (Full ~/)",
            "Custom Folder"
        ]
        
        # Load current mode
        config = load_config()
        self.current_mode = config.get('scan_mode', 'termux')

    def render(self):
        """Draw the scan options screen."""
        clear_screen()
        self.app.player_box.render()
        print()
        print(" Scan Options")
        print("-" * 50)
        
        for i, opt in enumerate(self.options):
            # Show current mode indicator
            mode_name = ["phone", "termux", "custom"][i]
            indicator = " ●" if self.current_mode == mode_name else ""
            
            if i == self.idx:
                # Inverted colors for selected item
                print(f"\033[7m {opt}{indicator}\033[0m")
            else:
                print(f" {opt}{indicator}")
        
        print("\n[Enter/→] Select and Rescan")
        print("[←/b] Back   [q] Quit")

    def handle_input(self, key):
        """Handle keypresses."""
        if key == "UP":
            self.idx = max(0, self.idx - 1)
            return self
        
        if key == "DOWN":
            self.idx = min(len(self.options) - 1, self.idx + 1)
            return self
        
        if key == "ENTER" or key == "RIGHT":
            selected = self.idx
            
            if selected == 0:
                # Phone Storage scan
                return self._scan_phone()
            elif selected == 1:
                # Termux Home scan
                return self._scan_termux()
            elif selected == 2:
                # Custom Folder - open browser
                from .folder_browser import FolderBrowserScreen
                return FolderBrowserScreen(self.app)
            
            return self
        
        if key == "b" or key == "LEFT":
            from .home import HomeScreen
            return HomeScreen(self.app)
        
        if key == "q":
            self.app.quit()
            return None
        
        return self
    
    def _scan_phone(self):
        """Scan phone storage (Music + Downloads)."""
        config = load_config()
        config['scan_mode'] = 'phone'
        save_config(config)
        self.current_mode = 'phone'
        
        def progress_callback(path, count):
            self.app.player_box.set_scanning(path, count)
        
        scanner = Scanner(status_callback=progress_callback)
        paths = [PHONE_MUSIC_PATH, PHONE_DOWNLOAD_PATH]
        files = scanner.scan(paths, PHONE_CACHE)
        
        self.app.player_box.set_idle(len(files))
        return self
    
    def _scan_termux(self):
        """Scan entire Termux home directory."""
        config = load_config()
        config['scan_mode'] = 'termux'
        save_config(config)
        self.current_mode = 'termux'
        
        def progress_callback(path, count):
            self.app.player_box.set_scanning(path, count)
        
        # Enable phone storage exclusion for Termux scan
        scanner = Scanner(status_callback=progress_callback, exclude_phone_storage=True)
        files = scanner.scan(HOME_PATH, TERMUX_CACHE)
        
        self.app.player_box.set_idle(len(files))
        return self
