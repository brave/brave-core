# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `chromium_checkout` module API."""

from __future__ import annotations

import contextlib
import functools
import logging
import subprocess
from typing import TYPE_CHECKING

from recipe_api import RecipeApi

if TYPE_CHECKING:
    from recipe_modules.git_cache.api import GitRef

# `chrome/VERSION` is a reliable fingerprint for a Chromium repo.
CHROME_VERSION_FILE = 'chrome/VERSION'

# Hermetic Windows toolchain base URL, so the checkout can build without a local
# Visual Studio install. Set only when not already configured by the caller.
WIN_HERMETIC_TOOLCHAIN_BASE_URL = (
    'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-'
    '2.on.aws/windows-hermetic-toolchain/'
)

# The URL for Chromium's googlesource.
CHROMIUM_URL = 'https://chromium.googlesource.com/chromium/src.git'


class ChromiumCheckoutApi(RecipeApi):
    """Clones, syncs, and validates a Chromium `src/` checkout."""

    @property
    def chromium_url(self) -> str:
        """The Chromium repo this module checks out and mirrors."""
        return CHROMIUM_URL

    @contextlib.contextmanager
    def chromium_layout(self):
        """Context manager entered before any Chromium checkout operation.

        Responsible for basic environment initialization.
        """
        with self.m.context(
            env={
                # CHROME_HEADLESS makes sure that running `gclient
                # runhooks` and other tools don't require user
                # interaction.
                'CHROME_HEADLESS': '1',
            }
        ):
            yield

    def _with_chromium_layout(fn):
        """Decorator applying `chromium_layout()` to a bound
        `ChromiumCheckoutApi` method.

        INTERNAL: decorates `ChromiumCheckoutApi` member functions only; do
        not use outside this class/module.
        """

        @functools.wraps(fn)
        def inner(self, *args, **kwargs):
            with self.chromium_layout():
                return fn(self, *args, **kwargs)

        return inner

    @_with_chromium_layout
    def ensure_checkout(
        self,
        *,
        chromium_src: str | Path | None = None,
        ref: str | None = None,
        run_sync: bool = True,
        run_hooks: bool = True,
        git_deps_only: bool = False,
    ) -> Path:
        """Guarantee a Chromium checkout at *chromium_src*, optionally on *ref*.

        Clones a fresh checkout if *chromium_src* is not already a valid
        Chromium repo, then checks out *ref* if given.

        The checkout is always made through the shared git cache, so it requires
        a valid git cache.

        Args:
            chromium_src: Path to the Chromium `src/` directory. Defaults to the
                `path` module's `chromium_src`, the standard job layout.
            ref: Optional git ref (branch, tag, or commit) to check out.
            run_sync: Whether `gclient sync` runs after the checkout (the
                default). See `checkout_ref`.
            run_hooks: Whether the sync runs the DEPS hooks (the default). See
                `checkout_ref`.
            git_deps_only: Sync only the git dependencies. See `checkout_ref`.

        Returns:
            The resolved absolute `src/` path.
        """
        if chromium_src is None:
            chromium_src = self.m.path.chromium_src
        chromium_src = self.m.path.abs(chromium_src)

        # depot_tools provides `fetch`/`gclient`/`git cache`, needed whether we
        # clone or operate on an existing checkout.
        self.m.depot_tools.ensure_on_path()

        self.checkout_ref(
            chromium_src,
            ref,
            run_sync=run_sync,
            run_hooks=run_hooks,
            git_deps_only=git_deps_only,
        )
        return chromium_src

    def has_valid_checkout(self, chromium_src: str | Path) -> bool:
        """Return whether *chromium_src* points to a valid Chromium repo."""
        chromium_src = self.m.path.abs(chromium_src)
        # `chrome/VERSION` is an unmistakable trait of a proper checkout.
        if not self.m.path.exists(chromium_src / CHROME_VERSION_FILE):
            return False

        logging.info('Checking for valid Chromium repo at %s', chromium_src)
        try:
            self.m.step(
                'check chrome/VERSION',
                ['git', 'log', '-1', '--oneline', str(CHROME_VERSION_FILE)],
                cwd=chromium_src,
            )
        except (subprocess.CalledProcessError, OSError):
            return False
        return True

    def checkout_ref(
        self,
        chromium_src: str | Path,
        ref: str | None = None,
        *,
        should_clone: bool = True,
        run_sync: bool = True,
        run_hooks: bool = True,
        git_deps_only: bool = False,
    ) -> None:
        """Ensure *chromium_src* is checked out at *ref*.

        Args:
            chromium_src: Path to the Chromium `src/` directory.
            ref: Git ref (branch, tag, or commit) to check out. `origin/HEAD`
                if not given and *chromium_src* needs cloning; a no-op if not
                given and *chromium_src* is already checked out.
            should_clone: Whether cloning *chromium_src* is allowed if it
                doesn't already hold a valid checkout (the default). Set to
                False to require an existing checkout, raising instead of
                cloning one.
            run_sync: Whether `gclient sync` runs after *chromium_src* is
                checked out (the default). Set to False to leave syncing
                (DEPS dependencies and hooks) to the caller; *run_hooks* and
                *git_deps_only* are then ignored.
            run_hooks: Whether the sync runs the DEPS hooks (the default).
            git_deps_only: Sync only the git dependencies, skipping the CIPD
                packages and GCS objects DEPS.

        If *chromium_src* isn't a valid checkout yet, rather than a plain
        network clone, `git cache populate` fetches into a persistent,
        shared bare mirror under `GIT_CACHE_PATH`. The mirror is populated with
        *ref* up front, and the working checkout is pointed straight at it.

        Otherwise, *chromium_src* already exists and its current state
        (branch/tag/commit) isn't known ahead of time, so it's re-pointed at
        the mirror and *ref* is fetched and checked out explicitly.
        """
        chromium_src = self.m.path.abs(chromium_src)
        git_ref = self.m.git_cache.parse_ref(ref) if ref else None

        if not self.has_valid_checkout(chromium_src):
            if not should_clone:
                raise RuntimeError(
                    f'No valid Chromium checkout at {chromium_src}, and '
                    'should_clone is False.'
                )
            logging.info(
                'Chromium src not found at %s, cloning...', chromium_src
            )

            self.m.path.mkdir(chromium_src.parent)
            # Writes the `.gclient` solution file so `gclient sync` (once
            # checked out below) knows about the `src` solution.
            self.m.step(
                'gclient config',
                [
                    'gclient',
                    'config',
                    '--name',
                    'src',
                    '--unmanaged',
                    CHROMIUM_URL,
                ],
                cwd=chromium_src.parent,
            )

            mirror_dir = self._populate_git_cache(
                git_ref,
                populate_step='git cache populate',
                exists_step='git cache exists',
            )
            self.m.git_cache.clone_checkout(
                CHROMIUM_URL, chromium_src, mirror_dir, git_ref
            )
            if not ref:
                return
        elif ref:
            # Already a valid checkout: its current state (branch/tag/commit)
            # is unknown ahead of time, so re-pointing it at `ref` needs an
            # explicit fetch+checkout.
            logging.info('Checking out Chromium ref %s', ref)
            mirror_dir = self._populate_git_cache(
                git_ref,
                populate_step='git cache populate for ref',
                exists_step='git cache exists for ref',
            )
            self.m.git_cache.update_checkout(
                CHROMIUM_URL, chromium_src, mirror_dir, git_ref
            )
        else:
            # Already a valid checkout and no `ref` requested: nothing to do.
            return

        if not run_sync:
            return

        using_hermetic_win_toolchain = (
            run_hooks
            and self.m.platform.is_win
            and 'DEPOT_TOOLS_WIN_TOOLCHAIN' not in self.m.env
        )
        if using_hermetic_win_toolchain:
            self.m.env.set(
                'DEPOT_TOOLS_WIN_TOOLCHAIN_BASE_URL',
                WIN_HERMETIC_TOOLCHAIN_BASE_URL,
            )
            # This is used by `gclient runhooks`.
            self._pin_win_toolchain_hash(chromium_src)

        sync_cmd = ['gclient', 'sync', '--force', '-D']
        if not run_hooks:
            sync_cmd.append('--nohooks')
        if git_deps_only:
            sync_cmd += ['--ignore-dep-type=gcs', '--ignore-dep-type=cipd']
        self.m.step('gclient sync', sync_cmd, cwd=chromium_src)

    def _pin_win_toolchain_hash(self, chromium_src: Path) -> None:
        """Point `GYP_MSVS_HASH_<hash>` at Brave's republished toolchain.

        `build/vs_toolchain.py` pins a `TOOLCHAIN_HASH`, which the
        `win_toolchain` gclient hook resolves to `<TOOLCHAIN_HASH>.zip` on
        Google's own toolchain bucket. Overriding
        `DEPOT_TOOLS_WIN_TOOLCHAIN_BASE_URL` points the hook at our own bucket.
        `GYP_MSVS_HASH_<TOOLCHAIN_HASH>` is the override
        `_GetDesiredVsToolchainHashes` (in `build/vs_toolchain.py`) reads to
        substitute a different hash, so setting it to the hash Brave actually
        published the archive under.

        Nothing is set if an index with cannot be found with a redirect.
        """
        vpython3 = self.m.depot_tools.vpython3()
        result = self.m.step(
            'resolve win toolchain hash',
            [
                vpython3,
                '-u',
                self.resource('win_toolchain_hash.py'),
                chromium_src / 'build' / 'vs_toolchain.py',
                WIN_HERMETIC_TOOLCHAIN_BASE_URL,
                '--json-output',
                self.m.json.output(),
            ],
            step_test_data=self.test_api.win_toolchain_hash,
        )
        info = result.json.output
        if info['published_hash']:
            self.m.env.set(
                f"GYP_MSVS_HASH_{info['toolchain_hash']}",
                info['published_hash'],
            )

    def _populate_git_cache(
        self, ref: GitRef | None, *, populate_step: str, exists_step: str
    ) -> str:
        """Populate (or refresh) the shared bare mirror of Chromium.

        `git cache populate` fetches into a persistent bare mirror under
        `GIT_CACHE_PATH`, which is reused across every checkout and build on
        this machine, rather than the working checkout talking to the remote
        directly.

        Args:
            ref: The ref to fetch into the mirror beyond its default
                `refs/heads/*`, if any.
            populate_step: Step name for the `git cache populate` call.
            exists_step: Step name for the `git cache exists` call.

        Returns:
            The absolute path to the mirror directory.
        """
        self.m.git_cache.populate(
            CHROMIUM_URL,
            ref=ref.populate_ref if ref else None,
            commit=ref.commit if ref else None,
            step_name=populate_step,
        )
        return self.m.git_cache.mirror_dir(CHROMIUM_URL, step_name=exists_step)
