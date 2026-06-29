# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `chromium_checkout` module API."""

from __future__ import annotations

import ast
from collections.abc import Sequence
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


def _read_gclient_config(gclient_file: Path) -> dict[str, object]:
    """Return the top-level literal assignments in a `.gclient` file.

    A `.gclient` is a Python file of plain assignments (`solutions = [...]`,
    optionally `target_os`, `cache_dir`, ...). Each right-hand side is a literal,
    so they are read with `ast.literal_eval` rather than executing the file.
    Assignments whose value is not a literal are skipped.
    """
    tree = ast.parse(gclient_file.read_bytes().decode('utf-8'))
    config: dict[str, object] = {}
    for node in tree.body:
        if (isinstance(node, ast.Assign) and len(node.targets) == 1
                and isinstance(node.targets[0], ast.Name)):
            try:
                config[node.targets[0].id] = ast.literal_eval(node.value)
            except ValueError:
                continue
    return config


class ChromiumCheckoutApi(RecipeApi):
    """Clones, syncs, and validates a Chromium `src/` checkout.

    Extracted from the checkout-related helpers in `build_rust_toolchain.py`
    (`_has_valid_chromium_path`, `_clone_chromium`, `_checkout_chromium_ref`).
    """

    def ensure_checkout(self,
                        chromium_src: str | Path,
                        ref: str | None = None,
                        target_os: Sequence[str] | None = None) -> Path:
        """Guarantee a Chromium checkout at *chromium_src*, optionally on *ref*.

        Mirrors the checkout phase of `ToolchainBuilder.run`: validate the git
        cache, ensure depot_tools is on PATH, clone a fresh checkout when none
        is found, then check out *ref* if one is given.

        Args:
            chromium_src: Path to the Chromium `src/` directory.
            ref: Optional git ref (branch, tag, or commit) to check out.
            target_os: Optional gclient `target_os` list to configure before the
                sync, so dependencies for those platforms are fetched too (e.g.
                `('win', 'mac', 'linux', 'android')`). Set when *ref* is given,
                since the sync that applies it runs as part of the ref checkout.

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

        # Point depot_tools at the hermetic Windows toolchain before any sync if
        # Windows deps are in play -- host is Windows, or 'win' is a target_os.
        self.ensure_win_toolchain(target_os)

        # Configure target platforms before the sync, so `checkout_ref`'s
        # `gclient sync` pulls their deps in one pass.
        if target_os:
            self.set_target_os(chromium_src, target_os)

        if ref:
            self.checkout_ref(chromium_src, ref)
        return chromium_src

    def set_target_os(self, chromium_src: str | Path,
                      target_os: Sequence[str]) -> None:
        """Configure gclient `target_os` so cross-platform deps are synced.

        Args:
            chromium_src: Path to the Chromium `src/` directory.
            target_os: gclient OS names to sync, e.g. `('win', 'mac', 'linux')`.

        Raises:
            RuntimeError: If `.gclient` is missing or declares no solutions.
        """
        chromium_src = Path(chromium_src).expanduser().resolve()
        parent = chromium_src.parent
        gclient_file = parent / '.gclient'
        if not gclient_file.is_file():
            raise RuntimeError(
                f'.gclient not found at {gclient_file}; the checkout must be '
                'cloned before target_os can be set')

        config = _read_gclient_config(gclient_file)
        if 'solutions' not in config:
            raise RuntimeError(f'no solutions found in {gclient_file}')

        # Preserve every existing assignment (solutions, custom vars, cache_dir,
        # ...) and just (re)set target_os; emit as a spec gclient can exec.
        config['target_os'] = list(target_os)
        spec = '\n'.join(f'{key} = {value!r}' for key, value in config.items())

        # `gclient config` refuses to overwrite an existing .gclient, so remove
        # it first; the spec we just built carries its contents forward.
        gclient_file.unlink()
        self.m.step('gclient config', ['gclient', 'config', '--spec', spec],
                    cwd=parent)
        logging.info('Regenerated %s with target_os=%s', gclient_file,
                     list(target_os))

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

    def ensure_win_toolchain(self,
                             target_os: Sequence[str] | None = None) -> None:
        """Point depot_tools at the hermetic Windows toolchain when needed.

        Windows dependencies are synced whenever the host is Windows or `win` is
        among *target_os*; in either case gclient needs the hermetic toolchain
        URL so it can build without a local Visual Studio install. No-op when no
        Windows deps are in play, when the caller opted out via
        `DEPOT_TOOLS_WIN_TOOLCHAIN`, or when the URL is already set.

        Args:
            target_os: gclient target OS list for the sync, if any.
        """
        targeting_windows = (sys.platform == 'win32'
                             or (target_os is not None and 'win' in target_os))
        if not targeting_windows or 'DEPOT_TOOLS_WIN_TOOLCHAIN' in os.environ:
            return
        os.environ.setdefault('DEPOT_TOOLS_WIN_TOOLCHAIN_BASE_URL',
                              WIN_HERMETIC_TOOLCHAIN_BASE_URL)

    def checkout_ref(self, chromium_src: str | Path, ref: str) -> None:
        """Check out *ref* in *chromium_src* and resync dependencies."""
        chromium_src = Path(chromium_src)
        logging.info('Checking out Chromium ref %s', ref)
        # Cover direct callers: ensure the Windows toolchain URL is set when the
        # host is Windows (ensure_checkout also handles the target_os case).
        self.ensure_win_toolchain()

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

    def fetch_tags(self, chromium_src: str | Path) -> None:
        """Fetch every tag from origin into the *chromium_src* checkout.

        The checkout is backed by the shared git cache, so fetching tags here
        also lands them in the cache -- which is what the mirror step later reads
        and publishes to Gerrit. `gclient sync` fetches with `--no-tags`, so tags
        would otherwise never make it into the cache.

        Args:
            chromium_src: Path to the Chromium `src/` directory.
        """
        chromium_src = Path(chromium_src).expanduser().resolve()
        self.m.step('fetch tags', ['git', 'fetch', '--tags', 'origin'],
                    cwd=chromium_src)
