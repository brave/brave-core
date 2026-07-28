# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test for `ensure_checkout`'s optional `depth` -- threaded down into both
`git cache populate --depth` (for a shallower shared mirror) and the actual
`git fetch --depth` that lands the requested ref in `chromium_src` (so
re-checking out a moved branch negotiates a fresh shallow window instead of
trying, and failing, to extend the existing one).
"""

from __future__ import annotations

import post_process

DEPS = ['chromium_checkout', 'env']


def RunSteps(api):
    ref = api.env.get('REF')
    depth = api.env.get('DEPTH')
    api.chromium_checkout.ensure_checkout(ref=ref,
                                          depth=int(depth) if depth else None)


def GenTests(api):
    yield api.test(
        'shallow tag',
        api.env.set('REF', '151.0.7917.1'),
        api.env.set('DEPTH', '1'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.post_process(post_process.StepCommandContains,
                         'git cache populate', ['--depth', '1']),
        api.post_process(post_process.StepCommandContains,
                         'git cache populate for ref', ['--depth', '1']),
        api.post_process(post_process.StepCommandContains, 'fetch tag',
                         ['--depth', '1']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    yield api.test(
        'shallow branch',
        api.env.set('REF', 'main'),
        api.env.set('DEPTH', '1'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.post_process(post_process.StepCommandContains,
                         'git cache populate', ['--depth', '1']),
        api.post_process(post_process.StepCommandContains,
                         'git cache populate for ref', ['--depth', '1']),
        api.post_process(post_process.StepCommandContains, 'fetch ref',
                         ['--depth', '1']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # Without an explicit `depth`, neither the mirror populate nor the actual
    # fetch is depth-limited.
    yield api.test(
        'no depth requested',
        api.env.set('REF', 'main'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.post_process(post_process.StepCommandRE, 'fetch ref',
                         ['git', 'fetch', 'origin', 'main']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
