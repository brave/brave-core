# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The mixins tests and bundles apply."""

from lib.config import targets

# Brave's ASan test runs leave ODR violations undetected.
targets.mixin(
    name='asan_no_odr_violation_detection',
    args=['--asan-detect-odr-violation=0'],
)
