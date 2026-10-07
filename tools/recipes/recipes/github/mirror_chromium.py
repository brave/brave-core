# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Push Chromium's `main` and tags from the git cache to GitHub."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    chromium_checkout,
    depot_tools,
    env,
    git,
    git_cache,
    path,
    raw_io,
)
from recipe_test_api import RecipeTestApi

GITHUB_URL = 'https://github.com/brave/chromium.git'


@dataclass
class DEPS(RecipeScriptApi):
    chromium_checkout: chromium_checkout.API
    depot_tools: depot_tools.API
    git: git.API
    git_cache: git_cache.API


def RunSteps(api: DEPS) -> None:
    # `git cache` is a depot_tools command.
    api.depot_tools.ensure_on_path()
    mirror_dir = api.git_cache.mirror_dir(api.chromium_checkout.chromium_url)
    api.git(
        'push',
        GITHUB_URL,
        'refs/heads/main:refs/heads/main',
        '--tags',
        name='push to github',
        cwd=mirror_dir,
    )


@dataclass
class TEST_DEPS(RecipeTestApi):
    chromium_checkout: chromium_checkout.TEST_API
    env: env.TEST_API
    path: path.TEST_API
    raw_io: raw_io.TEST_API


def GenTests(api: TEST_DEPS):
    _mirror = '/b/cache/chromium.googlesource.com-chromium-src'

    yield api.test(
        'push from mirror',
        api.chromium_checkout.with_git_cache(),
        api.env.on_path('gclient', '/depot_tools/gclient'),
        api.step_data(
            'git cache exists', stdout=api.raw_io.output_text(f'{_mirror}\n')
        ),
        api.post_process(post_process.DoesNotRun, 'clone depot_tools'),
        api.post_process(post_process.DoesNotRunRE, '.*populate.*'),
        api.post_process(post_process.StatusSuccess),
    )

    yield api.test(
        'no mirror in cache',
        api.chromium_checkout.with_git_cache(),
        api.env.on_path('gclient', '/depot_tools/gclient'),
        api.step_data('git cache exists', retcode=1),
        api.post_process(post_process.DoesNotRun, 'push to github'),
        api.post_process(post_process.StatusFailure),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )
