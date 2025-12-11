"""Player controller - handles mpv playback (dummy for now)."""

class DummyPlayer:
    """Dummy player. Will be replaced with real mpv controller later."""
    
    def __init__(self):
        self.state = "stopped"
        self.current = None

    def play(self, target):
        """Start playing a file or URL."""
        self.current = target
        self.state = "playing"
        print(f"\n[PLAYER] Playing: {target}")

    def pause(self):
        """Pause playback."""
        if self.state == "playing":
            self.state = "paused"
            print("\n[PLAYER] Paused")
    
    def resume(self):
        """Resume playback."""
        if self.state in ("paused", "stopped"):
            self.state = "playing"
            print("\n[PLAYER] Resumed")
    
    def stop(self):
        """Stop playback."""
        self.state = "stopped"
        print("\n[PLAYER] Stopped")
