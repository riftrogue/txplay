"""Persistent player status box shown at top of every screen."""

import os
import sys
from core.terminal_utils import get_terminal_size, truncate_filename

class PlayerStatusBox:
    """Shows current track, playback state, or scanning progress."""
    
    def __init__(self):
        self.mode = "idle"  # idle, playing, scanning
        self.track = None
        self.state = "stopped"  # playing / paused / stopped
        self.scan_path = None
        self.scan_count = 0
        self.queue_count = 0  # Number of items in queue

    def set_playing(self, track, state, queue_count=0):
        """Update playback status."""
        self.mode = "playing"
        self.track = track
        self.state = state
        self.queue_count = queue_count

    def set_scanning(self, path, count):
        """Update scanning progress."""
        self.mode = "scanning"
        self.scan_path = path
        self.scan_count = count
    
    def set_idle(self, song_count=0, queue_count=0):
        """Set to idle state."""
        self.mode = "idle"
        self.track = None
        self.scan_count = song_count
        self.queue_count = queue_count

    def render(self):
        """Draw the status box."""
        # Force cursor to top-left before rendering
        sys.stdout.write("\033[H")
        
        # Get terminal width and set box width (leave some margin)
        _, term_width = get_terminal_size()
        box_width = min(term_width - 4, 70)  # Max 70 chars or terminal width - 4
        content_width = box_width - 4  # Account for borders and padding
        
        if self.mode == "playing":
            # Show playback info with truncated filename
            filename = os.path.basename(self.track) if self.track else 'none'
            truncated = truncate_filename(filename, content_width - 10)  # Reserve space for "Playing: "
            line1 = f" Playing: {truncated} "
            
            # Show state and queue count
            state_text = f"State: {self.state}"
            if self.queue_count > 0:
                state_text += f" | Queue: {self.queue_count}"
            line2 = f" {state_text} "
        elif self.mode == "scanning":
            # Show scanning progress with truncated path
            scan_path = self.scan_path or '...'
            truncated = truncate_filename(scan_path, content_width - 11)  # Reserve space for "Scanning: "
            line1 = f" Scanning: {truncated} "
            line2 = f" Found: {self.scan_count} songs "
        else:
            # Idle state
            line1 = " Status: Ready "
            queue_text = f"Songs: {self.scan_count}"
            if self.queue_count > 0:
                queue_text += f" | Queue: {self.queue_count}"
            line2 = f" {queue_text} "
        
        # Use fixed width box
        top = "┌" + "─" * (box_width - 2) + "┐"
        bot = "└" + "─" * (box_width - 2) + "┘"
        middle1 = "│" + line1.ljust(box_width - 2) + "│"
        middle2 = "│" + line2.ljust(box_width - 2) + "│"
        
        print(top)
        print(middle1)
        print(middle2)
        print(bot)
        sys.stdout.flush()
