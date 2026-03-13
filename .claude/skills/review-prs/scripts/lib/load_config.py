"""Shared config helper for brave-dev-bot Python scripts.

Usage:
    from lib.load_config import load_config, get_config, require_config

    config = load_config()               # auto-discovers config.json
    org = require_config(config, "project.org")          # errors if missing
    repo = get_config(config, "project.prRepository")    # returns None if missing
"""

import json
import os
import sys


def load_config(config_path=None):
    """Load config.json, falling back to config.example.json.

    If config_path is None, searches relative to the bot repo root
    (derived from this file's location: lib/ -> scripts/ -> repo root).
    """
    if config_path and os.path.exists(config_path):
        with open(config_path) as f:
            return json.load(f)

    bot_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    for name in ("config.json", "config.example.json"):
        path = os.path.join(bot_dir, name)
        if os.path.exists(path):
            with open(path) as f:
                return json.load(f)

    return {}


def get_config(config, dotted_key, default=None):
    """Read a dotted key from the config dict (e.g. 'project.org')."""
    keys = dotted_key.split(".")
    value = config
    for key in keys:
        if isinstance(value, dict):
            value = value.get(key)
        else:
            return default
        if value is None:
            return default
    return value


def require_config(config, dotted_key):
    """Read a dotted key from the config dict, exit with error if missing or empty."""
    value = get_config(config, dotted_key)
    if not value:
        print(f"Error: '{dotted_key}' not set in config.json. Run 'make setup'.", file=sys.stderr)
        sys.exit(1)
    return value
