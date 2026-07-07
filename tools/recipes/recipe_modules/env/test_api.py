# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for the `env` module: seed environment vars and `which` results."""

from __future__ import annotations

from recipe_test_api import RecipeTestApi, TestData


class EnvTestApi(RecipeTestApi):
    """Seed the simulated environment a recipe sees."""

    def set(self, key: str, value: str) -> TestData:
        """Set env var *key* before the recipe runs."""
        return self._mod_data(vars={key: value})

    def on_path(self, cmd: str, resolved: str) -> TestData:
        """Make `api.env.which(cmd)` resolve to *resolved* during the run."""
        return self._mod_data(which={cmd: resolved})
