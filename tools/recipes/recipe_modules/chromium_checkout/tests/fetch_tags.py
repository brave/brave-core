# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test for `fetch_tags`: pulls every tag into a checkout backed by the shared
git cache, since `gclient sync` itself fetches with `--no-tags`.
"""

from __future__ import annotations

import post_process

DEPS = ['chromium_checkout']


def RunSteps(api):
    api.chromium_checkout.fetch_tags('/b/s/brave-browser/src')


def GenTests(api):
    yield api.test(
        'fetches every tag',
        api.post_process(post_process.StepCommandContains, 'fetch tags',
                         ['git', 'fetch', '--tags', 'origin']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
