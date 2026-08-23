# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Reads the secrets `.env` file.
"""

from __future__ import annotations

from pathlib import Path

import gen_paths

_CHROMIUM_SRC_DIR = gen_paths.BOTS_DIR.parents[2]
DEFAULT_PATH = _CHROMIUM_SRC_DIR / 'brave' / '.env'


def parse(text: str) -> dict[str, str]:
    """Returns a dictionary of `KEY=VALUE` pairs parsed from the given text."""
    result: dict[str, str] = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        key, sep, value = line.partition('=')
        if not sep:
            continue
        key = key.strip()
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in ('"',
                                                                      "'"):
            value = value[1:-1]
        result[key] = value
    return result


def read(path: Path | None = None) -> dict[str, str]:
    """Reads and parses the `.env` file.

    Returns a dictionary of `KEY=VALUE` pairs with the contents of the file or
    {} if no file exists.
    """
    path = path if path is not None else DEFAULT_PATH
    if not path.is_file():
        return {}
    return parse(path.read_bytes().decode('utf-8'))
