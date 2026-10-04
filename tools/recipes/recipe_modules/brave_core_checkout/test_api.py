# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `brave_core_checkout`: set up its checkout preconditions.

Exposes the module's simulated preconditions as helpers so a *recipe* that
merely depends on `brave_core_checkout` can arrange them without itself
depending on `path`.
"""

from __future__ import annotations

import json

from recipe_test_api import RecipeTestApi, TestData

# Default simulated git cache directory and the brave-core mirror within it.
_GIT_CACHE = '/b/cache'
_MIRROR_DIR = '/b/cache/github.com-brave-brave-core'

# The simulated Chromium mirror directory.
_CHROMIUM_MIRROR_DIR = '/b/cache/chromium.googlesource.com-chromium-src'

# The Chromium tag the simulated `package.json` pins by default.
_CHROMIUM_TAG = '155.0.8059.16'

# Where the module checks brave-core out (matches `api.path.brave_core`).
_BRAVE_CORE = 'b/src/brave'


class BraveCoreCheckoutTestApi(RecipeTestApi):
    """Seed the simulated state `brave_core_checkout.deploy` inspects."""

    def chromium_mirror_populated(
        self, mirror_dir: str = _CHROMIUM_MIRROR_DIR
    ) -> TestData:
        """Seed the Chromium mirror lookup `ensure_checkout` makes, the second
        `git cache exists`, after the brave-core one."""
        return self.step_data(
            'git cache exists (2)', stdout=self.m.raw_io.output_text(mirror_dir)
        )

    def package_json(self, chromium_tag: str = _CHROMIUM_TAG) -> str:
        """A minimal brave-core `package.json` pinning *chromium_tag*."""
        return json.dumps(
            {'config': {'projects': {'chrome': {'tag': chromium_tag}}}}
        )

    def chromium_tag(self, tag: str) -> TestData:
        """Seed the Chromium tag `read chromium tag` finds in `package.json`."""
        return self.step_data(
            'read chromium tag',
            stdout=self.m.raw_io.output_text(self.package_json(tag)),
        )

    def rbe(self, siso_cache_dir: str = '') -> TestData:
        """Enable remote execution through the module's properties."""
        return self.properties(
            **{
                '$brave_core_checkout': {
                    'use_remoteexec': True,
                    'siso_cache_dir': siso_cache_dir,
                }
            }
        )

    def brave_core_ref(self, ref: str) -> TestData:
        """Set the module's default brave-core ref (its `brave_core_ref`
        property)."""
        return self.properties(
            **{'$brave_core_checkout': {'brave_core_ref': ref}}
        )

    def deployed(self, *paths: str) -> TestData:
        """Mark repo-relative *paths* present after the sparse checkout.

        `deploy` verifies each requested path exists once checked out; seed them
        so a happy-path test passes (omit to exercise the missing-path error).
        """
        return self.m.path.files(*[f'{_BRAVE_CORE}/{p}' for p in paths])

    def existing_checkout(self) -> TestData:
        """Simulate an existing checkout (a `.git` dir), so it fetches, not
        clones."""
        return self.m.path.dirs(f'{_BRAVE_CORE}/.git')

    def with_git_cache(self, path: str = _GIT_CACHE) -> TestData:
        """Set `GIT_CACHE_PATH` and mark it a real directory (required by
        `checkout`)."""
        return self.m.env.set('GIT_CACHE_PATH', path) + self.m.path.dirs(path)

    def git_cache_populated(self, mirror_dir: str = _MIRROR_DIR) -> TestData:
        """Seed `checkout`'s `git cache exists` lookup of the mirror dir."""
        return self.step_data(
            'git cache exists', stdout=self.m.raw_io.output_text(mirror_dir)
        )
