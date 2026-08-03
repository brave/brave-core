# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `chromium_checkout`: set up its checkout preconditions.

`ensure_checkout` requires a valid `GIT_CACHE_PATH` and probes for an existing
checkout; these helpers seed those (via the `env`/`path` seams this module
depends on) so a recipe depending on `chromium_checkout` can arrange them
without depending on `env`/`path` directly.
"""

from __future__ import annotations

from recipe_test_api import RecipeTestApi, TestData

# Default simulated git cache directory.
_GIT_CACHE = '/b/cache'

# Default simulated mirror directory `git cache exists` resolves to.
_MIRROR_DIR = '/b/cache/chromium.googlesource.com-chromium-src'


class ChromiumCheckoutTestApi(RecipeTestApi):
    """Seed the simulated state `chromium_checkout.ensure_checkout` requires."""

    def with_git_cache(self, path: str = _GIT_CACHE) -> TestData:
        """Set `GIT_CACHE_PATH` and mark it a real directory (required)."""
        return self.m.env.set('GIT_CACHE_PATH', path) + self.m.path.dirs(path)

    def existing_checkout(self) -> TestData:
        """Simulate a valid existing checkout (`chrome/VERSION` present)."""
        return self.m.path.files('b/src/chrome/VERSION')

    def git_cache_populated(self, mirror_dir: str = _MIRROR_DIR) -> TestData:
        """Seed `clone`/`checkout_ref`'s `git cache exists` lookups.

        Both `clone` and `checkout_ref` run a `git cache exists` step to
        resolve the mirror directory `git cache populate` just fetched into;
        required whenever either of them runs, since the simulated stdout is
        an empty string unless seeded.
        """
        stdout = self.m.raw_io.output_text(mirror_dir)
        return (self.step_data('git cache exists', stdout=stdout) +
                self.step_data('git cache exists for ref', stdout=stdout))

    def win_toolchain_hash(self,
                           toolchain_hash: str = '',
                           published_hash: str | None = None) -> TestData:
        """Seed `_pin_win_toolchain_hash`'s lookup result.

        This is `checkout_ref`'s own default for the `resolve win toolchain
        hash` step (via `step_test_data`): with no *published_hash*, nothing
        is published yet for the upstream hash, so `GYP_MSVS_HASH_*` stays
        unset -- matching what an unseeded step should report.
        """
        return self.m.json.output({
            'toolchain_hash': toolchain_hash,
            'published_hash': published_hash,
        })

    def win_toolchain_published(self, toolchain_hash: str,
                                published_hash: str) -> TestData:
        """Simulate Brave having already republished a toolchain for the
        upstream *toolchain_hash*, resolving to *published_hash*.
        """
        return self.step_data(
            'resolve win toolchain hash',
            self.win_toolchain_hash(toolchain_hash, published_hash))
