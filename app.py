#!/usr/bin/env python3
"""
txplay - Terminal music player
Main application loop and keyboard input handling.
"""

import sys
import tty
import termios
import traceback
import os

from ui.home import HomeScreen
from ui.player_status_box import PlayerStatusBox
from core.player import MPVPlayer
from core.queue import QueueManager
from core.terminal_utils import hide_cursor, show_cursor

# Debug mode - set to True to enable crash logging
DEBUG_MODE = os.environ.get('TXPLAY_DEBUG', '').lower() in ('1', 'true', 'yes')
DEBUG_LOG = os.path.expanduser('~/.txplay_debug.log')


def log_debug(message):
    """Log debug message to file if DEBUG_MODE is enabled."""
    if DEBUG_MODE:
        try:
            with open(DEBUG_LOG, 'a') as f:
                from datetime import datetime
                timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                f.write(f"[{timestamp}] {message}\n")
        except:
            pass


def log_error(error_msg, exc_info=None):
    """Log error to debug file."""
    try:
        with open(DEBUG_LOG, 'a') as f:
            from datetime import datetime
            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            f.write(f"\n{'='*60}\n")
            f.write(f"[ERROR] {timestamp}\n")
            f.write(f"{error_msg}\n")
            if exc_info:
                f.write(f"\nTraceback:\n")
                traceback.print_exc(file=f)
            f.write(f"{'='*60}\n\n")
    except:
        pass


def get_key():
    """Read a single keypress and return a token string."""
    fd = sys.stdin.fileno()
    old = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        ch1 = sys.stdin.read(1)
        
        # Handle arrow keys (escape sequences)
        if ch1 == "\x1b":
            ch2 = sys.stdin.read(1)
            if ch2 == "[":
                ch3 = sys.stdin.read(1)
                if ch3 == "A":
                    return "UP"
                if ch3 == "B":
                    return "DOWN"
                if ch3 == "C":
                    return "RIGHT"
                if ch3 == "D":
                    return "LEFT"
                # Page Up/Down
                if ch3 == "5":
                    sys.stdin.read(1)  # consume ~
                    return "\x1b[5~"
                if ch3 == "6":
                    sys.stdin.read(1)  # consume ~
                    return "\x1b[6~"
            return "ESC"
        
        # Handle Enter
        if ch1 == "\r" or ch1 == "\n":
            return "ENTER"
        
        # Handle Space
        if ch1 == " ":
            return "SPACE"
        
        # Return single character (like 'q', 'b', etc.)
        return ch1
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old)


class App:
    """Main application class. Manages screens and player."""
    
    def __init__(self):
        self.player = MPVPlayer()
        self.queue = QueueManager()
        self.player_box = PlayerStatusBox()
        self.current_screen = HomeScreen(self)
        self.running = True
        
        # Set up track-end callback to auto-advance queue
        self.player.on_track_end = self._on_track_end
    
    def _on_track_end(self):
        """Called when current track ends - auto-play next from queue."""
        next_item = self.queue.next()
        if next_item:
            # Play next item from queue
            target = next_item.get('path') or next_item.get('url')
            self.player.play(target)
            self.player_box.set_playing(track=target, state=self.player.state, queue_count=self.queue.get_count())

    def player_play(self, target):
        """Start playing a file or URL."""
        self.player.play(target)
        self.player_box.set_playing(track=target, state=self.player.state, queue_count=self.queue.get_count())

    def player_pause(self):
        """Pause playback."""
        self.player.pause()
        self.player_box.set_playing(track=self.player.current, state=self.player.state, queue_count=self.queue.get_count())

    def player_resume_or_play(self, target):
        """Resume if paused, otherwise start playing."""
        if self.player.current == target and self.player.state == "paused":
            self.player.resume()
        else:
            self.player.play(target)
        self.player_box.set_playing(track=self.player.current, state=self.player.state, queue_count=self.queue.get_count())
    
    def player_stop(self):
        """Stop playback."""
        self.player.stop()
        self.player_box.set_idle(song_count=0, queue_count=self.queue.get_count())
    
    def player_seek(self, seconds):
        """Seek forward/backward by seconds."""
        self.player.seek(seconds)
    
    def queue_add(self, item_type, path_or_url, title):
        """Add item to queue."""
        if item_type == "local":
            self.queue.add_local(path_or_url, title)
        elif item_type == "youtube":
            self.queue.add_youtube(path_or_url, title)
        elif item_type == "stream":
            self.queue.add_stream(path_or_url, title)
    
    def queue_play_next(self):
        """Skip to next item in queue."""
        next_item = self.queue.next()
        if next_item:
            target = next_item.get('path') or next_item.get('url')
            self.player.play(target)
            self.player_box.set_playing(track=target, state=self.player.state, queue_count=self.queue.get_count())

    def quit(self):
        """Quit the application. Clean up player if needed."""
        self.running = False
        show_cursor()  # Restore cursor visibility
        self.player.quit()  # Terminate MPV process
        print("\nExiting txplay...")

    def run(self):
        """Main loop: render screen, get key, handle input, repeat."""
        log_debug("Starting application")
        hide_cursor()  # Hide cursor for cleaner UI
        
        try:
            while self.running:
                try:
                    # Draw current screen
                    log_debug(f"Rendering screen: {self.current_screen.__class__.__name__}")
                    self.current_screen.render()
                    
                    # Get single keypress
                    key = get_key()
                    log_debug(f"Key pressed: {key}")
                    
                    # Global quit rule: q or ESC = instant exit from anywhere
                    if key == "q" or key == "ESC":
                        log_debug("Quit requested")
                        self.quit()
                        break
                    
                    # Let current screen handle the key and possibly switch screens
                    next_screen = self.current_screen.handle_input(key)
                    if next_screen is None:
                        log_debug("Screen returned None, exiting")
                        break
                    self.current_screen = next_screen
                except Exception as e:
                    # Log the error but continue running
                    log_error(f"Error in main loop: {e}", exc_info=True)
                    # Show error to user
                    show_cursor()
                    print(f"\n\nError occurred: {e}")
                    print(f"Debug log: {DEBUG_LOG}")
                    print("\nPress Enter to continue or Ctrl+C to quit...")
                    try:
                        input()
                        hide_cursor()
                    except KeyboardInterrupt:
                        break
        finally:
            log_debug("Application shutting down")
            show_cursor()  # Always restore cursor on exit


def main():
    """Entry point."""
    try:
        log_debug("="*60)
        log_debug("txplay starting")
        log_debug(f"Debug mode: {DEBUG_MODE}")
        log_debug(f"Debug log: {DEBUG_LOG}")
        
        app = App()
        app.run()
    except KeyboardInterrupt:
        log_debug("KeyboardInterrupt received")
        try:
            app.quit()
        except:
            pass
    except Exception as e:
        # Fatal error - log and show to user
        log_error(f"Fatal error: {e}", exc_info=True)
        show_cursor()
        print(f"\n\n{'='*60}")
        print(f"FATAL ERROR")
        print(f"{'='*60}")
        print(f"\n{e}\n")
        print(f"Debug log saved to: {DEBUG_LOG}")
        print(f"\nTo enable verbose logging, run:")
        print(f"  TXPLAY_DEBUG=1 txplay")
        print(f"{'='*60}\n")
        sys.exit(1)


if __name__ == "__main__":
    main()
