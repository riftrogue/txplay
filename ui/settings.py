"""Settings screen - manage caches and configuration."""

import os
from core.terminal_utils import clear_screen
from .base_screen import Screen
from constants import PHONE_CACHE, TERMUX_CACHE, CUSTOM_CACHE, DATA_DIR


class SettingsScreen(Screen):
    """Settings menu for cache management and configuration."""
    
    def __init__(self, app):
        super().__init__(app)
        self.idx = 0
        self.options = [
            "Clear Phone Storage Cache",
            "Clear Termux Home Cache",
            "Clear Custom Folder Cache",
            "Clear All Caches",
            "View Cache Statistics"
        ]

    def render(self):
        """Draw the settings screen."""
        clear_screen()
        self.app.player_box.render()
        print()
        print(" Settings")
        print("-" * 50)
        
        for i, opt in enumerate(self.options):
            if i == self.idx:
                # Inverted colors for selected item
                print(f"\033[7m {opt}\033[0m")
            else:
                print(f" {opt}")
        
        print("\n[Enter/→] Select   [←/b] Back   [q] Quit")

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
                self._clear_cache(PHONE_CACHE, "Phone Storage")
            elif selected == 1:
                self._clear_cache(TERMUX_CACHE, "Termux Home")
            elif selected == 2:
                self._clear_cache(CUSTOM_CACHE, "Custom Folder")
            elif selected == 3:
                self._clear_all_caches()
            elif selected == 4:
                return CacheStatsScreen(self.app)
            
            return self
        
        if key == "b" or key == "LEFT":
            from .home import HomeScreen
            return HomeScreen(self.app)
        
        if key == "q":
            self.app.quit()
            return None
        
        return self
    
    def _clear_cache(self, cache_file, name):
        """Clear a specific cache file."""
        if os.path.exists(cache_file):
            try:
                os.remove(cache_file)
                # Show temporary confirmation
                clear_screen()
                self.app.player_box.render()
                print()
                print(f" ✓ {name} cache cleared successfully!")
                print("\n Press any key to continue...")
                import sys, tty, termios
                fd = sys.stdin.fileno()
                old = termios.tcgetattr(fd)
                try:
                    tty.setraw(fd)
                    sys.stdin.read(1)
                finally:
                    termios.tcsetattr(fd, termios.TCSADRAIN, old)
            except (OSError, IOError):
                pass
    
    def _clear_all_caches(self):
        """Clear all cache files."""
        caches = [
            (PHONE_CACHE, "Phone Storage"),
            (TERMUX_CACHE, "Termux Home"),
            (CUSTOM_CACHE, "Custom Folder")
        ]
        
        cleared = 0
        for cache_file, _ in caches:
            if os.path.exists(cache_file):
                try:
                    os.remove(cache_file)
                    cleared += 1
                except (OSError, IOError):
                    pass
        
        # Show confirmation
        clear_screen()
        self.app.player_box.render()
        print()
        print(f" ✓ Cleared {cleared} cache file(s) successfully!")
        print("\n Press any key to continue...")
        import sys, tty, termios
        fd = sys.stdin.fileno()
        old = termios.tcgetattr(fd)
        try:
            tty.setraw(fd)
            sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old)


class CacheStatsScreen(Screen):
    """Display cache statistics."""
    
    def __init__(self, app):
        super().__init__(app)
        self.stats = self._load_stats()
    
    def _load_stats(self):
        """Load cache statistics."""
        import json
        
        caches = [
            (PHONE_CACHE, "Phone Storage"),
            (TERMUX_CACHE, "Termux Home"),
            (CUSTOM_CACHE, "Custom Folder")
        ]
        
        stats = []
        total_files = 0
        total_size = 0
        
        for cache_file, name in caches:
            if os.path.exists(cache_file):
                try:
                    # Get file count from cache
                    with open(cache_file, 'r') as f:
                        data = json.load(f)
                        file_count = data.get('count', 0)
                    
                    # Get cache file size
                    cache_size = os.path.getsize(cache_file)
                    
                    stats.append({
                        'name': name,
                        'files': file_count,
                        'size': cache_size
                    })
                    
                    total_files += file_count
                    total_size += cache_size
                except (json.JSONDecodeError, IOError, OSError):
                    stats.append({
                        'name': name,
                        'files': 0,
                        'size': 0
                    })
            else:
                stats.append({
                    'name': name,
                    'files': 0,
                    'size': 0
                })
        
        return {
            'caches': stats,
            'total_files': total_files,
            'total_size': total_size
        }
    
    def _format_size(self, size_bytes):
        """Format bytes to human-readable size."""
        if size_bytes < 1024:
            return f"{size_bytes} B"
        elif size_bytes < 1024 * 1024:
            return f"{size_bytes / 1024:.1f} KB"
        else:
            return f"{size_bytes / (1024 * 1024):.1f} MB"
    
    def render(self):
        """Draw the cache statistics screen."""
        clear_screen()
        self.app.player_box.render()
        print()
        print(" Cache Statistics")
        print("-" * 50)
        print()
        
        for cache in self.stats['caches']:
            name = cache['name']
            files = cache['files']
            size = self._format_size(cache['size'])
            print(f" {name}:")
            print(f"   Files: {files}")
            print(f"   Cache size: {size}")
            print()
        
        print("-" * 50)
        total_files = self.stats['total_files']
        total_size = self._format_size(self.stats['total_size'])
        print(f" Total: {total_files} files ({total_size})")
        print()
        print("[←/b] Back   [q] Quit")
    
    def handle_input(self, key):
        """Handle keypresses."""
        if key == "b" or key == "LEFT":
            return SettingsScreen(self.app)
        
        if key == "q":
            self.app.quit()
            return None
        
        return self
