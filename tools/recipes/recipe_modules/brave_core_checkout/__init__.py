# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`brave_core_checkout` module: check out brave-core, in full or sparsely."""

from PB.recipe_modules.brave.brave_core_checkout.properties import (
    InputProperties)

DEPS = [
    'chromium_checkout', 'context', 'depot_tools', 'env', 'file', 'git',
    'git_cache', 'path', 'platform', 'raw_io', 'step'
]

PROPERTIES = InputProperties
