# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`file` module: basic filesystem operations (read, write, copy, move,
remove, ...) as recipe steps.

File content travels in and out of the step through placeholders (see
`api.py`).
"""

DEPS = ['depot_tools', 'json', 'path', 'proto', 'raw_io', 'step']
