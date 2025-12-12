"""Terminal utility functions for clean rendering without flicker."""

import sys


def clear_screen():
    """Clear screen using ANSI escape codes (no flicker)."""
    # Clear screen, clear scrollback, move cursor to home
    sys.stdout.write("\033[2J\033[3J\033[H")
    sys.stdout.flush()


def move_cursor(row=1, col=1):
    """Move cursor to specific position."""
    sys.stdout.write(f"\033[{row};{col}H")
    sys.stdout.flush()


def hide_cursor():
    """Hide terminal cursor."""
    sys.stdout.write("\033[?25l")
    sys.stdout.flush()


def show_cursor():
    """Show terminal cursor."""
    sys.stdout.write("\033[?25h")
    sys.stdout.flush()


def get_terminal_size():
    """Get terminal dimensions."""
    try:
        import shutil
        columns, rows = shutil.get_terminal_size()
        return rows, columns
    except:
        return 24, 80  # fallback


class Paginator:
    """Handle pagination for large lists."""
    
    def __init__(self, items, page_size=None):
        """
        Initialize paginator.
        
        Args:
            items: List of items to paginate
            page_size: Items per page (default: 20)
        """
        self.items = items
        self.current_idx = 0
        
        # Default page size is 20 items
        if page_size is None:
            self.page_size = 20
        else:
            self.page_size = page_size
    
    @property
    def total_items(self):
        """Total number of items."""
        return len(self.items)
    
    @property
    def total_pages(self):
        """Total number of pages."""
        if not self.items:
            return 0
        return (len(self.items) - 1) // self.page_size + 1
    
    @property
    def current_page(self):
        """Current page number (1-indexed)."""
        if not self.items:
            return 0
        return (self.current_idx // self.page_size) + 1
    
    @property
    def visible_items(self):
        """Items visible on current page."""
        if not self.items:
            return []
        
        start = (self.current_page - 1) * self.page_size
        end = min(start + self.page_size, len(self.items))
        return self.items[start:end]
    
    @property
    def visible_range(self):
        """Range of visible items (1-indexed for display)."""
        if not self.items:
            return (0, 0)
        
        start = (self.current_page - 1) * self.page_size + 1
        end = min(start + self.page_size - 1, len(self.items))
        return (start, end)
    
    @property
    def local_idx(self):
        """Index within current page."""
        return self.current_idx % self.page_size
    
    def move_up(self):
        """Move selection up."""
        if self.current_idx > 0:
            self.current_idx -= 1
    
    def move_down(self):
        """Move selection down."""
        if self.current_idx < len(self.items) - 1:
            self.current_idx += 1
    
    def page_up(self):
        """Jump to previous page."""
        self.current_idx = max(0, self.current_idx - self.page_size)
    
    def page_down(self):
        """Jump to next page."""
        self.current_idx = min(len(self.items) - 1, self.current_idx + self.page_size)
    
    def get_selected(self):
        """Get currently selected item."""
        if not self.items:
            return None
        return self.items[self.current_idx]
    
    def get_page_info(self):
        """Get page info string for display."""
        if not self.items:
            return "No items"
        
        start, end = self.visible_range
        return f"Items {start}-{end} of {self.total_items} | Page {self.current_page}/{self.total_pages}"
