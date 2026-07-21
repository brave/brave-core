# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the `brave_core_checkout` module's argument validation."""

from __future__ import annotations

import post_process

DEPS = ['brave_core_checkout']


def RunSteps(api):
    # deploy() requires at least one path.
    api.brave_core_checkout.deploy([])


def GenTests(api):
    yield api.test(
        'empty paths',
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
