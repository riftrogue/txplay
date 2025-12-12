"""Persistent player status box shown at top of every screen."""

import os
import sys

class PlayerStatusBox:
    """Shows current track, playback state, or scanning progress."""
    
    def __init__(self):
        self.mode = "idle"  # idle, playing, scanning
        self.track = None
        self.state = "stopped"  # playing / paused / stopped
        self.scan_path = None
        self.scan_count = 0

    def set_playing(self, track, state):
        """Update playback status."""
        self.mode = "playing"
        self.track = track
        self.state = state

    def set_scanning(self, path, count):
        """Update scanning progress."""
        self.mode = "scanning"
        self.scan_path = path
        self.scan_count = count
    
    def set_idle(self, song_count=0):
        """Set to idle state."""
        self.mode = "idle"
        self.track = None
        self.scan_count = song_count

    def render(self):
        """Draw the status box."""
        # Force cursor to top-left before rendering
        sys.stdout.write("\033[H")
        
        if self.mode == "playing":
            # Show playback info
            line1 = f" Playing: {os.path.basename(self.track) if self.track else 'none'} "
            line2 = f" State: {self.state} "
        elif self.mode == "scanning":
            # Show scanning progress
            line1 = f" Scanning: {self.scan_path or '...'} "
            line2 = f" Found: {self.scan_count} songs "
        else:
            # Idle state
            line1 = " Status: Ready "
            line2 = f" Songs loaded: {self.scan_count} "
        
        width = max(50, len(line1) + 4, len(line2) + 4)
        
        top = "┌" + "─" * (width - 2) + "┐"
        bot = "└" + "─" * (width - 2) + "┘"
        middle1 = "│" + line1.ljust(width - 2) + "│"
        middle2 = "│" + line2.ljust(width - 2) + "│"
        
        print(top)
        print(middle1)
        print(middle2)
        print(bot)
        sys.stdout.flush()
