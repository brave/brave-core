# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Import this to add tools/cr to sys.path."""

from pathlib import Path
import sys

_CR_DIR = str(Path(__file__).resolve().parents[2])
if _CR_DIR not in sys.path:
    sys.path.insert(0, _CR_DIR)
