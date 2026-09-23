# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for how `chromium_checkout` relies on the git cache.
"""

from __future__ import annotations

import post_process

DEPS = ['chromium_checkout', 'env', 'path', 'step']


def RunSteps(api):
    mode = api.env.get('MODE')
    if mode == 'should_clone_false':
        api.chromium_checkout.checkout_ref(api.path.chromium_src,
                                           ref='main',
                                           should_clone=False)
    else:
        api.chromium_checkout.ensure_checkout(ref='main')


def GenTests(api):
    # The cache comes from the environment; nothing is passed to the checkout.
    yield api.test(
        'checkout uses the configured cache',
        api.chromium_checkout.with_git_cache(),
        api.path.files('b/src/chrome/VERSION'),
        api.env.on_path('gclient', '/dt/gclient'),
        api.chromium_checkout.git_cache_populated(),
        api.post_process(post_process.MustRun, 'git cache populate for ref'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # Without a cache configured the checkout refuses to run, rather than
    # cloning straight from the remote.
    yield api.test(
        'no cache configured',
        api.post_process(post_process.DoesNotRun, 'gclient config'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # A failing `check chrome/VERSION` means the checkout isn't valid, so
    # ensure_checkout falls back to cloning.
    yield api.test(
        'invalid checkout falls back to clone',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.existing_checkout(),
        api.chromium_checkout.git_cache_populated(),
        api.env.on_path('gclient', '/dt/gclient'),
        api.step.data('check chrome/VERSION', retcode=1),
        api.post_process(post_process.MustRun, 'check chrome/VERSION'),
        api.post_process(post_process.MustRun, 'gclient config'),
        api.post_process(post_process.MustRun, 'clone from git cache'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # should_clone=False refuses to clone a missing checkout.
    yield api.test(
        'should_clone false without existing checkout',
        api.env.set('MODE', 'should_clone_false'),
        api.chromium_checkout.with_git_cache(),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
