# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `git_cache` module`.
"""

from __future__ import annotations

import logging

from PB.recipe_modules.brave.git_cache.properties import EnvProperties
from recipe_api import RecipeApi


class GitCacheApi(RecipeApi):
    """Populates and locates the shared git mirrors."""

    def __init__(self, env_properties: EnvProperties) -> None:
        del env_properties  # Declared only so ENV_PROPERTIES is documented.
        super().__init__()
        self._path: str | None = None

    def initialise(self) -> None:
        # The path to the git cache repositories.
        self._path = self.validate()

    def validate(self) -> str:
        """Require `GIT_CACHE_PATH` to be set and point to a real directory.

        Returns:
            The current `GIT_CACHE_PATH` value.

        Raises:
            RuntimeError: If `GIT_CACHE_PATH` is unset or not a directory.
        """
        git_cache_path = self.m.env.get('GIT_CACHE_PATH')
        if not git_cache_path:
            raise RuntimeError('GIT_CACHE_PATH is not set.')
        if not self.m.path.is_dir(git_cache_path):
            raise RuntimeError(
                f'GIT_CACHE_PATH is not a valid directory: {git_cache_path}')
        logging.info('Using GIT_CACHE_PATH=%s', git_cache_path)
        return git_cache_path

    def populate(self,
                 url: str,
                 *,
                 ref: str | None = None,
                 commit: str | None = None,
                 no_fetch_tags: bool = True,
                 step_name: str = 'git cache populate') -> None:
        """Populate (or refresh) the shared bare mirror for *url*.

        Args:
            url: The repo to mirror.
            ref: An additional ref to fetch into the mirror, beyond its
                default `refs/heads/*`.
            commit: An additional bare commit hash to fetch into the mirror.
            no_fetch_tags: Skip fetching tags that point at fetched objects.
            step_name: Step name for the `git cache populate` call.
        """
        # Disabled *before* `git cache populate` runs, not just after: the
        # failure this guards against is `git fetch` (run internally by
        # `git cache populate`, below) triggering git's own auto-maintenance
        # and getting OOM-killed or hanging for hours. If that's what this
        # very call is about to do, `git cache populate` never returns, and a
        # config write placed only after it would never run either -- so an
        # already-existing mirror needs the guard in place *before* its next
        # fetch, not applied retroactively once that fetch is done.
        self._disable_auto_gc(url, step_name, 'before')

        cmd = [
            'git', 'cache', 'populate', '--cache-dir', self._path, url,
            '--reset-fetch-config'
        ]
        if no_fetch_tags:
            cmd.append('--no-fetch-tags')
        if ref:
            cmd.extend(['--ref', ref])
        if commit:
            cmd.extend(['--commit', commit])
        self.m.step(step_name, cmd)

        # Also applied afterwards, for the one case the check above can't
        # cover: this call is the one that bootstraps or freshly initialises
        # the mirror, so it didn't exist yet when checked above. It does now,
        # so at least the *next* populate() call is protected.
        self._disable_auto_gc(url, step_name, 'after')

    def _disable_auto_gc(self, url: str, step_name: str, when: str) -> None:
        """Stop git's own automatic gc from ever running against this mirror.

        A `git fetch` into the mirror can trigger git's built-in auto-gc,
        which on recent git versions performs an incremental "geometric
        repack" once the mirror accumulates a handful of pack files. On a
        repo the size of chromium/src that repack is memory-hungry enough to
        get OOM-killed on every fetch, turning a routine `populate()` into an
        hours-long, ultimately fatal step. depot_tools' own
        `gc.autoDetach`/`gc.autoPackLimit=0` (set inside `git cache
        populate` itself) don't cover this newer maintenance path, so it's
        disabled here directly instead.

        A no-op when the mirror doesn't exist yet: a freshly-created mirror
        won't have accumulated enough packs to hit this on its own first
        fetch, and the config set here takes effect from its next one.

        Args:
            when: Distinguishes the pre- and post-populate call sites in step
                names (`populate()` runs this twice per call).
        """
        result = self.m.step(f'{step_name} exists ({when})', [
            'git', 'cache', 'exists', '--quiet', '--cache-dir', self._path,
            url
        ],
                             stdout=self.m.raw_io.output_text(),
                             check=False)
        mirror_dir = result.stdout.strip()
        if not mirror_dir:
            return
        for key, value in (('gc.auto', '0'),
                           ('maintenance.gc.enabled', 'false')):
            self.m.step(f'{step_name} disable {key} ({when})', [
                'git', '--git-dir', mirror_dir, 'config', key, value
            ])

    def mirror_dir(self,
                   url: str,
                   *,
                   step_name: str = 'git cache exists') -> str:
        """The absolute path of the mirror directory for *url*.

        Args:
            url: The mirrored repo.
            step_name: Step name for the `git cache exists` call.

        Returns:
            The mirror's path.
        """
        return self.m.step(step_name, [
            'git', 'cache', 'exists', '--quiet', '--cache-dir', self._path, url
        ],
                           stdout=self.m.raw_io.output_text()).stdout.strip()
