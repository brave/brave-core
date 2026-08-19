# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `git_cache` module."""

from __future__ import annotations

import post_process

DEPS = ['env', 'git_cache', 'path', 'raw_io', 'step']

_URL = 'https://chromium.googlesource.com/chromium/src.git'


def RunSteps(api):
    # Everything here works against whatever cache `$GIT_CACHE_PATH` names.
    api.step('cache path', ['echo', api.git_cache.validate()])

    api.git_cache.populate(_URL)
    # Extra refs and commits are fetched beyond the mirror's `refs/heads/*`.
    api.git_cache.populate(_URL,
                           ref='refs/branch-heads/1234',
                           commit='c0ffee' * 6,
                           step_name='populate at ref')

    api.step('mirror', ['echo', api.git_cache.mirror_dir(_URL)])


def GenTests(api):
    _mirror = '/b/cache/chromium.googlesource.com-chromium-src'

    # The environment names the cache; nothing else configures it.
    yield api.test(
        'cache from environment',
        api.env.set('GIT_CACHE_PATH', '/b/cache'),
        api.path.dirs('/b/cache'),
        api.step_data('git cache exists',
                      stdout=api.raw_io.output_text(f'{_mirror}\n')),
        api.post_process(post_process.StepCommandContains, 'cache path',
                         ['/b/cache']),
        api.post_process(post_process.StepCommandContains,
                         'git cache populate',
                         ['--reset-fetch-config', '--no-fetch-tags']),
        api.post_process(post_process.StepCommandContains, 'populate at ref',
                         ['--ref', 'refs/branch-heads/1234', '--commit']),
        api.post_process(post_process.StepCommandContains, 'mirror',
                         [_mirror]),
        api.post_process(post_process.StatusSuccess),
    )
    # No cache at all is a hard error rather than an uncached run.
    yield api.test(
        'no cache configured',
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # A configured path that is not a directory is equally fatal.
    yield api.test(
        'cache path is not a directory',
        api.env.set('GIT_CACHE_PATH', '/b/nope'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
