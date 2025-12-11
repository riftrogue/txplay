"""Home screen - main menu of txplay."""

import os
from .base_screen import Screen

class HomeScreen(Screen):
    """Main menu with options: Local Music, Saved Streams, Add Stream, Scan Options."""
    
    OPTIONS = ["Local Music", "Saved Streams", "Add Stream", "Scan Options"]

    def __init__(self, app):
        super().__init__(app)
        self.idx = 0  # which option is selected

    def render(self):
        """Draw the home screen."""
        os.system("clear")
        self.app.player_box.render()
        print()
        print(" txplay 0.1")
        print("-" * 40)
        
        for i, opt in enumerate(self.OPTIONS):
            prefix = ">" if i == self.idx else " "
            print(f"{prefix} {opt}")
        
        print("\n[q] Quit")

    def handle_input(self, key):
        """Handle keypresses."""
        if key == "UP":
            self.idx = max(0, self.idx - 1)
            return self
        
        if key == "DOWN":
            self.idx = min(len(self.OPTIONS) - 1, self.idx + 1)
            return self
        
        if key == "ENTER" or key == "RIGHT":
            # Import here to avoid circular imports
            from .local_music import LocalMusicScreen
            from .streams_menu import StreamsMenuScreen
            from .add_stream import AddStreamScreen
            from .scan_options import ScanOptionsScreen
            
            sel = self.OPTIONS[self.idx]
            if sel == "Local Music":
                return LocalMusicScreen(self.app)
            if sel == "Saved Streams":
                return StreamsMenuScreen(self.app)
            if sel == "Add Stream":
                return AddStreamScreen(self.app)
            if sel == "Scan Options":
                return ScanOptionsScreen(self.app)
            return self
        
        if key == "q":
            self.app.quit()
            return None
        
        return self
