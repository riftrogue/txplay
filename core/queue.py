"""Universal queue manager for local files and online streams."""

import json
import os
from constants import DATA_DIR


QUEUE_FILE = os.path.join(DATA_DIR, "queue.json")


class QueueManager:
    """Manages playback queue for both local files and online streams."""
    
    def __init__(self):
        self.items = []
        self.current_index = -1
        self.load()
    
    def add(self, item_type, path_or_url, title, metadata=None):
        """Add item to queue.
        
        Args:
            item_type: "local", "youtube", or "stream"
            path_or_url: File path or URL
            title: Display title
            metadata: Optional metadata dict
        """
        item = {
            "type": item_type,
            "path" if item_type == "local" else "url": path_or_url,
            "title": title
        }
        
        if metadata:
            item["metadata"] = metadata
        
        self.items.append(item)
        self.save()
    
    def add_local(self, path, title=None):
        """Add local file to queue."""
        if title is None:
            title = os.path.basename(path)
        self.add("local", path, title)
    
    def add_youtube(self, url, title, metadata=None):
        """Add YouTube stream to queue."""
        self.add("youtube", url, title, metadata)
    
    def add_stream(self, url, title):
        """Add generic stream to queue."""
        self.add("stream", url, title)
    
    def next(self):
        """Get next item in queue.
        
        Returns:
            Next queue item dict or None if queue is empty
        """
        if not self.items:
            return None
        
        self.current_index += 1
        if self.current_index >= len(self.items):
            self.current_index = 0  # Loop back to start
        
        return self.items[self.current_index]
    
    def peek_next(self):
        """Peek at next item without advancing.
        
        Returns:
            Next queue item dict or None
        """
        if not self.items:
            return None
        
        next_idx = (self.current_index + 1) % len(self.items)
        return self.items[next_idx]
    
    def remove(self, index):
        """Remove item at index from queue."""
        if 0 <= index < len(self.items):
            self.items.pop(index)
            if self.current_index >= index:
                self.current_index -= 1
            self.save()
    
    def clear(self):
        """Clear entire queue."""
        self.items = []
        self.current_index = -1
        self.save()
    
    def get_all(self):
        """Get all items in queue."""
        return self.items.copy()
    
    def get_count(self):
        """Get number of items in queue."""
        return len(self.items)
    
    def get_current(self):
        """Get current item in queue."""
        if 0 <= self.current_index < len(self.items):
            return self.items[self.current_index]
        return None
    
    def save(self):
        """Save queue to JSON file."""
        try:
            with open(QUEUE_FILE, 'w') as f:
                json.dump({
                    'items': self.items,
                    'current_index': self.current_index
                }, f, indent=2)
        except IOError:
            pass  # Fail silently
    
    def load(self):
        """Load queue from JSON file."""
        if not os.path.exists(QUEUE_FILE):
            return
        
        try:
            with open(QUEUE_FILE, 'r') as f:
                data = json.load(f)
                self.items = data.get('items', [])
                self.current_index = data.get('current_index', -1)
        except (json.JSONDecodeError, IOError):
            pass  # Start with empty queue
