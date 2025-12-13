"""Saved streams screen - manage and play online streams."""

from core.terminal_utils import clear_screen
from .base_screen import Screen

class StreamsMenuScreen(Screen):
    """Shows saved streaming URLs. Play, delete, or edit them."""
    
    def __init__(self, app):
        super().__init__(app)
        self.idx = 0
        # dummy data - will be replaced with real streams.json later
        self.streams = ["Lofi Girl (yt)", "Brown Noise 24/7", "Podcast XYZ"]

    def render(self):
        """Draw the streams list."""
        clear_screen()
        self.app.player_box.render()
        print()
        print(" Saved Streams")
        print("-" * 40)
        
        for i, s in enumerate(self.streams):
            if i == self.idx:
                # Inverted colors for selected item
                print(f"\033[7m {s}\033[0m")
            else:
                print(f" {s}")
        
        print("\n[Enter] Play   [Space] Play/Pause   [d] Delete   [e] Edit   [b] Back   [q] Quit")

    def handle_input(self, key):
        """Handle keypresses."""
        if key == "UP":
            self.idx = max(0, self.idx - 1)
            return self
        
        if key == "DOWN":
            self.idx = min(len(self.streams) - 1, self.idx + 1)
            return self
        
        if key == "ENTER" or key == "RIGHT":
            self.app.player_play(self.streams[self.idx])
            return self
        
        if key == "SPACE":
            if self.app.player.state == "playing":
                self.app.player_pause()
            else:
                self.app.player_resume_or_play(self.streams[self.idx])
            return self
        
        if key == "b" or key == "LEFT":
            from .home import HomeScreen
            return HomeScreen(self.app)
        
        if key == "q":
            self.app.quit()
            return None
        
        return self
