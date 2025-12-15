"""Universal queue manager for local files and online streams."""

import json
import os
from constants import DATA_DIR


QUEUE_FILE = os.path.join(DATA_DIR, "queue.json")


class QueueManager:
    """Manages playback queue for both local files and online streams."""
    
    def __init__(self):
        self.items = []
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
        """Get and remove next item from queue (FIFO).
        
        Song is removed as soon as this is called (when it starts playing).
        
        Returns:
            Next queue item dict or None if queue is empty
        """
        if not self.items:
            return None
        
        # Pop first item (FIFO - first in, first out)
        item = self.items.pop(0)
        self.save()
        return item
    
    def peek_next(self):
        """Peek at next item without removing it.
        
        Returns:
            Next queue item dict or None
        """
        if not self.items:
            return None
        
        return self.items[0]
    
    def remove(self, index):
        """Remove item at index from queue."""
        if 0 <= index < len(self.items):
            self.items.pop(index)
            self.save()
    
    def clear(self):
        """Clear entire queue."""
        self.items = []
        self.save()
    
    def get_all(self):
        """Get all items in queue."""
        return self.items.copy()
    
    def get_count(self):
        """Get number of items in queue."""
        return len(self.items)
    
    def get_current(self):
        """Get next item in queue without removing it.
        
        Same as peek_next() - provided for compatibility.
        """
        return self.peek_next()
    
    def save(self):
        """Save queue to JSON file."""
        try:
            with open(QUEUE_FILE, 'w') as f:
                json.dump({'items': self.items}, f, indent=2)
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
        except (json.JSONDecodeError, IOError):
            pass  # Start with empty queue
