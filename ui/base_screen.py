"""Base screen class that all other screens inherit from."""

class Screen:
    """Base screen. render() prints content. handle_input() returns next screen or self."""
    
    def __init__(self, app):
        self.app = app

    def render(self):
        """Override this to show your screen content."""
        pass

    def handle_input(self, key):
        """Override this to handle keypresses. Return next screen or self."""
        # Global quit rule: q = instant exit from anywhere
        if key == "q":
            self.app.quit()
            return None
        return self
