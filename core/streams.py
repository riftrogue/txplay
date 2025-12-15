import json
import os
from datetime import datetime
from constants import STREAMS_FILE


class StreamManager:
    def __init__(self):
        self.streams = []
        self.load_streams()

    def load_streams(self):
        """Load streams from JSON file"""
        if os.path.exists(STREAMS_FILE):
            try:
                with open(STREAMS_FILE, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                    self.streams = data.get('streams', [])
            except Exception:
                self.streams = []
        else:
            self.streams = []

    def save_streams(self):
        """Save streams to JSON file"""
        try:
            with open(STREAMS_FILE, 'w', encoding='utf-8') as f:
                json.dump({'streams': self.streams}, f, indent=2, ensure_ascii=False)
        except Exception:
            pass

    def add_stream(self, title, url, source="manual"):
        """Add a new stream"""
        # Generate new ID (max + 1, or 1 if empty)
        if self.streams:
            new_id = max(s['id'] for s in self.streams) + 1
        else:
            new_id = 1

        stream = {
            'id': new_id,
            'title': title,
            'url': url,
            'source': source,
            'added': datetime.now().isoformat()
        }
        self.streams.append(stream)
        self.save_streams()
        return new_id

    def get_all_streams(self):
        """Get all streams"""
        return self.streams

    def get_stream(self, stream_id):
        """Get a specific stream by ID"""
        for stream in self.streams:
            if stream['id'] == stream_id:
                return stream
        return None

    def delete_stream(self, stream_id):
        """Delete a stream by ID"""
        self.streams = [s for s in self.streams if s['id'] != stream_id]
        self.save_streams()
