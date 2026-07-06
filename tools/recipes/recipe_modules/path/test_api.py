# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for the `path` module: seed the simulated filesystem."""

from __future__ import annotations

from recipe_test_api import RecipeTestApi, TestData


class PathTestApi(RecipeTestApi):
    """Seed which paths "exist" during simulation.

    Paths may be absolute or workspace-relative (relative ones are resolved
    under the simulated workspace, matching `api.path.chromium_src` etc.). Use
    `files(...)` for regular files and `dirs(...)` for directories; a directory
    is also implicitly present if it is an ancestor of any seeded path.
    """

    def files(self, *paths: str) -> TestData:
        return self._mod_data(files=list(paths))

    def dirs(self, *paths: str) -> TestData:
        return self._mod_data(dirs=list(paths))
