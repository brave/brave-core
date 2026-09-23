# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Refresh the Gerrit mirrors from a freshly synced git cache.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

import post_process
from PB.recipes.brave.gerrit.refresh_mirrors import InputProperties

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = [
    'path', 'step', 'chromium_checkout', 'depot_tools', 'git_cache', 'raw_io'
]

PROPERTIES = InputProperties


def RunSteps(api: RecipeScriptApi, properties: InputProperties) -> None:
    api.chromium_checkout.ensure_checkout(ref=properties.chromium_ref
                                          or 'main',
                                          run_hooks=False,
                                          git_deps_only=True)

    api.git_cache.populate(api.chromium_checkout.chromium_url,
                           ref='refs/tags/*',
                           no_fetch_tags=False,
                           step_name='fetch tags')

    git_cache_path = api.git_cache.validate()

    # Phase 2: publish the now-populated cache into Gerrit.
    vpython3 = api.depot_tools.vpython3()
    api.step('refresh gerrit mirrors', [
        vpython3,
        api.resource('refresh_mirrors.py'),
        '--user',
        properties.gerrit_user,
        '--git-cache-path',
        git_cache_path,
    ])


def GenTests(api):
    yield api.test(
        'fresh checkout',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.properties(gerrit_user='chromium-mirror-bot'),
        api.step_data('fetch tags exists (before)',
                      stdout=api.raw_io.output_text(
                          '/b/cache/chromium.googlesource'
                          '.com-chromium-src\n')),
        api.post_process(post_process.MustRun, 'clone from git cache'),
        api.post_process(post_process.StepCommandContains, 'fetch tags',
                         ['--ref', 'refs/tags/*']),
        api.post_process(post_process.StepCommandDoesNotContain, 'fetch tags',
                         ['--no-fetch-tags']),
        api.post_process(post_process.MustRun,
                         'fetch tags disable (before): gc.auto=0'),
        api.post_process(
            post_process.MustRun,
            'fetch tags disable (before): maintenance.gc.enabled=false'),
        api.post_process(post_process.MustRun, 'refresh gerrit mirrors'),
        api.post_process(post_process.StepCommandContains,
                         'refresh gerrit mirrors',
                         ['--user', 'chromium-mirror-bot']),
        api.post_process(post_process.StepCommandContains,
                         'refresh gerrit mirrors',
                         ['--git-cache-path', '/b/cache']),
        api.post_process(post_process.StepCommandContains, 'checkout ref',
                         ['main']),
        api.post_process(post_process.StatusSuccess),
    )

    yield api.test(
        'reused checkout at explicit ref',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.existing_checkout(),
        api.chromium_checkout.git_cache_populated(),
        api.properties(gerrit_user='chromium-mirror-bot',
                       chromium_ref='151.0.7917.1'),
        api.post_process(post_process.DoesNotRun, 'clone from git cache'),
        api.post_process(post_process.MustRun, 'fetch tag'),
        api.post_process(post_process.MustRun, 'fetch tags'),
        api.post_process(post_process.MustRun, 'refresh gerrit mirrors'),
        api.post_process(post_process.StatusSuccess),
    )
