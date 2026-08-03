# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `chromium_checkout` module API."""

from __future__ import annotations

import contextlib
import functools
import logging
from pathlib import Path
import re
import subprocess

from recipe_api import RecipeApi

# A file that is reliably present in any Chromium checkout, used as a token to
# decide whether a path holds a valid repo.
CHROME_VERSION_FILE = Path('chrome/VERSION')

# Hermetic Windows toolchain base URL, so the checkout can build without a local
# Visual Studio install. Set only when not already configured by the caller.
WIN_HERMETIC_TOOLCHAIN_BASE_URL = (
    'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-'
    '2.on.aws/windows-hermetic-toolchain/')

# The URL for Chromium's googlesource.
CHROMIUM_URL = 'https://chromium.googlesource.com/chromium/src.git'

# Resolves the `GYP_MSVS_HASH_<hash>` override for the hermetic Windows
# toolchain. See `_pin_win_toolchain_hash`.
_WIN_TOOLCHAIN_HASH_SCRIPT = (Path(__file__).resolve().parent / 'resources' /
                              'win_toolchain_hash.py')


class ChromiumCheckoutApi(RecipeApi):
    """Clones, syncs, and validates a Chromium `src/` checkout."""

    @contextlib.contextmanager
    def chromium_layout(self):
        """Context manager entered before any Chromium checkout operation.

        Responsible for basic environment initialization.
        """
        with self.m.context(
                env=
            {
                # CHROME_HEADLESS makes sure that running `gclient
                # runhooks` and other tools don't require user
                # interaction.
                'CHROME_HEADLESS': '1',
            }):
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
    def ensure_checkout(self,
                        *,
                        chromium_src: str | Path | None = None,
                        ref: str | None = None,
                        git_cache: str | Path | None = None,
                        depth: int | None = None) -> Path:
        """Guarantee a Chromium checkout at *chromium_src*, optionally on *ref*.

        Clones a fresh checkout if *chromium_src* is not already a valid
        Chromium repo, then checks out *ref* if given.

        Args:
            chromium_src: Path to the Chromium `src/` directory. Defaults to the
                `path` module's `chromium_src`, the standard job layout.
            ref: Optional git ref (branch, tag, or commit) to check out.
            git_cache: Optional explicit git cache directory. When given it sets
                `GIT_CACHE_PATH`; otherwise an existing `GIT_CACHE_PATH` in the
                environment is used as-is.
            depth: Optional history depth for the shared git-cache mirror (see
                `clone`/`checkout_ref`). `None` fetches full history, matching
                `git cache populate`'s own default.

        Returns:
            The resolved absolute `src/` path.
        """
        if chromium_src is None:
            chromium_src = self.m.path.chromium_src
        chromium_src = self.m.path.abs(chromium_src)

        if git_cache is not None:
            self.set_git_cache(git_cache or None)
        self.validate_git_cache()

        # depot_tools provides `fetch`/`gclient`/`git cache`, needed whether we
        # clone or operate on an existing checkout.
        self.m.depot_tools.ensure_on_path()

        # No valid checkout yet -> clone one.
        if not self.has_valid_checkout(chromium_src):
            logging.info('Chromium src not found at %s, cloning...',
                         chromium_src)
            self.clone(chromium_src, depth=depth)

        if ref:
            self.checkout_ref(chromium_src, ref, depth=depth)
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
        git_cache_path = self.m.env.get('GIT_CACHE_PATH')
        if not git_cache_path:
            raise RuntimeError(
                'GIT_CACHE_PATH is not set; a shared git cache is required. '
                'Set it in the environment or via set_git_cache() before '
                'running the checkout.')
        if not self.m.path.is_dir(git_cache_path):
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
        if 'GIT_CACHE_PATH' in self.m.env:
            raise RuntimeError('GIT_CACHE_PATH is already set in the '
                               'environment.')

        if path:
            git_cache_path = self.m.path.abs(path)
        else:
            home_var = 'USERPROFILE' if self.m.platform.is_win else 'HOME'
            home = self.m.env.get(home_var) or self.m.path.home()
            git_cache_path = Path(home) / 'cache'

        if not self.m.path.is_dir(git_cache_path):
            raise RuntimeError(
                f'GIT_CACHE_PATH is not a valid directory: {git_cache_path}')

        self.m.env.set('GIT_CACHE_PATH', str(git_cache_path))
        logging.info('Set GIT_CACHE_PATH=%s', git_cache_path)
        return git_cache_path

    def has_valid_checkout(self, chromium_src: str | Path) -> bool:
        """Return whether *chromium_src* points to a valid Chromium repo."""
        chromium_src = Path(chromium_src)
        # `chrome/VERSION` is an unmistakable trait of a proper checkout.
        if not self.m.path.exists(chromium_src / CHROME_VERSION_FILE):
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

    def clone(self,
              chromium_src: str | Path,
              *,
              depth: int | None = None) -> None:
        """Clone a fresh Chromium checkout at *chromium_src*.

        Rather than a plain network clone, `git cache populate` fetches into
        a persistent, shared bare mirror under `GIT_CACHE_PATH` (reused across
        every checkout and build on this machine, not just this one), and the
        working checkout is a `--local --shared` clone of it, so re-cloning is
        disk I/O rather than a repeat of the same network fetch.
        """
        chromium_src = Path(chromium_src)
        self.m.path.mkdir(chromium_src.parent)

        # Writes the `.gclient` solution file so `gclient sync` (for DEPS, in
        # `checkout_ref`) knows about the `src` solution once it exists below.
        self.m.step('gclient config', [
            'gclient', 'config', '--name', 'src', '--unmanaged', CHROMIUM_URL
        ],
                    cwd=chromium_src.parent)

        git_cache_path = self.validate_git_cache()
        mirror_dir = self._populate_git_cache(
            git_cache_path,
            CHROMIUM_URL,
            depth=depth,
            populate_step='git cache populate',
            exists_step='git cache exists')
        # `--local --shared`: a same-volume, hardlink-sharing clone of the
        # mirror, effectively free compared to a network clone. `origin` ends up
        # pointing at `mirror_dir` (the clone source), which is exactly what we
        # want for `checkout_ref`'s subsequent fetches to stay local too.
        self.m.step('clone from git cache', [
            'git', 'clone', '--no-checkout', '--local', '--shared', mirror_dir,
            chromium_src
        ])
        self._disable_git_gc(chromium_src)
        self.m.step('checkout origin/HEAD',
                    ['git', 'checkout', '--force', 'origin/HEAD', '--'],
                    cwd=chromium_src)

    def checkout_ref(self,
                     chromium_src: str | Path,
                     ref: str,
                     *,
                     depth: int | None = None) -> None:
        """Check out *ref* in *chromium_src* and resync dependencies."""
        chromium_src = Path(chromium_src)
        logging.info('Checking out Chromium ref %s', ref)
        # Build hermetically without a local VS install, unless the caller has
        # already made an explicit choice about the toolchain.
        using_hermetic_win_toolchain = (self.m.platform.is_win
                                        and 'DEPOT_TOOLS_WIN_TOOLCHAIN'
                                        not in self.m.env)
        if using_hermetic_win_toolchain:
            self.m.env.set('DEPOT_TOOLS_WIN_TOOLCHAIN_BASE_URL',
                           WIN_HERMETIC_TOOLCHAIN_BASE_URL)

        is_tag = bool(re.fullmatch(r'\d+\.\d+\.\d+\.\d+', ref))

        git_cache_path = self.validate_git_cache()
        # The mirror's default `refs/heads/*` fetch already covers branches;
        # tags need an explicit ref, since fetching every tag chromium/src has
        # ever had would be far more expensive than the one we actually want.
        mirror_dir = self._populate_git_cache(
            git_cache_path,
            CHROMIUM_URL,
            ref=f'refs/tags/{ref}' if is_tag else None,
            depth=depth,
            populate_step='git cache populate for ref',
            exists_step='git cache exists for ref')

        # `chromium_src` may already exist from a prior run (or from before
        # this checkout started using a git cache mirror at all), so point
        # `origin` at the mirror unconditionally rather than assuming `clone`
        # already did it. Everything below then runs as local disk I/O instead
        # of talking to the real remote.
        self.m.step('point origin at git cache',
                    ['git', 'remote', 'set-url', 'origin', mirror_dir],
                    cwd=chromium_src)
        self.m.step(
            'restore origin push url',
            ['git', 'remote', 'set-url', '--push', 'origin', CHROMIUM_URL],
            cwd=chromium_src)

        # `--depth` here (not just on the `populate` above) matters whenever
        # *chromium_src* is already shallow at a different commit than the
        # mirror's current one for this ref (e.g. re-checking out a branch
        # after it moved on): a plain `git fetch` tries to extend the
        # existing shallow history and fails outright ("did not send all
        # necessary objects") once the mirror can no longer connect the two.
        # Passing `--depth` here instead negotiates a fresh, self-contained
        # shallow window for the requested ref, which doesn't depend on that
        # connection at all.
        depth_args = ['--depth', str(depth)] if depth else []
        if is_tag:
            # Chromium release tag (e.g. `150.0.7850.1`): fetch it as a tag so
            # it lands at `refs/tags/<ref>` in the local repo.
            self.m.step('fetch tag', [
                'git', 'fetch', *depth_args, '--no-tags', 'origin',
                f'refs/tags/{ref}:refs/tags/{ref}'
            ],
                        cwd=chromium_src)
        else:
            self.m.step('fetch ref',
                        ['git', 'fetch', *depth_args, 'origin', ref],
                        cwd=chromium_src)

        # A manual `git checkout --force` rather than `gclient sync -r <ref>`
        # sidesteps a gclient bug; see
        # https://github.com/brave/brave-browser/issues/44921.
        self.m.step('checkout FETCH_HEAD',
                    ['git', 'checkout', '--force', 'FETCH_HEAD'],
                    cwd=chromium_src)

        if using_hermetic_win_toolchain:
            # This is used by `gclient runhooks`.
            self._pin_win_toolchain_hash(chromium_src)

        self.m.step('gclient sync', ['gclient', 'sync', '--force', '-D'],
                    cwd=chromium_src)

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
        result = self.m.step('resolve win toolchain hash', [
            vpython3, '-u', _WIN_TOOLCHAIN_HASH_SCRIPT,
            chromium_src / 'build' / 'vs_toolchain.py',
            WIN_HERMETIC_TOOLCHAIN_BASE_URL, '--json-output',
            self.m.json.output()
        ],
                             step_test_data=self.test_api.win_toolchain_hash)
        info = result.json.output
        if info['published_hash']:
            self.m.env.set(f"GYP_MSVS_HASH_{info['toolchain_hash']}",
                           info['published_hash'])

    def _populate_git_cache(self,
                            git_cache_path: str | Path,
                            url: str,
                            *,
                            ref: str | None = None,
                            depth: int | None = None,
                            populate_step: str,
                            exists_step: str) -> str:
        """Populate (or refresh) the shared bare mirror for *url*.

        `git cache populate` fetches into a persistent bare mirror under
        `GIT_CACHE_PATH`, which is reused across every checkout and build on
        this machine, rather than the working checkout talking to the remote
        directly. `--no-fetch-tags` skips chromium/src's huge tag history. The
        mirror's default `refs/heads/*` fetch already covers branches, so a
        specific tag *ref* is fetched precisely via `--ref` instead of ever
        needing the full tag catalog.

        Args:
            git_cache_path: `GIT_CACHE_PATH` (the `--cache-dir` for `git
                cache`).
            url: The repo to mirror.
            ref: An additional fully-qualified ref (e.g. `refs/tags/<tag>`) to
                fetch into the mirror, beyond its default `refs/heads/*`.
            depth: Optional history depth; `None` fetches full history.
            populate_step: Step name for the `git cache populate` call.
            exists_step: Step name for the `git cache exists` call.

        Returns:
            The absolute path to the mirror directory.
        """
        populate_cmd = [
            'git', 'cache', 'populate', '--cache-dir', git_cache_path, url,
            '--reset-fetch-config', '--no-fetch-tags'
        ]
        if ref:
            populate_cmd.extend(['--ref', ref])
        if depth:
            populate_cmd.extend(['--depth', str(depth)])
        self.m.step(populate_step, populate_cmd)

        return self.m.step(exists_step, [
            'git', 'cache', 'exists', '--quiet', '--cache-dir', git_cache_path,
            url
        ],
                           stdout=self.m.raw_io.output_text()).stdout.strip()

    def _disable_git_gc(self, chromium_src: str | Path) -> None:
        """Disable background gc in *chromium_src*.

        A shared, long-lived checkout can have several tools touching it
        around the same time, and an auto-triggered `git gc` racing with them
        (or being killed midway) can corrupt the repo.
        """
        for key in ('gc.auto', 'gc.autodetach', 'gc.autopacklimit'):
            self.m.step(f'git config {key}=0', ['git', 'config', key, '0'],
                        cwd=chromium_src)
