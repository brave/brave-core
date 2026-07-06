# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `brave_core_shallow`: set up its checkout preconditions.

Exposes the module's simulated preconditions as helpers so a *recipe* that
merely depends on `brave_core_shallow` can arrange them without itself depending
on `path`, mirroring how recipes_py modules provide their own test seams.
"""

from __future__ import annotations

from recipe_test_api import RecipeTestApi, TestData

# Where the module checks brave-core out (matches `api.path.brave_core`).
_BRAVE_CORE = 'chromium/src/brave'


class BraveCoreShallowTestApi(RecipeTestApi):
    """Seed the simulated state `brave_core_shallow.deploy` inspects."""

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
