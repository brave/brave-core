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


def _is_tag_ref(ref: str) -> bool:
    """Whether *ref* looks like a Chromium release tag (e.g. `150.0.7850.1`),
    as opposed to a branch name or a commit hash."""
    return bool(re.fullmatch(r'\d+\.\d+\.\d+\.\d+', ref))


def _is_commit_hash_ref(ref: str) -> bool:
    """Whether *ref* looks like a full git commit hash, as opposed to a
    branch name."""
    return bool(re.fullmatch(r'[0-9a-fA-F]{40}', ref))


def _is_fully_qualified_ref(ref: str) -> bool:
    """Whether *ref* is already a fully-qualified ref path.
    """
    return ref.startswith('refs/')


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
            depth: Optional history depth for this working checkout (see
                `checkout_ref`). The shared git-cache mirror is always
                populated with full history regardless; `None` here checks
                out full history too.

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

    def checkout_ref(self,
                     chromium_src: str | Path,
                     ref: str | None = None,
                     *,
                     should_clone: bool = True,
                     depth: int | None = None) -> None:
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
            depth: Optional history depth for this working checkout.

        If *chromium_src* isn't a valid checkout yet, rather than a plain
        network clone, `git cache populate` fetches into a persistent,
        shared bare mirror under `GIT_CACHE_PATH`. The mirror is populated with
        *ref* up front, and the working checkout is pointed straight at it.

        Otherwise, *chromium_src* already exists and its current state
        (branch/tag/commit) isn't known ahead of time, so it's re-pointed at
        the mirror and *ref* is fetched and checked out explicitly.
        """
        chromium_src = Path(chromium_src)
        is_tag = bool(ref and _is_tag_ref(ref))
        is_commit = bool(ref and not is_tag and _is_commit_hash_ref(ref))
        is_qualified_ref = bool(ref and not is_tag and not is_commit
                                and _is_fully_qualified_ref(ref))
        populate_ref = f'refs/tags/{ref}' if is_tag else (
            None if is_commit else ref)
        git_cache_path = self.validate_git_cache()

        if not self.has_valid_checkout(chromium_src):
            if not should_clone:
                raise RuntimeError(
                    f'No valid Chromium checkout at {chromium_src}, and '
                    'should_clone is False.')
            logging.info('Chromium src not found at %s, cloning...',
                         chromium_src)

            self.m.path.mkdir(chromium_src.parent)
            # Writes the `.gclient` solution file so `gclient sync` (once
            # checked out below) knows about the `src` solution.
            self.m.step('gclient config', [
                'gclient', 'config', '--name', 'src', '--unmanaged',
                CHROMIUM_URL
            ],
                        cwd=chromium_src.parent)

            mirror_dir = self._populate_git_cache(
                git_cache_path,
                CHROMIUM_URL,
                ref=populate_ref,
                commit=ref if is_commit else None,
                populate_step='git cache populate',
                exists_step='git cache exists')
            # `--local --shared`: a same-volume, hardlink-sharing clone of
            # the mirror, effectively free compared to a network clone.
            depth_args = ['--depth', str(depth)] if depth else []
            self.m.step('clone from git cache', [
                'git', 'clone', '--no-checkout', '--local', '--shared',
                *depth_args, mirror_dir, chromium_src
            ])
            self._disable_git_gc(chromium_src)

            if is_qualified_ref:
                # `ref` is a fully-qualified ref outside `refs/heads/*` and
                # `refs/tags/*` (e.g. a Chromium release branch under
                # `refs/branch-heads/*`).
                self.m.step('fetch ref',
                            ['git', 'fetch', *depth_args, 'origin', ref],
                            cwd=chromium_src)
                self.m.step('checkout ref',
                            ['git', 'checkout', '--force', 'FETCH_HEAD'],
                            cwd=chromium_src)
            else:
                checkout_target = populate_ref or ref or 'origin/HEAD'
                step_name = ('checkout tag'
                             if is_tag else 'checkout commit' if is_commit else
                             'checkout ref' if ref else 'checkout origin/HEAD')
                self.m.step(
                    step_name,
                    ['git', 'checkout', '--force', checkout_target, '--'],
                    cwd=chromium_src)
            if not ref:
                return
            # `origin`'s push url should still point at the real remote, not
            # the local mirror `git clone` just set it to.
            self.m.step(
                'restore origin push url',
                ['git', 'remote', 'set-url', '--push', 'origin', CHROMIUM_URL],
                cwd=chromium_src)
        elif ref:
            # Already a valid checkout: its current state (branch/tag/commit)
            # is unknown ahead of time, so re-pointing it at `ref` needs an
            # explicit fetch+checkout.
            logging.info('Checking out Chromium ref %s', ref)
            mirror_dir = self._populate_git_cache(
                git_cache_path,
                CHROMIUM_URL,
                ref=populate_ref,
                commit=ref if is_commit else None,
                populate_step='git cache populate for ref',
                exists_step='git cache exists for ref')

            # `chromium_src` may already exist from before this checkout
            # started using a git cache mirror at all, so point `origin` at
            # the mirror unconditionally. Everything below then runs as
            # local disk I/O instead of talking to the real remote.
            self.m.step('point origin at git cache',
                        ['git', 'remote', 'set-url', 'origin', mirror_dir],
                        cwd=chromium_src)
            self.m.step(
                'restore origin push url',
                ['git', 'remote', 'set-url', '--push', 'origin', CHROMIUM_URL],
                cwd=chromium_src)

            # `chromium_src` may already be shallow at a different commit
            # than this ref (e.g. re-checking out a branch after it moved
            # on): a plain `git fetch` tries to extend the existing shallow
            # history and fails outright ("did not send all necessary
            # objects") once the (fully-populated) mirror can no longer
            # connect the two. Passing `--depth` here instead negotiates a
            # fresh, self-contained shallow window for the requested ref,
            # independent of that connection.
            depth_args = ['--depth', str(depth)] if depth else []
            if is_tag:
                # Chromium release tag (e.g. `150.0.7850.1`): fetch it as a
                # tag so it lands at `refs/tags/<ref>` in the local repo.
                self.m.step('fetch tag', [
                    'git', 'fetch', *depth_args, '--no-tags', 'origin',
                    f'refs/tags/{ref}:refs/tags/{ref}'
                ],
                            cwd=chromium_src)
            else:
                # A branch name or a bare commit hash both resolve directly
                # against `origin` -- no destination refspec needed.
                self.m.step('fetch commit' if is_commit else 'fetch ref',
                            ['git', 'fetch', *depth_args, 'origin', ref],
                            cwd=chromium_src)

            # A manual `git checkout --force` rather than `gclient sync -r
            # <ref>` sidesteps a gclient bug; see
            # https://github.com/brave/brave-browser/issues/44921.
            self.m.step('checkout FETCH_HEAD',
                        ['git', 'checkout', '--force', 'FETCH_HEAD'],
                        cwd=chromium_src)
        else:
            # Already a valid checkout and no `ref` requested: nothing to do.
            return

        # `chromium_src` is now checked out at `ref` -- build hermetically
        # without a local VS install, unless the caller has already made an
        # explicit choice about the toolchain.
        using_hermetic_win_toolchain = (self.m.platform.is_win
                                        and 'DEPOT_TOOLS_WIN_TOOLCHAIN'
                                        not in self.m.env)
        if using_hermetic_win_toolchain:
            self.m.env.set('DEPOT_TOOLS_WIN_TOOLCHAIN_BASE_URL',
                           WIN_HERMETIC_TOOLCHAIN_BASE_URL)
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

    def fetch_tags(self, chromium_src: str | Path) -> None:
        """Fetch every tag from origin into the *chromium_src* checkout.

        Args:
            chromium_src: Path to the Chromium `src/` directory.
        """
        chromium_src = self.m.path.abs(chromium_src)
        self.m.step('fetch tags', ['git', 'fetch', '--tags', 'origin'],
                    cwd=chromium_src)

    def _populate_git_cache(self,
                            git_cache_path: str | Path,
                            url: str,
                            *,
                            ref: str | None = None,
                            commit: str | None = None,
                            populate_step: str,
                            exists_step: str) -> str:
        """Populate (or refresh) the shared bare mirror for *url*.

        `git cache populate` fetches into a persistent bare mirror under
        `GIT_CACHE_PATH`, which is reused across every checkout and build on
        this machine, rather than the working checkout talking to the remote
        directly.

        Args:
            git_cache_path: `GIT_CACHE_PATH` (the `--cache-dir` for `git
                cache`).
            url: The repo to mirror.
            ref: An additional ref (a plain branch name, or a fully-qualified
                ref such as `refs/tags/<tag>` or `refs/branch-heads/<n>`) to
                fetch into the mirror, beyond its default `refs/heads/*`.
            commit: An additional bare commit hash to fetch into the mirror.
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
        if commit:
            populate_cmd.extend(['--commit', commit])
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
