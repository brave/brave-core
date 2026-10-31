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
    --force-commit-hook
                Runs the hook where it would bail out → $force_commit_hook

  The hook leaves the message alone during a rebase or cherry-pick, and for
  `fixup!`/`amend!` commits. Where that would drop what was asked for, the
  commit is refused unless `--force-commit-hook` is given.

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
from alias.base import (
    HOOK_DEST,
    WINDOWS_SHIM,
    UserValidationError,
    check_hooks_path,
)


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
            'Run:    git cr install-hook'
        )


@dataclass(frozen=True)
class _HookBailout:
    """Where the commit-msg hook leaves the message alone, and so would drop
    what the commit asks of it."""

    # The git state files of an operation the hook leaves alone.
    _OPERATIONS: ClassVar[dict[str, str]] = {
        'rebase-merge': 'a rebase is in progress',
        'rebase-apply': 'a rebase is in progress',
        'CHERRY_PICK_HEAD': 'a cherry-pick is in progress',
    }
    _SQUASH_MARKERS: ClassVar[tuple[str, ...]] = ('fixup!', 'amend!')

    reason: str

    @staticmethod
    def find(git_args: list[str], asks_hook: bool) -> _HookBailout | None:
        """The bail-out this commit would hit, or None.

        `asks_hook` is whether the commit passes tags, issues or culprits,
        which a `fixup!`/`amend!` commit would drop; ordinary fixups are left
        alone, since skipping them is what the hook is for.
        """
        for state, reason in _HookBailout._OPERATIONS.items():
            path = subprocess.run(
                ['git', 'rev-parse', '--git-path', state],
                capture_output=True,
                text=True,
                check=False,
            ).stdout.strip()
            if path and os.path.exists(path):
                return _HookBailout(reason)
        if asks_hook and _HookBailout._is_squash_marker(git_args):
            return _HookBailout('the commit is a fixup!/amend! commit')
        return None

    @staticmethod
    def _is_squash_marker(git_args: list[str]) -> bool:
        parser = _LenientArgumentParser(add_help=False)
        parser.add_argument('--fixup')
        parser.add_argument('--squash')
        parser.add_argument('-m', '--message', action='append', default=[])
        try:
            parsed, _ = parser.parse_known_args(git_args)
        except ValueError:
            return False
        if parsed.fixup or parsed.squash:
            return True
        return bool(parsed.message) and parsed.message[0].startswith(
            _HookBailout._SQUASH_MARKERS
        )

    def error(self) -> UserValidationError:
        return UserValidationError(
            f'git_cr: {self.reason}, so the commit-msg hook would leave the '
            'message alone.\n'
            'Pass --force-commit-hook to run it anyway.'
        )


def _run_commit(
    tagged: str | None,
    issue: str | None,
    culprit: str | None,
    git_args: list[str],
    force_hook: bool = False,
) -> int:
    """Execute git commit with hook environment variables injected."""
    _check_hook_ready()
    if (
        not force_hook
        and '--no-verify' not in git_args
        and '-n' not in git_args
    ):
        asks_hook = bool(tagged or issue or culprit)
        if bailout := _HookBailout.find(git_args, asks_hook):
            raise bailout.error()
    env = os.environ.copy()
    if force_hook:
        env['force_commit_hook'] = '1'
    if tagged:
        env['tags'] = tagged
    if issue:
        env['issue'] = issue
    if culprit:
        env['culprit'] = culprit
    return subprocess.run(
        ['git', 'commit'] + git_args, check=False, env=env
    ).returncode


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
                f'other arguments; remove: {" ".join(other_args)}'
            )
        if not target:
            raise UserValidationError(
                f'git_cr: --fixup={verb}: requires a commit reference, '
                f'e.g. --fixup={verb}:HEAD~2'
            )
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
        except (
            brockit.InvalidInputException,
            brockit.BadOutcomeException,
            brockit.ActionNeededException,
        ):
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
    parser.add_argument(
        '--force-commit-hook',
        action='store_true',
        help='Run the commit-msg hook where it would leave the message alone: '
        'during a rebase or cherry-pick, and for fixup!/amend! commits',
    )
    parsed, extra = parser.parse_known_args(args)
    return _run_commit(
        parsed.tagged,
        parsed.issue,
        parsed.culprit,
        extra,
        force_hook=parsed.force_commit_hook,
    )
