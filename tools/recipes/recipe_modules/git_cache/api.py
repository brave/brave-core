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
