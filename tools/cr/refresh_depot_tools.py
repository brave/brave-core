#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Rebase Brave's depot_tools patches onto upstream and mirror the result.

Clones depot_tools from the upstream googlesource repository into the given
clone directory and fetches the `patches` branch from the brave-experiments
mirror. The work is conditional:

  * `patches` is rebased onto upstream `main` only when it has fallen behind --
    i.e. it no longer sits on the current `origin/main` tip. If the rebase fails
    the script stops with an error; otherwise the rebased branch is force-pushed
    back to brave-experiments.
  * `main` is reset to `patches` and pushed only when it does not already point
    at the `patches` HEAD -- so it advances whenever `patches` is rebased or has
    commits merged into it out of band, and is left untouched otherwise.

Usage:
    tools/cr/refresh_depot_tools.py --clone-dir <path>
"""

from __future__ import annotations

import argparse
import logging
import subprocess
import sys
from pathlib import Path

from terminal import terminal

_LOG = logging.getLogger(__name__)

# Upstream Chromium depot_tools repository, cloned as the `origin` remote.
_GOOGLESOURCE_URL = (
    'https://chromium.googlesource.com/chromium/tools/depot_tools')
_ORIGIN_REMOTE = 'origin'

# Remote name and URL for the brave-experiments mirror.
_BRAVE_EXPERIMENTS_REMOTE = 'brave-experiments'
_BRAVE_EXPERIMENTS_URL = 'git@github.com:brave-experiments/depot_tools.git'

# The upstream branch Brave's patches are rebased onto.
_MAIN_BRANCH = 'main'

# The brave-experiments branch holding Brave's depot_tools patches.
_PATCHES_BRANCH = 'patches'

# Directory name for the checkout created under the clone directory.
_CHECKOUT_NAME = 'depot_tools'


class RebaseError(RuntimeError):
    """Raised when the patches branch cannot be rebased onto upstream."""


def _run_git(repo: Path, *args: str) -> str:
    """Runs a git command against the checkout at `repo`.

    Git is invoked with `-C <repo>` so the command always targets that checkout
    regardless of the caller's cwd.

    Args:
        repo: Path to the git checkout.
        *args: Arguments passed through to git.

    Returns:
        The trimmed stdout of the git command.
    """
    return terminal.run_git('-C', str(repo), *args)


def _fetch_remotes(repo: Path) -> dict[str, str]:
    """Returns a mapping of remote name to fetch URL for the checkout."""
    remotes: dict[str, str] = {}
    for line in _run_git(repo, 'remote', '-v').splitlines():
        # Each line has the form "<name>\t<url> (fetch|push)".
        parts = line.split()
        if len(parts) >= 3 and parts[-1] == '(fetch)':
            remotes[parts[0]] = parts[1]
    return remotes


def _ensure_remote(repo: Path, remotes: dict[str, str], name: str,
                   url: str) -> None:
    """Adds a remote if missing, or corrects its URL if it points elsewhere.

    Args:
        repo: Path to the git checkout.
        remotes: The currently configured remotes, as returned by
            `_fetch_remotes`.
        name: The remote name to ensure.
        url: The URL the remote should point at.
    """
    current = remotes.get(name)
    if current is None:
        _LOG.info('Adding remote "%s" -> %s', name, url)
        _run_git(repo, 'remote', 'add', name, url)
    elif current != url:
        _LOG.info('Updating remote "%s" URL: %s -> %s', name, current, url)
        _run_git(repo, 'remote', 'set-url', name, url)
    else:
        _LOG.info('Remote "%s" already configured (%s)', name, url)


def _clone_or_update(clone_dir: Path) -> Path:
    """Ensures an up-to-date depot_tools checkout under `clone_dir`.

    Clones depot_tools from upstream when no checkout is present, otherwise
    reuses the existing one and fetches `main`.

    Args:
        clone_dir: Directory the checkout is created under.

    Returns:
        The path to the depot_tools checkout.
    """
    checkout = clone_dir / _CHECKOUT_NAME
    if (checkout / '.git').is_dir():
        _LOG.info('Reusing existing depot_tools checkout at %s', checkout)
        _ensure_remote(checkout, _fetch_remotes(checkout), _ORIGIN_REMOTE,
                       _GOOGLESOURCE_URL)
        _LOG.info('Fetching %s from %s...', _MAIN_BRANCH, _ORIGIN_REMOTE)
        _run_git(checkout, 'fetch', _ORIGIN_REMOTE, _MAIN_BRANCH)
    else:
        clone_dir.mkdir(parents=True, exist_ok=True)
        _LOG.info('Cloning depot_tools from %s into %s', _GOOGLESOURCE_URL,
                  checkout)
        terminal.run_git('clone', _GOOGLESOURCE_URL, str(checkout))
    return checkout


def _rev_parse(repo: Path, ref: str) -> str | None:
    """Returns the commit SHA for `ref`, or None if it does not exist."""
    try:
        return _run_git(repo, 'rev-parse', '--verify', '--quiet', ref)
    except subprocess.CalledProcessError:
        return None


def _rebase_patches(checkout: Path, origin_main: str, upstream: str) -> None:
    """Rebases the local `patches` branch onto `origin_main`.

    Replays only the commits `patches` carries on top of `upstream` (its
    previous upstream base) so patches that were already upstreamed drop out
    cleanly.

    Args:
        checkout: Path to the depot_tools checkout, with `patches` checked out.
        origin_main: The upstream ref to rebase onto (e.g. `origin/main`).
        upstream: The old base to replay from -- commits in `upstream..patches`
            are the ones moved onto `origin_main`.

    Raises:
        RebaseError: If the rebase does not apply cleanly. The in-progress
            rebase is aborted before raising so the checkout is left clean.
    """
    _LOG.info('Rebasing %s onto %s (from %s)...', _PATCHES_BRANCH, origin_main,
              upstream)
    try:
        _run_git(checkout, 'rebase', '--onto', origin_main, upstream,
                 _PATCHES_BRANCH)
    except subprocess.CalledProcessError as e:
        # Leave the checkout in a clean state before surfacing the failure.
        try:
            _run_git(checkout, 'rebase', '--abort')
        except subprocess.CalledProcessError:
            pass
        raise RebaseError(
            f'Failed to rebase {_PATCHES_BRANCH} onto {origin_main}; resolve '
            'the conflicts manually and re-run.') from e


def refresh(clone_dir: Path) -> None:
    """Rebases Brave's patches onto upstream and mirrors the result.

    Args:
        clone_dir: Directory the depot_tools checkout is created under.

    Raises:
        RebaseError: If the patches branch cannot be rebased onto upstream.
    """
    checkout = _clone_or_update(clone_dir)
    _ensure_remote(checkout, _fetch_remotes(checkout),
                   _BRAVE_EXPERIMENTS_REMOTE, _BRAVE_EXPERIMENTS_URL)

    # Fetch the mirror's branches (into remote-tracking refs) and check out a
    # local patches branch. `checkout -B` resets it even when `patches` is
    # already the current branch on a reused checkout.
    _LOG.info('Fetching branches from %s...', _BRAVE_EXPERIMENTS_REMOTE)
    _run_git(checkout, 'fetch', _BRAVE_EXPERIMENTS_REMOTE)
    _run_git(checkout, 'checkout', '-B', _PATCHES_BRANCH,
             f'{_BRAVE_EXPERIMENTS_REMOTE}/{_PATCHES_BRANCH}')

    # Only rebase when patches has fallen behind upstream. If origin/main is
    # already an ancestor of patches -- its merge base with patches equals
    # origin/main -- then patches still sits on the current upstream tip and
    # needs no rebase.
    origin_main = f'{_ORIGIN_REMOTE}/{_MAIN_BRANCH}'
    origin_main_sha = _run_git(checkout, 'rev-parse', origin_main)
    merge_base = _run_git(checkout, 'merge-base', origin_main, _PATCHES_BRANCH)
    if merge_base == origin_main_sha:
        _LOG.info('%s already sits on the current %s tip; skipping rebase.',
                  _PATCHES_BRANCH, origin_main)
    else:
        _rebase_patches(checkout, origin_main, merge_base)
        # The rebase rewrote history, so the push must be forced.
        _LOG.info('Pushing rebased %s to %s...', _PATCHES_BRANCH,
                  _BRAVE_EXPERIMENTS_REMOTE)
        _run_git(checkout, 'push', '--force', _BRAVE_EXPERIMENTS_REMOTE,
                 f'{_PATCHES_BRANCH}:refs/heads/{_PATCHES_BRANCH}')

    # Move main to follow patches, but only when it isn't already there. This
    # advances main whenever patches was rebased above or had commits merged
    # into it out of band.
    patches_sha = _run_git(checkout, 'rev-parse', _PATCHES_BRANCH)
    mirror_main = f'{_BRAVE_EXPERIMENTS_REMOTE}/{_MAIN_BRANCH}'
    if patches_sha == _rev_parse(checkout, mirror_main):
        _LOG.info('%s already points at %s HEAD; nothing to update.',
                  _MAIN_BRANCH, _PATCHES_BRANCH)
    else:
        # patches is strictly ahead of main after a rebase-and-push or an
        # out-of-band merge, so this fast-forwards; force keeps it robust to a
        # main that was itself reset in a prior run.
        _LOG.info('Resetting %s to %s and pushing to %s...', _MAIN_BRANCH,
                  _PATCHES_BRANCH, _BRAVE_EXPERIMENTS_REMOTE)
        _run_git(checkout, 'branch', '--force', _MAIN_BRANCH, _PATCHES_BRANCH)
        _run_git(checkout, 'push', '--force', _BRAVE_EXPERIMENTS_REMOTE,
                 f'{_MAIN_BRANCH}:refs/heads/{_MAIN_BRANCH}')

    _LOG.info('Done.')


def main() -> int:
    logging.basicConfig(level=logging.INFO, format='%(message)s')

    parser = argparse.ArgumentParser(
        description='Rebase Brave\'s depot_tools patches onto upstream '
        'googlesource main and mirror the result to brave-experiments.')
    parser.add_argument(
        '--clone-dir',
        required=True,
        type=Path,
        help='Directory to clone the depot_tools checkout into.')
    args = parser.parse_args()

    try:
        refresh(args.clone_dir.expanduser().resolve())
    except RebaseError as e:
        _LOG.error(e)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
