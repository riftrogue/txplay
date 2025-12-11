"""Local music screen - shows and plays local audio files."""

import os
from .base_screen import Screen
from core.config import load_config
from core.scanner import Scanner
from constants import PHONE_CACHE, TERMUX_CACHE, CUSTOM_CACHE


class LocalMusicScreen(Screen):
    """Shows list of local music files. Navigate and play them."""

    def __init__(self, app):
        super().__init__(app)
        self.idx = 0
        self.songs = self._load_songs()
    
    def _load_songs(self):
        """Load songs from cache based on current scan mode."""
        config = load_config()
        mode = config.get('scan_mode', 'termux')
        
        # Determine which cache to use
        cache_map = {
            'phone': PHONE_CACHE,
            'termux': TERMUX_CACHE,
            'custom': CUSTOM_CACHE,
        }
        
        cache_file = cache_map.get(mode, TERMUX_CACHE)
        scanner = Scanner()
        files = scanner._load_cache(cache_file)
        
        return files if files else []

    def render(self):
        """Draw the music list."""
        os.system("clear")
        self.app.player_box.render()
        print()
        print(" Local Music")
        print("-" * 50)
        
        if not self.songs:
            print("\n No music files found.")
            print(" Try scanning in Scan Options.")
        else:
            for i, song_path in enumerate(self.songs):
                prefix = ">" if i == self.idx else " "
                # Show only filename, not full path
                filename = os.path.basename(song_path)
                print(f"{prefix} {filename}")
        
        print("\n[Enter/→] Play   [Space] Play/Pause")
        print("[←/b] Back   [q] Quit")

    def handle_input(self, key):
        """Handle keypresses."""
        # Skip navigation if no songs
        if not self.songs:
            if key == "b" or key == "LEFT":
                from .home import HomeScreen
                return HomeScreen(self.app)
            if key == "q":
                self.app.quit()
                return None
            return self
        
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
