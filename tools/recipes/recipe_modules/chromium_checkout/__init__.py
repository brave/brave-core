# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`chromium_checkout` module: clone / sync / validate a Chromium src/ tree."""

DEPS = [
    'path', 'raw_io', 'json', 'step', 'context', 'depot_tools', 'env', 'git',
    'git_cache', 'platform'
]
