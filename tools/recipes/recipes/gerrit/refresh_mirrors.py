# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Refresh the Gerrit mirrors from a freshly synced git cache.

Runs in two phases:

  1. Clone/sync Chromium at the latest `main`, so the shared git cache fills
     with the host OS's dependency repos, then fetch all tags into the cache.
  2. Once that sync finishes, run this recipe's own `refresh_mirrors.py`
     resource script, publishing every cached repo into Gerrit.

    vpython3 tools/recipes/engine.py gerrit/refresh_mirrors \\
        --properties '{"gerrit_user": "chromium-mirror-bot"}'
"""

from __future__ import annotations

from pathlib import Path
from typing import TYPE_CHECKING

import post_process
from PB.recipes.brave.gerrit.refresh_mirrors import (EnvProperties,
                                                     InputProperties)

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = ['path', 'step', 'chromium_checkout', 'depot_tools']

# Publishes the git cache into Gerrit; lives alongside this recipe (rather than
# in brave-core proper) since this recipe is its only caller.
_REFRESH_MIRRORS_SCRIPT = (Path(__file__).resolve().parent /
                           'refresh_mirrors.resources' / 'refresh_mirrors.py')

PROPERTIES = InputProperties
ENV_PROPERTIES = EnvProperties


def RunSteps(api: RecipeScriptApi, properties: InputProperties,
             env_properties: EnvProperties) -> None:
    # Phase 1: sync Chromium so the git cache is populated, then pull all
    # tags into it (gclient sync fetches with --no-tags).
    chromium_src = api.chromium_checkout.ensure_checkout(
        ref=properties.chromium_ref or 'main',
        git_cache=env_properties.GIT_CACHE or None)
    api.chromium_checkout.fetch_tags(chromium_src)
    git_cache_path = api.chromium_checkout.validate_git_cache()

    # Phase 2: publish the now-populated cache into Gerrit.
    vpython3 = api.depot_tools.vpython3()
    api.step('refresh gerrit mirrors', [
        vpython3,
        _REFRESH_MIRRORS_SCRIPT,
        '--user',
        properties.gerrit_user,
        '--git-cache-path',
        git_cache_path,
    ])


def GenTests(api):
    # Happy path: a fresh checkout (seeded git cache), tags fetched, then the
    # mirror script run.
    yield api.test(
        'fresh checkout',
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.properties(gerrit_user='chromium-mirror-bot'),
        api.post_process(post_process.MustRun, 'clone from git cache'),
        api.post_process(post_process.MustRun, 'fetch tags'),
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
    # A reused checkout (already valid) is still checked out at the given ref,
    # rather than only happening on a fresh clone.
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
