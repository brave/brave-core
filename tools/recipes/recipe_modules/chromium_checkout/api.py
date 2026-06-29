# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `chromium_checkout` module API."""

from __future__ import annotations

import logging
import os
from pathlib import Path
import re
import subprocess
import sys

from recipe_api import RecipeApi

# A file that is reliably present in any Chromium checkout, used as a token to
# decide whether a path holds a valid repo.
CHROME_VERSION_FILE = Path('chrome/VERSION')

# Hermetic Windows toolchain base URL, so the checkout can build without a local
# Visual Studio install. Set only when not already configured by the caller.
WIN_HERMETIC_TOOLCHAIN_BASE_URL = (
    'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-'
    '2.on.aws/windows-hermetic-toolchain/')


class ChromiumCheckoutApi(RecipeApi):
    """Clones, syncs, and validates a Chromium `src/` checkout.

    Extracted from the checkout-related helpers in `build_rust_toolchain.py`
    (`_has_valid_chromium_path`, `_clone_chromium`, `_checkout_chromium_ref`).
    """

    def ensure_checkout(self,
                        chromium_src: str | Path,
                        ref: str | None = None) -> Path:
        """Guarantee a Chromium checkout at *chromium_src*, optionally on *ref*.

        Mirrors the checkout phase of `ToolchainBuilder.run`: validate the git
        cache, ensure depot_tools is on PATH, clone a fresh checkout when none
        is found, then check out *ref* if one is given.

        Args:
            chromium_src: Path to the Chromium `src/` directory.
            ref: Optional git ref (branch, tag, or commit) to check out.

        Returns:
            The resolved absolute `src/` path.
        """
        chromium_src = Path(chromium_src).expanduser().resolve()

        # A missing/invalid GIT_CACHE_PATH would silently break the clone/sync
        # steps below, so fail fast before doing any work.
        self.validate_git_cache()

        # depot_tools provides `fetch`/`gclient`, needed whether we clone or
        # operate on an existing checkout.
        self.m.depot_tools.ensure_on_path(chromium_src)

        # No valid checkout yet -> clone one.
        if not self.has_valid_checkout(chromium_src):
            logging.info('Chromium src not found at %s, cloning...',
                         chromium_src)
            self.clone(chromium_src)

        if ref:
            self.checkout_ref(chromium_src, ref)
        return chromium_src

    def validate_git_cache(self) -> str:
        """Require `GIT_CACHE_PATH` to be set and point to a real directory.

        git/gclient honour `GIT_CACHE_PATH` to share object storage across
        checkouts. The pipeline mandates a cache, so a missing value is a hard
        error -- we refuse to run an uncached checkout -- as is a value that
        does not point at an existing directory.

        Returns:
            The current `GIT_CACHE_PATH` value.

        Raises:
            RuntimeError: If `GIT_CACHE_PATH` is unset or not a directory. Set
                it in the environment or via `set_git_cache()` beforehand.
        """
        git_cache_path = os.environ.get('GIT_CACHE_PATH')
        if not git_cache_path:
            raise RuntimeError(
                'GIT_CACHE_PATH is not set; a shared git cache is required. '
                'Set it in the environment or via set_git_cache() before '
                'running the checkout.')
        if not Path(git_cache_path).is_dir():
            raise RuntimeError(
                f'GIT_CACHE_PATH is not a valid directory: {git_cache_path}')
        logging.info('Using GIT_CACHE_PATH=%s', git_cache_path)
        return git_cache_path

    def set_git_cache(self, path: str | Path | None = None) -> Path:
        """Set `GIT_CACHE_PATH` for subsequent git/gclient steps.

        Mirrors `build_rust_toolchain.py`'s `--with-git-cache` handling: an
        explicit *path* is used as-is (user-expanded); otherwise it defaults to
        `<home>/cache` (`USERPROFILE` on Windows, `HOME` elsewhere), the layout
        our CI bakes the cache under. The directory must already exist, and
        `GIT_CACHE_PATH` must not already be set -- refusing to clobber an
        existing value avoids masking a misconfiguration.

        Args:
            path: Explicit cache directory, or None/empty to use `<home>/cache`.

        Returns:
            The `Path` that `GIT_CACHE_PATH` was set to.

        Raises:
            RuntimeError: If `GIT_CACHE_PATH` is already set, or the resolved
                directory does not exist.
        """
        if 'GIT_CACHE_PATH' in os.environ:
            raise RuntimeError('GIT_CACHE_PATH is already set in the '
                               'environment.')

        if path:
            git_cache_path = Path(path).expanduser()
        else:
            home_var = 'USERPROFILE' if sys.platform == 'win32' else 'HOME'
            home = os.environ.get(home_var, str(Path.home()))
            git_cache_path = Path(home) / 'cache'

        if not git_cache_path.is_dir():
            raise RuntimeError(
                f'GIT_CACHE_PATH is not a valid directory: {git_cache_path}')

        os.environ['GIT_CACHE_PATH'] = str(git_cache_path)
        logging.info('Set GIT_CACHE_PATH=%s', git_cache_path)
        return git_cache_path

    def has_valid_checkout(self, chromium_src: str | Path) -> bool:
        """Return whether *chromium_src* points to a valid Chromium repo."""
        chromium_src = Path(chromium_src)
        # `chrome/VERSION` is an unmistakable trait of a proper checkout.
        if not (chromium_src / CHROME_VERSION_FILE).exists():
            return False

        logging.info('Checking for valid Chromium repo at %s', chromium_src)
        try:
            self.m.step(
                'check chrome/VERSION',
                ['git', 'log', '-1', '--oneline',
                 str(CHROME_VERSION_FILE)],
                cwd=chromium_src)
        except (subprocess.CalledProcessError, OSError):
            return False
        return True

    def clone(self, chromium_src: str | Path) -> None:
        """Clone a fresh Chromium checkout at *chromium_src* via `fetch`."""
        chromium_src = Path(chromium_src)
        chromium_src.parent.mkdir(parents=True, exist_ok=True)
        self.m.step('fetch chromium', ['fetch', '--nohooks', 'chromium'],
                    cwd=chromium_src.parent)

    def checkout_ref(self, chromium_src: str | Path, ref: str) -> None:
        """Check out *ref* in *chromium_src* and resync dependencies."""
        chromium_src = Path(chromium_src)
        logging.info('Checking out Chromium ref %s', ref)
        if (sys.platform == 'win32'
                and 'DEPOT_TOOLS_WIN_TOOLCHAIN' not in os.environ):
            # Build hermetically without a local VS install.
            os.environ['DEPOT_TOOLS_WIN_TOOLCHAIN_BASE_URL'] = (
                WIN_HERMETIC_TOOLCHAIN_BASE_URL)

        if re.fullmatch(r'\d+\.\d+\.\d+\.\d+', ref):
            # Chromium release tag (e.g. `150.0.7850.1`): fetch it as a tag so
            # it lands at `refs/tags/<ref>` in the local repo.
            self.m.step('fetch tag', [
                'git', 'fetch', '--no-tags', 'origin',
                f'refs/tags/{ref}:refs/tags/{ref}'
            ],
                        cwd=chromium_src)
        else:
            self.m.step('fetch ref', ['git', 'fetch', 'origin', ref],
                        cwd=chromium_src)

        # A manual `git checkout --force` rather than `gclient sync -r <ref>`
        # sidesteps a gclient bug; see
        # https://github.com/brave/brave-browser/issues/44921.
        self.m.step('checkout FETCH_HEAD',
                    ['git', 'checkout', '--force', 'FETCH_HEAD'],
                    cwd=chromium_src)
        self.m.step('gclient sync', ['gclient', 'sync', '--force', '-D'],
                    cwd=chromium_src)
