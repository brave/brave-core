# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The targets bundles build without running them."""

from lib.config import targets

# The browser, what `pnpm run build` builds by default.
targets.compile_target(name='brave', label='//brave:brave')
