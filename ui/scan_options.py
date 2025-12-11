"""Scan options screen - configure where to scan for music."""

import os
from .base_screen import Screen

class ScanOptionsScreen(Screen):
    """Configure music scanning options."""
    
    def __init__(self, app):
        super().__init__(app)
        self.idx = 0
        self.options = ["Full Storage (/sdcard)", "Termux Home", "Custom Folder", "Rescan Now"]

    def render(self):
        """Draw the scan options screen."""
        os.system("clear")
        self.app.player_box.render()
        print()
        print(" Scan Options")
        print("-" * 40)
        
        for i, o in enumerate(self.options):
            prefix = ">" if i == self.idx else " "
            print(f"{prefix} {o}")
        
        print("\n[Enter] Select   [b] Back   [q] Quit")

    def handle_input(self, key):
        """Handle keypresses."""
        if key == "UP":
            self.idx = max(0, self.idx - 1)
            return self
        
        if key == "DOWN":
            self.idx = min(len(self.options) - 1, self.idx + 1)
            return self
        
        if key == "ENTER" or key == "RIGHT":
            # placeholder: just print selection (later: change config)
            print(f"Selected: {self.options[self.idx]}")
            input("Press Enter to continue...")
            return self
        
        if key == "b" or key == "LEFT":
            from .home import HomeScreen
            return HomeScreen(self.app)
        
        if key == "q":
            self.app.quit()
            return None
        
        return self
