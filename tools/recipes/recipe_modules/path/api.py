# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The core `path` module API."""

from __future__ import annotations

from pathlib import Path

from recipe_api import RecipeApi


class PathApi(RecipeApi):
    """Named, workspace-relative job paths, seeded by the engine.

    Mirrors (in miniature) `recipe_engine/path`: recipes build paths from these
    named roots instead of hardcoding them or taking them as properties. Only
    the workspace root is configurable (engine `--workspace`); everything else
    is derived from it, so the on-disk layout is fixed and consistent across
    recipes.
    """

    @property
    def workspace(self) -> Path:
        """Root directory the job runs in (engine-provided)."""
        return self._workspace

    @property
    def chromium_src(self) -> Path:
        """Chromium `src/` checkout: `<workspace>/chromium/src`."""
        return self._workspace / 'chromium' / 'src'

    @property
    def brave_core(self) -> Path:
        """brave-core checkout inside Chromium: `<chromium_src>/brave`."""
        return self.chromium_src / 'brave'

    @property
    def out(self) -> Path:
        """Build output directory: `<workspace>/out`."""
        return self._workspace / 'out'
