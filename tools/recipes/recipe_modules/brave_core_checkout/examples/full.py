# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `brave_core_checkout` module."""

from __future__ import annotations

import post_process

DEPS = ['brave_core_checkout', 'path', 'step']


def RunSteps(api):
    api.brave_core_checkout.deploy('third_party/node')
    # Deploy the bootstrap shims and put them first on PATH for the block; the
    # prefix is dropped again once the `with` exits.
    with api.brave_core_checkout.bootstrap_on_path():
        api.step('npm version', ['npm', '--version'])


def GenTests(api):
    # Fresh clone: `.git` absent, so it clones and sparse-checks-out; the
    # requested paths are seeded so the post-checkout existence checks pass.
    yield api.test(
        'clone',
        api.path.files('brave-browser/src/brave/third_party/node',
                       'brave-browser/src/brave/tools/cr/bootstrap'),
        api.post_process(post_process.MustRun,
                         'clone brave-core (shallow, sparse)'),
        api.post_process(post_process.MustRun, 'sparse-checkout add'),
        api.post_process(post_process.MustRun, 'npm version'),
        api.post_process(post_process.StatusSuccess),
    )
    # Existing checkout: `.git` present, so it fetches/checks-out the ref
    # instead of cloning.
    yield api.test(
        'reuse',
        api.path.dirs('brave-browser/src/brave/.git'),
        api.path.files('brave-browser/src/brave/third_party/node',
                       'brave-browser/src/brave/tools/cr/bootstrap'),
        api.post_process(post_process.MustRun, 'fetch brave-core ref'),
        api.post_process(post_process.MustRun, 'checkout brave-core ref'),
        api.post_process(post_process.DoesNotRun,
                         'clone brave-core (shallow, sparse)'),
        api.post_process(post_process.StatusSuccess),
    )
    # Already deployed: the live sparse set already covers both requested
    # paths (`third_party/node` directly, `tools/cr/bootstrap` via its
    # `tools/cr` ancestor), so nothing is re-added.
    yield api.test(
        'already deployed',
        api.path.files('brave-browser/src/brave/third_party/node',
                       'brave-browser/src/brave/tools/cr/bootstrap'),
        api.step_data('sparse-checkout list',
                      stdout='third_party/node\ntools/cr\n'),
        api.post_process(post_process.DoesNotRun, 'sparse-checkout add'),
        api.post_process(post_process.MustRun, 'npm version'),
        api.post_process(post_process.StatusSuccess),
    )
    # Requested path missing after checkout -> the module raises.
    yield api.test(
        'missing path',
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
