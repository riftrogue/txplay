"""Configuration management for txplay."""

import json
import os
from constants import CONFIG_FILE, HOME_PATH


DEFAULT_CONFIG = {
    "scan_mode": "termux",  # phone, termux, or custom
    "custom_scan_path": HOME_PATH,
}


def load_config():
    """Load configuration from file."""
    if not os.path.exists(CONFIG_FILE):
        return DEFAULT_CONFIG.copy()
    
    try:
        with open(CONFIG_FILE, 'r') as f:
            config = json.load(f)
            # Ensure all default keys exist
            for key, value in DEFAULT_CONFIG.items():
                if key not in config:
                    config[key] = value
            return config
    except (json.JSONDecodeError, IOError):
        return DEFAULT_CONFIG.copy()


def save_config(config):
    """Save configuration to file."""
    try:
        with open(CONFIG_FILE, 'w') as f:
            json.dump(config, f, indent=2)
    except IOError:
        pass  # Fail silently if can't write
