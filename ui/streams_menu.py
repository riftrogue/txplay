"""Saved streams screen - manage and play online streams."""

from core.terminal_utils import clear_screen
from core.streams import StreamManager
from .base_screen import Screen

class StreamsMenuScreen(Screen):
    """Shows saved streaming URLs. Play, delete, or edit them."""
    
    def __init__(self, app):
        super().__init__(app)
        self.idx = 0
        self.stream_manager = StreamManager()
        self.refresh_streams()

    def refresh_streams(self):
        """Reload streams from manager."""
        self.stream_manager.load_streams()
        self.stream_data = self.stream_manager.get_all_streams()
        # Create display items: "Title (source)"
        self.streams = [f"{s['title']} ({s['source']})" for s in self.stream_data]
        # Reset index if out of bounds
        if self.idx >= len(self.streams):
            self.idx = max(0, len(self.streams) - 1)

    def render(self):
        """Draw the streams list."""
        clear_screen()
        self.app.player_box.render()
        print()
        print(" Saved Streams")
        print("-" * 40)
        
        if not self.streams:
            print(" No saved streams yet.")
            print("\n Press '+' to add a new stream")
            print("\n[b] Back   [q] Quit")
            return
        
        for i, s in enumerate(self.streams):
            if i == self.idx:
                # Inverted colors for selected item
                print(f"\033[7m {s}\033[0m")
            else:
                print(f" {s}")
        
        print("\n[Enter] Play   [Space] Play/Pause   [a] Add to Queue   [d] Delete")
        print("[+] Add Stream   [n] Next   [s] Stop   [b] Back   [q] Quit")

    def handle_input(self, key):
        """Handle keypresses."""
        if key == "UP":
            self.idx = max(0, self.idx - 1)
            return self
        
        if key == "DOWN":
            if self.streams:
                self.idx = min(len(self.streams) - 1, self.idx + 1)
            return self
        
        if key == "ENTER" or key == "RIGHT":
            if self.stream_data:
                stream = self.stream_data[self.idx]
                self.app.player_play(stream['url'])
            return self
        
        if key == "SPACE":
            if self.app.player.state == "playing":
                self.app.player_pause()
            elif self.stream_data:
                stream = self.stream_data[self.idx]
                self.app.player_resume_or_play(stream['url'])
            return self
        
        if key == "a":
            # Add stream to queue
            if self.stream_data:
                stream = self.stream_data[self.idx]
                self.app.queue_add("stream", stream['url'], stream['title'])
            return self
        
        if key == "d":
            # Delete selected stream
            if self.stream_data:
                stream = self.stream_data[self.idx]
                self.stream_manager.delete_stream(stream['id'])
                self.refresh_streams()
            return self
        
        if key == "+" or key == "=":
            # Go to add stream screen
            from .add_stream import AddStreamScreen
            return AddStreamScreen(self.app)
        
        if key == "n":
            # Play next in queue
            self.app.queue_play_next()
            return self
        
        if key == "s":
            # Stop playback
            self.app.player_stop()
            return self
        
        if key == "b" or key == "LEFT":
            from .home import HomeScreen
            return HomeScreen(self.app)
        
        if key == "q":
            self.app.quit()
            return None
        
        return self
