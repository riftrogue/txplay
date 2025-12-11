"""Persistent player status box shown at top of every screen."""

class PlayerStatusBox:
    """Shows current track and playback state."""
    
    def __init__(self):
        self.track = None
        self.state = "stopped"  # playing / paused / stopped

    def set(self, track, state):
        """Update what's currently playing."""
        self.track = track
        self.state = state

    def render(self):
        """Draw the status box."""
        title = f" Playing: {self.track or 'none'} "
        state = f" State: {self.state} "
        width = max(40, len(title) + 4, len(state) + 4)
        
        top = "┌" + "─" * (width - 2) + "┐"
        bot = "└" + "─" * (width - 2) + "┘"
        middle1 = "│" + title.ljust(width - 2) + "│"
        middle2 = "│" + state.ljust(width - 2) + "│"
        
        print(top)
        print(middle1)
        print(middle2)
        print(bot)
