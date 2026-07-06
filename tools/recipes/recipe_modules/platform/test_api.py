# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for the `platform` module: choose the simulated host OS."""

from __future__ import annotations

from recipe_test_api import RecipeTestApi, TestData


class PlatformTestApi(RecipeTestApi):
    """Select the platform a `GenTests` case simulates."""

    def name(self, name: str) -> TestData:
        """Simulate host platform *name* (`'linux'`, `'mac'`, or `'win'`)."""
        return self._mod_data(name=name)
