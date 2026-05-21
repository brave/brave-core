# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Import this to add the parent tools/cr to sys.path."""
import sys
from pathlib import Path

_cr = str(Path(__file__).parent.parent)
if _cr not in sys.path:
    sys.path.insert(0, _cr)
