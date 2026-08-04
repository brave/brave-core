# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`osx_sdk` module: install/select the Xcode pinned by a Chromium checkout's
macOS SDK.
"""

DEPS = [
    'brave_core_checkout', 'depot_tools', 'json', 'path', 'platform', 'step'
]
