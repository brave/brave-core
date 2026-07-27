# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Temporarily apply upstream cherry-picks onto a git checkout for a build.

Build scripts often need upstream fixes that the active checked-out ref
predates. This module packages that "cherry-pick for the build's duration, then
restore HEAD" dance as a reusable context manager (`cherry_picks`) so it can be
driven against any target repository, not just the Chromium `src/` tree.
"""

from __future__ import annotations

import contextlib
import logging
import os
from pathlib import Path
import platform
import shutil
import subprocess

# Fixed git author/committer metadata applied when we cherry-pick onto a
# checkout to make the hash reproducible.
GIT_METADATA_OVERRIDES = {
    'GIT_AUTHOR_NAME': 'Dummy Author',
    'GIT_AUTHOR_EMAIL': 'none@none.com',
    'GIT_AUTHOR_DATE': '2099-01-01 10:10:10',
    'GIT_COMMITTER_NAME': 'Dummy Committer',
    'GIT_COMMITTER_EMAIL': 'none@none.com',
    'GIT_COMMITTER_DATE': '2099-01-01 10:10:10',
}


def _check_call(*command,
                cwd=None,
                env=None,
                check=True,
                capture_output=False):
    """Run *command* as a subprocess, logging the invocation.

    Shared subprocess helper for the toolchain build scripts in this directory
    (`build_rust_toolchain`, `build_xcode_toolchain`, `ephemeral_xcode`), which
    all import it rather than carrying their own copy.

    Logs the full command string at INFO level before executing it.  Stdout
    and stderr are inherited from the parent process (so subprocess output
    streams directly to the terminal) unless `capture_output` is set, in which
    case both are captured on the returned `CompletedProcess` as text.  Callers
    that want the captured stdout read `.stdout` on the result.

    Args:
        *command: The program and its arguments (passed as positional args,
            not as a list).
        cwd: Optional working directory for the subprocess.  Defaults to the
            caller's current working directory when `None`.
        env: Optional environment mapping for the subprocess.  Inherits the
            parent process environment when `None`.
        check: Raise `CalledProcessError` on a non-zero exit when True.
        capture_output: Capture stdout/stderr (as text) instead of inheriting
            the parent's streams.

    Returns:
        The `subprocess.CompletedProcess` for the invocation.  When
        `capture_output` is True, `.stdout`/`.stderr` are decoded strings
        (undecodable bytes are replaced rather than raising).

    Raises:
        subprocess.CalledProcessError: If `check` is True and the process exits
            with a non-zero return code.
        RuntimeError: On Windows, if the command cannot be resolved to an
        executable path.
    """
    logging.info(' >>>> %s', ' '.join(str(a) for a in command))

    if platform.system() == 'Windows':
        # On Windows, resolve the command to an absolute path to avoid issues
        # with bat files not matching the command name (e.g. `gclient` vs
        # `gclient.bat`). This avoids the use of `shell=True`.
        resolved = shutil.which(command[0])
        if resolved is None:
            raise RuntimeError(f'Command not found: {command[0]}')
        if resolved != command[0]:
            command = [resolved] + list(command[1:])

    return subprocess.run(command,
                          cwd=cwd,
                          env=env,
                          check=check,
                          capture_output=capture_output,
                          text=True,
                          errors='replace')


@contextlib.contextmanager
def cherry_picks(repo_dir: str | Path,
                 commits: list[str],
                 metadata_overrides: dict[str, str] | None = None):
    """Context manager: apply upstream cherry-picks for the build's duration.

    Ensures every commit in *commits* is in *repo_dir*'s history while the build
    runs, then restores the original HEAD afterwards. Used to pull in upstream
    fixes the checked-out ref may predate — for example the commit that pins the
    git author/committer metadata `tools/clang/scripts/build.py` stamps onto its
    LLVM cherry-picks so a derived toolchain hash is reproducible.

    Args:
        repo_dir: Path to the git checkout the cherry-picks are applied onto.
        commits: Commit-ish list to ensure is present in the checkout's history.
        metadata_overrides: Environment overrides pinning the git
            author/committer identity for the cherry-pick commits, so the
            resulting HEAD is deterministic regardless of local git config.
            Defaults to `GIT_METADATA_OVERRIDES` when `None`.

    Protocol:
    1. For each commit, fetch it if its object is missing (the ref may have
       branched before it landed), then skip it if it is already reachable
       from HEAD.
    2. If nothing needs applying, `yield` immediately and leave the checkout
       untouched — there is nothing to clean up.
    3. Otherwise cherry-pick the remaining commits — with `metadata_overrides`
       and `--no-gpg-sign` so the resulting HEAD is deterministic — `yield`, and
       unconditionally restore the checkout to its original HEAD in a `finally`
       block. Build outputs are untracked, so the reset leaves them in place.
    """
    if metadata_overrides is None:
        metadata_overrides = GIT_METADATA_OVERRIDES

    def _run_git(*args,
                 check: bool = True,
                 capture_output: bool = False,
                 env: dict | None = None) -> subprocess.CompletedProcess:
        """Run `git -C <repo_dir> <args...>` via `_check_call`."""
        return _check_call('git',
                           '-C',
                           str(repo_dir),
                           *args,
                           check=check,
                           capture_output=capture_output,
                           env=env)

    to_apply = []
    for commit in commits:
        object_present = _run_git('cat-file',
                                  '-e',
                                  f'{commit}^{{commit}}',
                                  check=False,
                                  capture_output=True).returncode == 0
        if not object_present:
            logging.info('Fetching cherry-pick %s.', commit)
            _run_git('fetch', 'origin', commit)

        # This tests commit *reachability*, not whether the change is still
        # effectively present. A commit that landed and was later reverted
        # upstream is still an ancestor, so it is treated as applied and not
        # re-picked. Acceptable for this curated list of upstream fixes;
        # revisit if a pick can legitimately be reverted-then-wanted.
        already_applied = _run_git('merge-base',
                                   '--is-ancestor',
                                   commit,
                                   'HEAD',
                                   check=False,
                                   capture_output=True).returncode == 0
        if already_applied:
            logging.info('Cherry-pick %s already present; skipping.', commit)
        else:
            to_apply.append(commit)

    if not to_apply:
        yield
        return

    original_head = _run_git('rev-parse', 'HEAD',
                             capture_output=True).stdout.strip()
    logging.info('Cherry-picking %s.', ', '.join(to_apply))
    _run_git('cherry-pick',
             '--keep-redundant-commits',
             '--no-gpg-sign',
             *to_apply,
             env={
                 **os.environ,
                 **metadata_overrides
             })
    try:
        yield
    finally:
        logging.info('Restoring checkout to %s after cherry-picks.',
                     original_head)
        _run_git('reset', '--hard', original_head)
