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
  git cr commit --fixup=reassign:<ref>   # shortcut for `brockit.py reassign`
  git cr commit --fixup=drop:<ref>       # shortcut for `brockit.py drop`

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
from dataclasses import dataclass
from typing import ClassVar, NoReturn

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


class _LenientArgumentParser(argparse.ArgumentParser):
    """An `ArgumentParser` that raises instead of terminating on error.

    This class is useful when parsing shortcuts, as we just bail out when the
    arguments don't match.
    """

    def error(self, message: str) -> NoReturn:
        raise ValueError(message)


@dataclass(frozen=True)
class _MarkChangeShortcut:
    """The `git cr commit --fixup=<verb>:<ref>` shortcut.

    This is a convenience shortcut for brockit's change-marking tasks. Each
    supported `<verb>` maps to a brockit task that drops an empty marker commit
    (e.g. `reassign!`/`drop!`) which brockit's `rebase` command later acts on.
    It can be used either as:
      git cr commit --fixup=reassign:HEAD
      git cr commit --fixup=drop:HEAD

    or with the split spelling:
      git cr commit --fixup reassign:HEAD

    `from_args` is the entry point: it returns a ready-to-run instance when
    the shortcut is present and valid, or None when the args are an ordinary
    commit.
    """

    # Maps each supported `<verb>:` value -- mirroring git's own `amend:` /
    # `reword:` fixup modes -- to the name of the brockit task that implements
    # it. The task is resolved lazily in `run` so brockit stays unimported on
    # the ordinary commit path.
    _TASKS: ClassVar[dict[str, str]] = {
        'reassign': 'Reassign',
        'drop': 'Drop',
    }

    # The verb selecting the brockit task, e.g. `reassign` or `drop`.
    verb: str
    # The commit reference being marked.
    target: str

    @staticmethod
    def from_args(args: list[str]) -> _MarkChangeShortcut | None:
        """Build the shortcut from *args*, or return None when it isn't used.

        Parses *args* strictly looking only for `--fixup`, then validates its
        value. Anything that isn't one of our `<verb>:<ref>` shortcuts -- an
        ordinary commit, a malformed `--fixup`, or git's own fixup modes
        (`amend:` / `reword:`) -- silently bails out as None so the caller falls
        back to the normal commit path.
        """
        parser = _LenientArgumentParser(add_help=False)
        parser.add_argument('--fixup')
        try:
            parsed, other_args = parser.parse_known_args(args)
        except ValueError:
            return None

        fixup = parsed.fixup
        if fixup is None or ':' not in fixup:
            return None
        verb, target = fixup.split(':', 1)
        # Leave git's own fixup modes (and anything unrecognised) untouched.
        if verb not in _MarkChangeShortcut._TASKS:
            return None
        if other_args:
            raise UserValidationError(
                f'git_cr: --fixup={verb}:<ref> cannot be combined with '
                f'other arguments; remove: {" ".join(other_args)}')
        if not target:
            raise UserValidationError(
                f'git_cr: --fixup={verb}: requires a commit reference, '
                f'e.g. --fixup={verb}:HEAD~2')
        return _MarkChangeShortcut(verb, target)

    def run(self) -> int:
        """Run the selected brockit change-marking task for the target.

        The commit-msg hook is irrelevant (brockit commits with --no-verify),
        so it is not checked.
        """
        # Imported lazily: brockit is a heavy module, so we only import it when
        # needed.
        import brockit
        task = getattr(brockit, _MarkChangeShortcut._TASKS[self.verb])
        try:
            task().run(change=self.target)
        except (brockit.InvalidInputException, brockit.BadOutcomeException,
                brockit.ActionNeededException):
            # Handling brockit exceptions as failures to create the marker
            # commit.
            return 1
        return 0


def cmd_commit(args: list[str]) -> int:
    """Parse commit flags and invoke git commit with injected environment."""
    # Let's try first to check for a change-marking shortcut being used.
    mark_change = _MarkChangeShortcut.from_args(args)
    if mark_change is not None:
        return mark_change.run()

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
