"""Global constants for txplay."""

import os

# Audio file extensions
AUDIO_EXTENSIONS = (
    '.mp3',
    '.flac',
    '.m4a',
    '.wav',
    '.ogg',
    '.opus',
    '.aac',
    '.wma',
    '.alac',
)

# Default scan paths
HOME_PATH = os.path.expanduser("~")
PHONE_MUSIC_PATH = "/sdcard/Music"
PHONE_DOWNLOAD_PATH = "/sdcard/Download"

# Cache files
DATA_DIR = os.path.join(os.path.dirname(__file__), "data")
PHONE_CACHE = os.path.join(DATA_DIR, "phone_music_cache.json")
TERMUX_CACHE = os.path.join(DATA_DIR, "termux_music_cache.json")
CUSTOM_CACHE = os.path.join(DATA_DIR, "custom_music_cache.json")
CONFIG_FILE = os.path.join(DATA_DIR, "config.json")
STREAMS_FILE = os.path.join(DATA_DIR, "streams.json")

# Ensure data directory exists
os.makedirs(DATA_DIR, exist_ok=True)
