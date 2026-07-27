# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`file` module: basic filesystem operations (read, copy, move, remove, ...)
as recipe steps.

Covers every operation that doesn't need to inject arbitrary file content
into the step (see `api.py` for what that excludes and why).
"""

DEPS = ['depot_tools', 'step']
