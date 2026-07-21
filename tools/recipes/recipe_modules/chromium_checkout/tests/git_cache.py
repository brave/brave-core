# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `chromium_checkout`'s git-cache resolution and checkout probing.

The behaviour under test is selected by a seeded `MODE` env var, so each case
drives one branch of `set_git_cache` / `validate_git_cache` / `ensure_checkout`
without needing a typed PROPERTIES message.
"""

from __future__ import annotations

import post_process

DEPS = ['chromium_checkout', 'env', 'path', 'step']


def RunSteps(api):
    mode = api.env.get('MODE')
    if mode == 'with_cache':
        api.chromium_checkout.ensure_checkout(ref='main', git_cache='/b/cache')
    elif mode == 'default_home':
        api.chromium_checkout.set_git_cache()
    elif mode == 'already_set':
        api.chromium_checkout.set_git_cache('/b/cache')
    elif mode == 'bad_dir':
        api.chromium_checkout.set_git_cache('/b/nonexistent')
    elif mode == 'validate_bad_dir':
        api.chromium_checkout.validate_git_cache()
    elif mode == 'invalid_checkout':
        api.chromium_checkout.ensure_checkout(ref='main')


def GenTests(api):
    # An explicit git_cache dir is set (and validated) before the checkout.
    yield api.test(
        'ensure with cache',
        api.env.set('MODE', 'with_cache'),
        api.path.dirs('/b/cache'),
        api.path.files('brave-browser/src/chrome/VERSION'),
        api.env.on_path('gclient', '/dt/gclient'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # set_git_cache() with no argument defaults to <HOME>/cache.
    yield api.test(
        'default home cache',
        api.env.set('MODE', 'default_home'),
        api.path.dirs('/b/home/cache'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # set_git_cache when GIT_CACHE_PATH is already set is an error.
    yield api.test(
        'already set',
        api.env.set('MODE', 'already_set'),
        api.env.set('GIT_CACHE_PATH', '/b/cache'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # set_git_cache pointed at a non-existent directory is an error.
    yield api.test(
        'bad cache dir',
        api.env.set('MODE', 'bad_dir'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # validate_git_cache with GIT_CACHE_PATH pointing at a non-dir is an error.
    yield api.test(
        'validate bad dir',
        api.env.set('MODE', 'validate_bad_dir'),
        api.env.set('GIT_CACHE_PATH', '/b/nope'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # A failing `check chrome/VERSION` means the checkout isn't valid, so
    # ensure_checkout falls back to cloning.
    yield api.test(
        'invalid checkout falls back to clone',
        api.env.set('MODE', 'invalid_checkout'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.existing_checkout(),
        api.env.on_path('gclient', '/dt/gclient'),
        api.step.data('check chrome/VERSION', retcode=1),
        api.post_process(post_process.MustRun, 'check chrome/VERSION'),
        api.post_process(post_process.MustRun, 'fetch chromium'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
