"""YouTube Music search screen - search and play songs from YouTube Music."""

from core.terminal_utils import clear_screen, show_cursor, hide_cursor
from .base_screen import Screen

try:
    from ytmusicapi import YTMusic
    YTMUSIC_AVAILABLE = True
except ImportError:
    YTMUSIC_AVAILABLE = False


class YTMusicSearchScreen(Screen):
    """Search YouTube Music and play results."""
    
    def __init__(self, app):
        super().__init__(app)
        self.idx = 0
        self.results = []
        self.search_query = ""
        self.waiting_for_input = False
        self.message = None
        self.ytmusic = None
        
        if YTMUSIC_AVAILABLE:
            try:
                # Initialize without authentication for search only
                self.ytmusic = YTMusic()
            except Exception as e:
                self.message = f"Error initializing YouTube Music: {e}"
        else:
            self.message = "ytmusicapi not installed. Install with: pip install ytmusicapi"
    
    def render(self):
        """Draw the search screen."""
        clear_screen()
        self.app.player_box.render()
        print()
        print(" YouTube Music Search")
        print("-" * 40)
        
        if self.message:
            print(f"\n {self.message}")
            print("\n Press any key to continue...")
            return
        
        if not self.waiting_for_input and not self.results:
            print("\n Press [Enter] to search YouTube Music")
            print(" Press [b] to go back")
            print("\n[b] Back   [q] Quit")
            return
        
        if self.results:
            print(f"\n Search: '{self.search_query}'")
            print()
            for i, result in enumerate(self.results):
                # Format: "Title - Artist"
                artists = ", ".join([a['name'] for a in result.get('artists', [])])
                display = f"{result.get('title', 'Unknown')} - {artists}"
                
                if i == self.idx:
                    # Inverted colors for selected item
                    print(f"\033[7m {display}\033[0m")
                else:
                    print(f" {display}")
            
            print("\n[Enter] Play   [Space] Play/Pause   [a] Add to Queue")
            print("[r] New Search   [n] Next   [s] Stop   [b] Back   [q] Quit")
    
    def get_input(self, prompt):
        """Get text input from user."""
        show_cursor()
        try:
            print(f"\n {prompt}", end='', flush=True)
            value = input().strip()
            return value
        finally:
            hide_cursor()
    
    def perform_search(self):
        """Search YouTube Music."""
        if not self.ytmusic:
            self.message = "YouTube Music not available"
            return
        
        clear_screen()
        self.app.player_box.render()
        print()
        print(" YouTube Music Search")
        print("-" * 40)
        print("\n Enter search query (or press Ctrl+C to cancel)")
        
        try:
            query = self.get_input("Search:")
            if not query:
                self.message = "Search query cannot be empty"
                self.waiting_for_input = False
                return
            
            # Perform search
            print("\n Searching...")
            search_results = self.ytmusic.search(query, filter="songs", limit=5)
            
            if not search_results:
                self.message = "No results found"
                self.waiting_for_input = False
                return
            
            self.search_query = query
            self.results = search_results
            self.idx = 0
            self.waiting_for_input = False
            
        except KeyboardInterrupt:
            self.message = "Search cancelled."
            self.waiting_for_input = False
        except Exception as e:
            self.message = f"Search error: {e}"
            self.waiting_for_input = False
    
    def get_youtube_url(self, result):
        """Construct YouTube URL from search result."""
        video_id = result.get('videoId')
        if video_id:
            return f"https://www.youtube.com/watch?v={video_id}"
        return None
    
    def handle_input(self, key):
        """Handle keypresses."""
        # If showing a message, any key clears it
        if self.message:
            self.message = None
            return self
        
        if key == "b" or key == "LEFT":
            from .home import HomeScreen
            return HomeScreen(self.app)
        
        if key == "q":
            self.app.quit()
            return None
        
        if key == "ENTER" and not self.waiting_for_input and not self.results:
            # Start search
            self.waiting_for_input = True
            self.perform_search()
            return self
        
        if self.results:
            # Navigation in results
            if key == "UP":
                self.idx = max(0, self.idx - 1)
                return self
            
            if key == "DOWN":
                self.idx = min(len(self.results) - 1, self.idx + 1)
                return self
            
            if key == "ENTER" or key == "RIGHT":
                # Play selected result
                result = self.results[self.idx]
                url = self.get_youtube_url(result)
                if url:
                    self.app.player_play(url)
                return self
            
            if key == "SPACE":
                # Play/Pause
                if self.app.player.state == "playing":
                    self.app.player_pause()
                else:
                    result = self.results[self.idx]
                    url = self.get_youtube_url(result)
                    if url:
                        self.app.player_resume_or_play(url)
                return self
            
            if key == "a":
                # Add to queue
                result = self.results[self.idx]
                url = self.get_youtube_url(result)
                if url:
                    artists = ", ".join([a['name'] for a in result.get('artists', [])])
                    title = f"{result.get('title', 'Unknown')} - {artists}"
                    self.app.queue_add("youtube", url, title)
                return self
            
            if key == "r":
                # New search
                self.results = []
                self.search_query = ""
                self.idx = 0
                return self
            
            if key == "n":
                # Play next in queue
                self.app.queue_play_next()
                return self
            
            if key == "s":
                # Stop playback
                self.app.player_stop()
                return self
        
        return self
