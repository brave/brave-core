# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `json`'s rejection of values `dumps` cannot encode.

`dumps` raises `TypeError`, same as stdlib's `json.dumps`, for a type it (and
`_default_serializer`) doesn't know how to turn into JSON. That is a bug in the
recipe, not a step failure to recover from, so it aborts the recipe rather than
returning some fallback value.
"""

from __future__ import annotations

import post_process

DEPS = ['json']


def RunSteps(api):
    api.json.dumps({'set': {'a', 'b'}})


def GenTests(api):
    yield api.test(
        'not_serializable',
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
