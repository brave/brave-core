# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""git_cr commit — wraps `git commit` with brave-core hook environment flags.

Forwards user-specified tags, issues, and culprit commit hashes to the
commit-msg hook via environment variables. All other arguments pass through
to `git commit` unchanged.

Usage
-----
  git cr commit [--tagged TAG[,TAG…]] [--issue NUM[,NUM…]]
                [--culprit HASH[,HASH…]] [<git-commit-args> …]

  git cr commit -m "Fix login button alignment"
  git cr commit --tagged WIP -m "Work in progress"

  Custom flags (forwarded to the commit-msg hook via environment variables):
    --tagged    Comma-separated tags            → $tags
    --issue     Comma-separated issue numbers   → $issue
    --culprit   Comma-separated commit hashes   → $culprit

  All other arguments are forwarded verbatim to git commit.
"""

from __future__ import annotations

import argparse
import os
import platform
import stat
import subprocess

import _boot  # noqa: F401
from alias.base import (HOOK_DEST, WINDOWS_SHIM, UserValidationError,
                        check_hooks_path)


def _check_hook_ready() -> None:
    """Raise UserValidationError if the commit-msg hook is not ready to run.

    Checks core.hooksPath first, then verifies the hook file exists and is
    executable (POSIX) or contains the expected shim content (Windows).
    """
    check_hooks_path()
    ready = False
    if HOOK_DEST.exists():
        if platform.system() == 'Windows':
            try:
                ready = HOOK_DEST.read_bytes() == WINDOWS_SHIM
            except OSError:
                pass
        else:
            ready = bool(HOOK_DEST.stat().st_mode & stat.S_IXUSR)
    if not ready:
        raise UserValidationError(
            'git_cr: commit-msg hook is not installed or not executable.\n'
            'Run:    git cr install-hook')


def _run_commit(
    tagged: str | None,
    issue: str | None,
    culprit: str | None,
    git_args: list[str],
) -> int:
    """Execute git commit with hook environment variables injected."""
    _check_hook_ready()
    env = os.environ.copy()
    if tagged:
        env['tags'] = tagged
    if issue:
        env['issue'] = issue
    if culprit:
        env['culprit'] = culprit
    return subprocess.run(['git', 'commit'] + git_args, check=False,
                          env=env).returncode


def cmd_commit(args: list[str]) -> int:
    """Parse commit flags and invoke git commit with injected environment."""
    parser = argparse.ArgumentParser(
        prog='git cr commit',
        description='git commit wrapper with brave-core hook flags',
        epilog='All other arguments are forwarded verbatim to git commit.',
    )
    parser.add_argument(
        '--tagged',
        metavar='TAG[,TAG...]',
        help='Comma-separated tags injected as $tags for the commit-msg hook',
    )
    parser.add_argument(
        '--issue',
        metavar='NUM[,NUM...]',
        help='GitHub issue numbers injected as $issue',
    )
    parser.add_argument(
        '--culprit',
        metavar='HASH[,HASH...]',
        help='Chromium commit hashes injected as $culprit',
    )
    parsed, extra = parser.parse_known_args(args)
    return _run_commit(parsed.tagged, parsed.issue, parsed.culprit, extra)
