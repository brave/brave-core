# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `git_cache` module`."""

from __future__ import annotations

import logging
from pathlib import Path
from typing import TYPE_CHECKING

from PB.recipe_modules.brave.git_cache.properties import EnvProperties
from recipe_api import RecipeApi
from recipe_modules.git.api import RefKind

if TYPE_CHECKING:
    from recipe_modules import git_cache
    from recipe_modules.git.api import GitRef

# Flags we want passed in every fetch.
FETCH_ARGS = ('--no-show-forced-updates',)


class GitCacheApi(RecipeApi):
    """Populates and locates the shared git mirrors."""

    m: git_cache.DEPS

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
                f'GIT_CACHE_PATH is not a valid directory: {git_cache_path}'
            )
        logging.info('Using GIT_CACHE_PATH=%s', git_cache_path)
        return git_cache_path

    def populate(
        self,
        url: str,
        *,
        ref: str | None = None,
        commit: str | None = None,
        no_fetch_tags: bool = True,
        step_name: str = 'git cache populate',
    ) -> None:
        """Populate (or refresh) the shared bare mirror for *url*.

        Args:
            url: The repo to mirror.
            ref: An additional ref to fetch into the mirror, beyond its
                default `refs/heads/*`.
            commit: An additional bare commit hash to fetch into the mirror.
            no_fetch_tags: Skip fetching tags that point at fetched objects.
            step_name: Step name for the `git cache populate` call.
        """
        # Disable auto-gc before the populate() call, to avoid long maintainance
        # tasks running. No-op if the mirror doesn't exist in git cache.
        git_config_updated = self._disable_auto_gc(url, step_name, 'before')

        args = [
            'cache',
            'populate',
            '--cache-dir',
            self._path,
            url,
            '--reset-fetch-config',
        ]
        if no_fetch_tags:
            args.append('--no-fetch-tags')
        if ref:
            args.extend(['--ref', ref])
        if commit:
            args.extend(['--commit', commit])
        self.m.git(*args, name=step_name)

        # Apply the auto-gc disabling if it has not been done yet. (There is
        # a small chance for git cache to wipe the shared repo and build again,
        # but then we can skip seting the config for that particular run, and
        # leave it for the next one)
        if not git_config_updated:
            self._disable_auto_gc(url, step_name, 'after')

    def _disable_auto_gc(self, url: str, step_name: str, when: str) -> bool:
        """Stop git's own automatic gc from ever running against this mirror.

        A `git fetch` into the mirror can trigger git's built-in auto-gc,
        which on a repo the size of chromium/src can OOM or take hours (see
        the `git` module's `disable_auto_gc` for what exactly this disables
        and why). A no-op when the mirror doesn't exist yet: a
        freshly-created mirror won't have accumulated enough packs to hit
        this on its own first fetch, and the config set here takes effect
        from its next one.

        Args:
            when: Distinguishes the pre- and post-populate call sites in step
                names (`populate()` runs this twice per call).
        """
        result = self.m.git(
            'cache',
            'exists',
            '--quiet',
            '--cache-dir',
            self._path,
            url,
            name=f'{step_name} exists ({when})',
            stdout=self.m.raw_io.output_text(),
            check=False,
        )
        mirror_dir = result.stdout.strip()
        if not mirror_dir:
            return False

        self.m.git.disable_auto_gc(
            mirror_dir, step_name=f'{step_name} disable ({when})'
        )
        return True

    def mirror_dir(
        self, url: str, *, step_name: str = 'git cache exists'
    ) -> str:
        """The absolute path of the mirror directory for *url*.

        Args:
            url: The mirrored repo.
            step_name: Step name for the `git cache exists` call.

        Returns:
            The mirror's path.
        """
        return self.m.git(
            'cache',
            'exists',
            '--quiet',
            '--cache-dir',
            self._path,
            url,
            name=step_name,
            stdout=self.m.raw_io.output_text(),
        ).stdout.strip()

    def clone_checkout(
        self,
        url: str,
        dest: Path,
        mirror_dir: str,
        ref: GitRef | None = None,
        *,
        step_prefix: str = '',
    ) -> None:
        """Clone *dest* from the populated *mirror_dir* and check out *ref*.

        The checkout shares the mirror's objects, and `origin`'s push URL is
        pointed back at *url*.

        Args:
            url: The mirrored repo.
            dest: The directory to clone into; must not exist yet.
            mirror_dir: The mirror's directory, once populated with *ref*.
            ref: What to check out; `origin/HEAD` if not given.
            step_prefix: Put before every step name, to tell repos apart.
        """
        name = _step_namer(step_prefix)
        self.m.git(
            'clone',
            '--no-checkout',
            '--local',
            '--shared',
            mirror_dir,
            dest,
            name=name('clone from git cache'),
        )
        self.m.git.disable_auto_gc(dest, step_name=name('disable auto-gc'))

        if ref is None:
            self.m.git(
                'checkout',
                '--force',
                'origin/HEAD',
                '--',
                name=name('checkout origin/HEAD'),
                cwd=dest,
            )
        elif ref.kind == RefKind.OTHER:
            # Neither a branch nor a tag, so the clone has no such ref to check
            # out.
            self.m.git(
                'fetch',
                *FETCH_ARGS,
                'origin',
                ref.name,
                name=name('fetch ref'),
                cwd=dest,
            )
            self.m.git(
                'checkout',
                '--force',
                'FETCH_HEAD',
                name=name('checkout ref'),
                cwd=dest,
            )
        else:
            # The clone brought the mirror's branches (as `origin/*`) and tags;
            # a commit is reachable through the shared objects.
            target = (
                f'origin/{ref.branch}' if ref.kind == RefKind.HEAD else ref.name
            )
            self.m.git(
                'checkout',
                '--force',
                target,
                '--',
                name=name(
                    'checkout tag'
                    if ref.kind == RefKind.TAG
                    else 'checkout commit'
                    if ref.kind == RefKind.COMMIT
                    else 'checkout ref'
                ),
                cwd=dest,
            )
        self._restore_push_url(url, dest, name)

    def update_checkout(
        self,
        url: str,
        dest: Path,
        mirror_dir: str,
        ref: GitRef,
        *,
        step_prefix: str = '',
    ) -> None:
        """Bring the existing checkout at *dest* to *ref*, through the mirror.

        The checkout's state is unknown, so it is re-pointed at the populated
        *mirror_dir*, and *ref* is fetched and checked out explicitly.

        Args:
            url: The mirrored repo.
            dest: The existing checkout.
            mirror_dir: The mirror's directory, once populated with *ref*.
            ref: What to check out.
            step_prefix: Put before every step name, to tell repos apart.
        """
        name = _step_namer(step_prefix)
        # As on a fresh clone, so the fetch below cannot set off auto-gc; the
        # checkout may also predate this, and never have had it disabled.
        self.m.git.disable_auto_gc(dest, step_name=name('disable auto-gc'))
        # The checkout may predate the git cache, so point `origin` at the
        # mirror unconditionally. Everything below is then local disk I/O.
        self.m.git(
            'remote',
            'set-url',
            'origin',
            mirror_dir,
            name=name('point origin at git cache'),
            cwd=dest,
        )
        self._restore_push_url(url, dest, name)

        if ref.kind == RefKind.TAG:
            # Fetched as a tag, so it lands at `refs/tags/<ref>`.
            self.m.git(
                'fetch',
                *FETCH_ARGS,
                '--no-tags',
                'origin',
                f'{ref.name}:{ref.name}',
                name=name('fetch tag'),
                cwd=dest,
            )
        else:
            # A branch, qualified ref or bare commit all resolve directly
            # against `origin`.
            self.m.git(
                'fetch',
                *FETCH_ARGS,
                'origin',
                ref.name,
                name=name(
                    'fetch commit'
                    if ref.kind == RefKind.COMMIT
                    else 'fetch ref'
                ),
                cwd=dest,
            )
        # A manual `git checkout --force` rather than `gclient sync -r <ref>`
        # sidesteps a gclient bug; see
        # https://github.com/brave/brave-browser/issues/44921.
        self.m.git(
            'checkout',
            '--force',
            'FETCH_HEAD',
            name=name('checkout FETCH_HEAD'),
            cwd=dest,
        )

    def _restore_push_url(self, url: str, dest: Path, name) -> None:
        # `origin` points at the local mirror; pushes still go to the remote.
        self.m.git(
            'remote',
            'set-url',
            '--push',
            'origin',
            url,
            name=name('restore origin push url'),
            cwd=dest,
        )


def _step_namer(prefix: str):
    """A function naming a step *base*, led by *prefix* if there is one."""
    return lambda base: f'{prefix} {base}' if prefix else base
