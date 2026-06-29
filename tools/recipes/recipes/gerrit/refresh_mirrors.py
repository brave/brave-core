# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Refresh the Gerrit mirrors from a freshly synced git cache.

Runs in two phases:

  1. Clone/sync Chromium at the latest `main`, with every desktop and mobile
     platform added to gclient's `target_os`, so the shared git cache fills with
     as many dependency repos as possible, then fetch all tags into the cache.
  2. Once that sync finishes, deploy `tools/cr/refresh_mirrors.py` from
     brave-core and run it, publishing every cached repo into Gerrit.

    python3 tools/recipes/engine.py gerrit/refresh_mirrors \\
        --properties '{"gerrit_user": "chromium-mirror-bot"}'
"""

from __future__ import annotations

import sys
from dataclasses import dataclass
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = ['path', 'step', 'chromium_checkout', 'brave_core_shallow']

# Pull dependencies for every platform we ship, so the cache (and therefore the
# mirrors) covers the full dependency set rather than just the host OS's.
TARGET_OS = ['android', 'linux', 'mac', 'win']


@dataclass(frozen=True)
class InputProperties:
    # Gerrit SSH user the script authenticates as (e.g. "chromium-mirror-bot").
    gerrit_user: str
    # Chromium ref to sync before mirroring; defaults to the latest main.
    chromium_ref: str = 'main'
    # Optional override for the git cache root; defaults to $GIT_CACHE_PATH in
    # the environment, shared by the sync and the mirror step.
    git_cache: str | None = None


PROPERTIES = InputProperties


def RunSteps(api: RecipeScriptApi, properties: InputProperties) -> None:
    if properties.git_cache is not None:
        api.chromium_checkout.set_git_cache(properties.git_cache or None)

    # Phase 1: sync all-platform Chromium so the git cache is fully populated,
    # then pull all tags into it (gclient sync fetches with --no-tags).
    api.chromium_checkout.ensure_checkout(api.path.chromium_src,
                                          ref=properties.chromium_ref,
                                          target_os=TARGET_OS)
    api.chromium_checkout.fetch_tags(api.path.chromium_src)

    # Phase 2: publish the now-populated cache into Gerrit.
    brave_core_root = api.brave_core_shallow.deploy(api.path.brave_core,
                                                    'tools/cr')

    cmd = [
        sys.executable,
        brave_core_root / 'tools/cr/refresh_mirrors.py',
        '--user',
        properties.gerrit_user,
    ]
    if properties.git_cache:
        cmd += ['--git-cache-path', properties.git_cache]

    api.step('refresh gerrit mirrors', cmd)
