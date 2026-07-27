# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test for `ensure_checkout`'s optional `depth` -- threaded down into
`git cache populate --depth` for a shallower shared mirror.
"""

from __future__ import annotations

import post_process

DEPS = ['chromium_checkout']


def RunSteps(api):
    api.chromium_checkout.ensure_checkout(ref='151.0.7917.1', depth=1)


def GenTests(api):
    yield api.test(
        'shallow',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.post_process(post_process.StepCommandContains,
                         'git cache populate', ['--depth', '1']),
        api.post_process(post_process.StepCommandContains,
                         'git cache populate for ref', ['--depth', '1']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
