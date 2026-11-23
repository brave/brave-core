# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The paths bots reads and writes.

Read as `gen_paths.<NAME>` when used, never copied at import time, so that
tests can point them elsewhere.
"""

from __future__ import annotations

from pathlib import Path

BOTS_DIR = Path(__file__).resolve().parent
CONFIG_DIR = BOTS_DIR.parent / 'config'
BUILDERS_OUTPUT_DIR = CONFIG_DIR / 'generated' / 'builders'

# Chromium's `src/`, the root of `//` paths and of build directories.
CHROMIUM_SRC_DIR = BOTS_DIR.parents[2]
