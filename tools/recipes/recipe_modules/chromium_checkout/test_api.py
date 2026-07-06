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


class ChromiumCheckoutTestApi(RecipeTestApi):
    """Seed the simulated state `chromium_checkout.ensure_checkout` requires."""

    def with_git_cache(self, path: str = _GIT_CACHE) -> TestData:
        """Set `GIT_CACHE_PATH` and mark it a real directory (required)."""
        return self.m.env.set('GIT_CACHE_PATH', path) + self.m.path.dirs(path)

    def existing_checkout(self) -> TestData:
        """Simulate a valid existing checkout (`chrome/VERSION` present)."""
        return self.m.path.files('chromium/src/chrome/VERSION')
