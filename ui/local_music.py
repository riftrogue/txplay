"""Local music screen - shows and plays local audio files."""

import os
from .base_screen import Screen

class LocalMusicScreen(Screen):
    """Shows list of local music files. Navigate and play them."""
    
    DUMMY_SONGS = ["Song A.mp3", "Song B.mp3", "Song C.mp3"]

    def __init__(self, app):
        super().__init__(app)
        self.idx = 0
        self.songs = list(self.DUMMY_SONGS)  # will be replaced with real scanner later

    def render(self):
        """Draw the music list."""
        os.system("clear")
        self.app.player_box.render()
        print()
        print(" Local Music")
        print("-" * 40)
        
        for i, s in enumerate(self.songs):
            prefix = ">" if i == self.idx else " "
            print(f"{prefix} {s}")
        
        print("\n[Enter] Play   [Space] Play/Pause   [b] Back   [q] Quit")

    def handle_input(self, key):
        """Handle keypresses."""
        if key == "UP":
            self.idx = max(0, self.idx - 1)
            return self
        
        if key == "DOWN":
            self.idx = min(len(self.songs) - 1, self.idx + 1)
            return self
        
        if key == "ENTER" or key == "RIGHT":
            selected = self.songs[self.idx]
            self.app.player_play(selected)
            return self
        
        if key == "SPACE":
            # toggle play/pause
            if self.app.player.state == "playing":
                self.app.player_pause()
            else:
                self.app.player_resume_or_play(self.songs[self.idx])
            return self
        
        if key == "b" or key == "LEFT":
            from .home import HomeScreen
            return HomeScreen(self.app)
        
        if key == "q":
            self.app.quit()
            return None
        
        return self
