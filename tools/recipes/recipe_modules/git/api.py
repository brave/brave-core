# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `git` module API."""

from __future__ import annotations

from pathlib import Path

from recipe_api import RecipeApi

# When disabling auto-gc, the keys we want to set.
_DISABLE_AUTO_GC_CONFIG = (
    ('gc.auto', '0'),
    ('gc.autodetach', '0'),
    ('gc.autopacklimit', '0'),
    # New versions of git requrire this to be set too on what used to be under
    # gc functionality.
    ('maintenance.gc.enabled', 'false'),
)


class GitApi(RecipeApi):
    """Generic git repository operations shared by modules that own one.
    """

    def disable_auto_gc(self,
                        repo: str | Path,
                        *,
                        step_name: str = 'disable auto-gc') -> None:
        """Stop git's own automatic gc/maintenance from running in *repo*.

        Disabling auto-gc/maintenance is useful in our infra, as a lot of
        workspaces have a limited lifetime, and gc work means resources spent on
        a repo that will be discarded at some point.

        Args:
            repo: Path to the git repository, working tree or bare mirror
                alike; passed as `cwd`, so git's own repository discovery
                resolves it either way.
            step_name: Prefix for each `git config` step's name, so callers
                setting this up at multiple points (e.g. before and after
                another step) can tell them apart in the step list.
        """
        for key, value in _DISABLE_AUTO_GC_CONFIG:
            self.m.step(f'{step_name}: {key}={value}',
                        ['git', 'config', key, value],
                        cwd=repo)
