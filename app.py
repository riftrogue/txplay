#!/usr/bin/env python3
"""
txplay - Terminal music player
Main application loop and keyboard input handling.
"""

import sys
import tty
import termios

from ui.home import HomeScreen
from ui.player_status_box import PlayerStatusBox
from core.player import DummyPlayer


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
        self.player = DummyPlayer()
        self.player_box = PlayerStatusBox()
        self.current_screen = HomeScreen(self)
        self.running = True

    def player_play(self, target):
        """Start playing a file or URL."""
        self.player.play(target)
        self.player_box.set_playing(track=target, state=self.player.state)

    def player_pause(self):
        """Pause playback."""
        self.player.pause()
        self.player_box.set_playing(track=self.player.current, state=self.player.state)

    def player_resume_or_play(self, target):
        """Resume if paused, otherwise start playing."""
        if self.player.current == target and self.player.state == "paused":
            self.player.resume()
        else:
            self.player.play(target)
        self.player_box.set_playing(track=self.player.current, state=self.player.state)

    def quit(self):
        """Quit the application. Clean up player if needed."""
        self.running = False
        # TODO: terminate mpv subprocess when real player is implemented
        print("\nExiting txplay...")

    def run(self):
        """Main loop: render screen, get key, handle input, repeat."""
        while self.running:
            # Draw current screen
            self.current_screen.render()
            
            # Get single keypress
            key = get_key()
            
            # Global quit rule: q = instant exit from anywhere
            if key == "q":
                self.quit()
                break
            
            # Let current screen handle the key and possibly switch screens
            next_screen = self.current_screen.handle_input(key)
            if next_screen is None:
                break
            self.current_screen = next_screen


def main():
    """Entry point."""
    app = App()
    try:
        app.run()
    except KeyboardInterrupt:
        app.quit()


if __name__ == "__main__":
    main()
