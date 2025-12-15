"""Add stream screen - add new streaming URLs."""

from core.terminal_utils import clear_screen, show_cursor, hide_cursor
from core.streams import StreamManager
from .base_screen import Screen

class AddStreamScreen(Screen):
    """Screen for adding new streaming links."""
    
    def __init__(self, app):
        super().__init__(app)
        self.stream_manager = StreamManager()
        self.waiting_for_input = False
        self.message = None
    
    def render(self):
        """Draw the add stream screen."""
        clear_screen()
        self.app.player_box.render()
        print()
        print(" Add Stream")
        print("-" * 40)
        
        if self.message:
            print(f"\n {self.message}")
            print("\n Press any key to continue...")
            return
        
        if not self.waiting_for_input:
            print("\n Press [Enter] to add a new stream")
            print(" Press [b] to go back")
            print("\n[b] Back   [q] Quit")

    def get_input(self, prompt):
        """Get text input from user."""
        show_cursor()
        try:
            print(f"\n {prompt}", end='', flush=True)
            value = input().strip()
            return value
        finally:
            hide_cursor()

    def handle_input(self, key):
        """Handle keypresses."""
        # If showing a message, any key returns to streams menu
        if self.message:
            from .streams_menu import StreamsMenuScreen
            return StreamsMenuScreen(self.app)
        
        if key == "b" or key == "LEFT":
            from .streams_menu import StreamsMenuScreen
            return StreamsMenuScreen(self.app)
        
        if key == "q":
            self.app.quit()
            return None
        
        if key == "ENTER" and not self.waiting_for_input:
            # Start input process
            self.waiting_for_input = True
            clear_screen()
            self.app.player_box.render()
            print()
            print(" Add Stream")
            print("-" * 40)
            print("\n Enter stream details (or press Ctrl+C to cancel)")
            
            try:
                # Get title
                title = self.get_input("Title:")
                if not title:
                    self.message = "Error: Title cannot be empty"
                    self.waiting_for_input = False
                    return self
                
                # Get URL
                url = self.get_input("URL:")
                if not url:
                    self.message = "Error: URL cannot be empty"
                    self.waiting_for_input = False
                    return self
                
                # Add stream
                stream_id = self.stream_manager.add_stream(title, url, source="manual")
                self.message = f"Stream added successfully! (ID: {stream_id})"
                self.waiting_for_input = False
                
            except KeyboardInterrupt:
                self.message = "Cancelled."
                self.waiting_for_input = False
            except Exception as e:
                self.message = f"Error: {e}"
                self.waiting_for_input = False
            
            return self
        
        return self
