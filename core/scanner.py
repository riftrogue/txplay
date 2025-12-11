"""Music file scanner with smart caching."""

import os
import json
from constants import AUDIO_EXTENSIONS


class Scanner:
    """Recursively scan directories for audio files."""
    
    def __init__(self, status_callback=None):
        """
        Initialize scanner.
        
        Args:
            status_callback: Function to call with progress updates (path, count)
        """
        self.status_callback = status_callback
        self.visited_paths = set()  # Track visited paths to avoid symlink loops
    
    def scan(self, paths, cache_file):
        """
        Smart scan: merge new files with existing cache.
        
        Args:
            paths: List of paths to scan (or single path string)
            cache_file: Path to cache JSON file
            
        Returns:
            Sorted list of audio file paths
        """
        # Ensure paths is a list
        if isinstance(paths, str):
            paths = [paths]
        
        # Load existing cache
        old_files = self._load_cache(cache_file)
        
        # Scan fresh
        new_files = []
        for path in paths:
            if os.path.exists(path) and os.path.isdir(path):
                new_files.extend(self._scan_directory(path))
        
        # Merge: combine old + new, remove duplicates
        all_files = list(set(old_files + new_files))
        
        # Remove files that no longer exist
        existing_files = [f for f in all_files if os.path.exists(f)]
        
        # Sort alphabetically by filename
        existing_files.sort(key=lambda f: os.path.basename(f).lower())
        
        # Save to cache
        self._save_cache(cache_file, existing_files)
        
        return existing_files
    
    def _scan_directory(self, path):
        """
        Recursively scan a directory for audio files.
        
        Args:
            path: Directory path to scan
            
        Returns:
            List of audio file paths
        """
        music_files = []
        
        # Avoid infinite loops with symlinks
        real_path = os.path.realpath(path)
        if real_path in self.visited_paths:
            return music_files
        self.visited_paths.add(real_path)
        
        try:
            items = os.listdir(path)
        except (PermissionError, OSError):
            # Skip inaccessible directories silently
            return music_files
        
        for item in items:
            full_path = os.path.join(path, item)
            
            # Update progress
            if self.status_callback:
                self.status_callback(full_path, len(music_files))
            
            # Check if directory or symlink to directory
            if os.path.isdir(full_path):
                # Recursively scan subdirectory
                music_files.extend(self._scan_directory(full_path))
            
            # Check if it's an audio file
            elif self._is_audio_file(full_path):
                music_files.append(full_path)
        
        return music_files
    
    def _is_audio_file(self, filepath):
        """Check if file has audio extension."""
        return filepath.lower().endswith(AUDIO_EXTENSIONS)
    
    def _load_cache(self, cache_file):
        """Load file list from cache JSON."""
        if not os.path.exists(cache_file):
            return []
        
        try:
            with open(cache_file, 'r') as f:
                data = json.load(f)
                return data.get('files', [])
        except (json.JSONDecodeError, IOError):
            return []
    
    def _save_cache(self, cache_file, files):
        """Save file list to cache JSON."""
        try:
            with open(cache_file, 'w') as f:
                json.dump({'files': files, 'count': len(files)}, f, indent=2)
        except IOError:
            pass  # Fail silently if can't write
