"""Add stream screen - add new streaming URLs."""

import os
from .base_screen import Screen

class AddStreamScreen(Screen):
    """Screen for adding new streaming links. Placeholder for now."""
    
    def render(self):
        """Draw the add stream screen."""
        os.system("clear")
        self.app.player_box.render()
        print()
        print(" Add Stream")
        print("-" * 40)
        print(" (placeholder) - enter URL support comes later")
        print("\n[b] Back   [q] Quit")

    def handle_input(self, key):
        """Handle keypresses."""
        if key == "b" or key == "LEFT":
            from .home import HomeScreen
            return HomeScreen(self.app)
        
        if key == "q":
            self.app.quit()
            return None
        
        return self
