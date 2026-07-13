# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `brave_core_shallow` module."""

from __future__ import annotations

import post_process

DEPS = ['brave_core_shallow', 'path', 'step']


def RunSteps(api):
    api.brave_core_shallow.deploy('third_party/node')


def GenTests(api):
    # Fresh clone: `.git` absent, so it clones and sparse-checks-out; the
    # requested path is seeded so the post-checkout existence check passes.
    yield api.test(
        'clone',
        api.path.files('chromium/src/brave/third_party/node'),
        api.post_process(post_process.MustRun,
                         'clone brave-core (shallow, sparse)'),
        api.post_process(post_process.MustRun, 'sparse-checkout add'),
        api.post_process(post_process.StatusSuccess),
    )
    # Existing checkout: `.git` present, so it fetches/checks-out the ref
    # instead of cloning.
    yield api.test(
        'reuse',
        api.path.dirs('chromium/src/brave/.git'),
        api.path.files('chromium/src/brave/third_party/node'),
        api.post_process(post_process.MustRun, 'fetch brave-core ref'),
        api.post_process(post_process.MustRun, 'checkout brave-core ref'),
        api.post_process(post_process.DoesNotRun,
                         'clone brave-core (shallow, sparse)'),
        api.post_process(post_process.StatusSuccess),
    )
    # Requested path missing after checkout -> the module raises.
    yield api.test(
        'missing path',
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
