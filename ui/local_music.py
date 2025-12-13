"""Local music screen - shows and plays local audio files."""

import os
from .base_screen import Screen
from core.config import load_config
from core.scanner import Scanner
from core.terminal_utils import clear_screen, Paginator, get_terminal_size, truncate_filename
from constants import PHONE_CACHE, TERMUX_CACHE, CUSTOM_CACHE


class LocalMusicScreen(Screen):
    """Shows list of local music files. Navigate and play them."""

    def __init__(self, app):
        super().__init__(app)
        songs = self._load_songs()
        self.paginator = Paginator(songs)
    
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
        clear_screen()
        self.app.player_box.render()
        print()
        print(" Local Music")
        print("-" * 50)
        
        if not self.paginator.items:
            print("\n No music files found.")
            print(" Try scanning in Scan Options.")
        else:
            # Get terminal width for truncation
            _, term_width = get_terminal_size()
            max_filename_len = term_width - 5  # Leave margin for padding and borders
            
            # Show visible items on current page
            for i, song_path in enumerate(self.paginator.visible_items):
                is_selected = (i == self.paginator.local_idx)
                filename = os.path.basename(song_path)
                truncated = truncate_filename(filename, max_filename_len)
                if is_selected:
                    # Inverted colors for selected item
                    print(f"\033[7m {truncated}\033[0m")
                else:
                    print(f" {truncated}")
            
            # Show pagination info
            print()
            print(f" {self.paginator.get_page_info()}")
        
        print("\n[Enter/→] Play   [Space] Play/Pause   [PgUp/PgDn] Page")
        print("[←/b] Back   [q] Quit")

    def handle_input(self, key):
        """Handle keypresses."""
        # Skip navigation if no songs
        if not self.paginator.items:
            if key == "b" or key == "LEFT":
                from .home import HomeScreen
                return HomeScreen(self.app)
            if key == "q":
                self.app.quit()
                return None
            return self
        
        if key == "UP":
            self.paginator.move_up()
            return self
        
        if key == "DOWN":
            self.paginator.move_down()
            return self
        
        # Page navigation (handle escape sequences for PgUp/PgDn)
        if key == "\x1b[5~":  # Page Up
            self.paginator.page_up()
            return self
        
        if key == "\x1b[6~":  # Page Down
            self.paginator.page_down()
            return self
        
        if key == "ENTER" or key == "RIGHT":
            selected = self.paginator.get_selected()
            if selected:
                self.app.player_play(selected)
            return self
        
        if key == "SPACE":
            # toggle play/pause
            if self.app.player.state == "playing":
                self.app.player_pause()
            else:
                selected = self.paginator.get_selected()
                if selected:
                    self.app.player_resume_or_play(selected)
            return self
        
        if key == "b" or key == "LEFT":
            from .home import HomeScreen
            return HomeScreen(self.app)
        
        if key == "q":
            self.app.quit()
            return None
        
        return self
