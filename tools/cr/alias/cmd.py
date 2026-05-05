#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""git_cr — brave-core git helper.

Subcommands
-----------
  git cr commit       Commit with brave-core hook flags
  git cr mv           Move a file/directory and repair all artefacts
  git cr follow-renames
                      Repair artefacts after upstream Chromium renames
  git cr install-hook Install commit-msg hook from tools/cr/alias/commit-msg.py
  git cr setup-alias  Register the git cr alias in .git/config

commit
------
  git cr commit [--tagged TAG[,TAG…]] [--issue NUM[,NUM…]]
                [--culprit HASH[,HASH…]] [<git-commit-args> …]

  git cr commit -m "Fix login button alignment"
  git cr commit --tagged WIP -m "Work in progress"

  Custom flags (forwarded to the commit-msg hook via environment variables):
    --tagged    Comma-separated tags            → $tags
    --issue     Comma-separated issue numbers   → $issue
    --culprit   Comma-separated commit hashes   → $culprit

  All other arguments are forwarded verbatim to git commit.

mv
--
  git cr mv [--mkdir] [--no-git] <source> <destination>

  git cr mv foo/bar.h baz/bar.h
  git cr mv --mkdir foo/bar.h new_dir/bar.h

follow-renames
--------------
  git cr follow-renames [--no-git] <rev-range>

  # All renames between two Chromium version tags (typical version bump):
  git cr follow-renames 130.0.6723.58..131.0.6778.85

  # Renames introduced by a single upstream commit:
  git cr follow-renames abc123^..abc123

Installing the alias
--------------------
  From inside the brave-core repository, run once:
    python3 tools/cr/alias/cmd.py setup-alias
  This writes a 'cr' entry to .git/config so that 'git cr' works immediately
  in the current repository without modifying any shell config files.

Installing the hook
-------------------
  From inside the brave-core repository, run once:
    git cr install-hook
  This creates a symlink so that future updates to commit-msg.py are
  reflected automatically without re-running this command.
"""

from __future__ import annotations

import argparse
import os
import platform
import stat
import subprocess
import sys
from pathlib import Path

import _boot  # noqa: F401
from user_validation_error import UserValidationError

# tools/cr/ directory (parent of alias/).
_REBASE_TOOLS_DIR: Path = Path(__file__).resolve().parent.parent

# brave-core repository root, two levels above tools/cr/.
_BRAVE_CORE_ROOT: Path = _REBASE_TOOLS_DIR.parent.parent

# The commit-msg hook script that install-hook symlinks into .git/hooks/.
_HOOK_SOURCE: Path = Path(__file__).resolve().parent / 'commit-msg.py'

# Where the hook must live to be picked up by git.
_HOOK_DEST: Path = _BRAVE_CORE_ROOT / '.git' / 'hooks' / 'commit-msg'

# This script's path in POSIX form, suitable for embedding in a git alias.
# Git aliases prefixed with '!' run via git's bundled bash on all platforms,
# including Git for Windows, which requires POSIX paths (C:/… → /c/…).
_SCRIPT_PATH: str = Path(__file__).resolve().as_posix()
if platform.system() == 'Windows':
    _SCRIPT_PATH = '/' + _SCRIPT_PATH[0].lower() + _SCRIPT_PATH[2:]
    # On Windows a bash shim is written instead of a symlink.  cmd_install_hook
    # writes this exact content so the hook always delegates to the live source.
    _WINDOWS_SHIM: bytes = (
        f'#!/bin/sh\nexec python3 "{_HOOK_SOURCE}" "$@"\n'.encode('utf-8'))


def _check_hooks_path() -> None:
    """Raise _UserError if core.hooksPath redirects git away from .git/hooks/.

    When core.hooksPath is set to any directory other than .git/hooks/, git
    ignores .git/hooks/ entirely and neither install-hook nor commit will have
    any effect.
    """
    result = subprocess.run(
        ['git', 'config', 'core.hooksPath'],
        capture_output=True,
        text=True,
        check=False,
        cwd=_BRAVE_CORE_ROOT,
    )
    if result.returncode != 0 or not result.stdout.strip():
        return
    configured = result.stdout.strip()
    resolved = (Path(configured) if Path(configured).is_absolute() else
                _BRAVE_CORE_ROOT / configured).resolve()
    if resolved == _HOOK_DEST.parent.resolve():
        return  # Points at .git/hooks/ — our hook will still run.
    raise UserValidationError(
        f'git_cr: core.hooksPath is set to "{configured}".\n'
        'git ignores .git/hooks/ while this is set.\n'
        'Unset core.hooksPath or point it to .git/hooks/.')


def _check_hook_ready() -> None:
    """Raise _UserError if the commit-msg hook is not ready to run.

    Checks core.hooksPath first, then verifies the hook file exists and is
    executable (POSIX) or contains the expected shim content (Windows).
    """
    _check_hooks_path()
    ready = False
    if _HOOK_DEST.exists():
        if platform.system() == 'Windows':
            try:
                ready = _HOOK_DEST.read_bytes() == _WINDOWS_SHIM
            except OSError:
                pass
        else:
            ready = bool(_HOOK_DEST.stat().st_mode & stat.S_IXUSR)
    if not ready:
        raise UserValidationError(
            'git_cr: commit-msg hook is not installed or not executable.\n'
            'Run:    git cr install-hook')


def _build_commit_parser() -> argparse.ArgumentParser:
    """Return the argument parser for the commit subcommand's own flags."""
    p = argparse.ArgumentParser(
        prog='git cr commit',
        description='git commit wrapper with brave-core hook flags',
        epilog='All other arguments are forwarded verbatim to git commit.',
    )
    p.add_argument(
        '--tagged',
        metavar='TAG[,TAG...]',
        help='Comma-separated tags injected as $tags for the commit-msg hook',
    )
    p.add_argument(
        '--issue',
        metavar='NUM[,NUM...]',
        help='GitHub issue numbers injected as $issue',
    )
    p.add_argument(
        '--culprit',
        metavar='HASH[,HASH...]',
        help='Chromium commit hashes injected as $culprit',
    )
    return p


def _run_commit(
    tagged: str | None,
    issue: str | None,
    culprit: str | None,
    git_args: list[str],
) -> int:
    """Execute git commit with hook environment variables injected."""
    env = os.environ.copy()
    if tagged:
        env['tags'] = tagged
    if issue:
        env['issue'] = issue
    if culprit:
        env['culprit'] = culprit
    return subprocess.run(['git', 'commit'] + git_args, check=False,
                          env=env).returncode


def cmd_install_hook() -> int:
    """Install the commit-msg hook from the source tree.

    On POSIX a symlink is created so that edits to commit-msg.py take effect
    immediately without re-running this command.  On Windows, where symlinks
    require elevated privileges, a small #!/bin/sh shim is written instead;
    Git for Windows uses its bundled bash to run hooks regardless of the host
    OS, so a shebang script works correctly.
    """
    _check_hooks_path()
    _HOOK_DEST.parent.mkdir(parents=True, exist_ok=True)

    if _HOOK_DEST.exists() or _HOOK_DEST.is_symlink():
        _HOOK_DEST.unlink()

    if platform.system() == 'Windows':
        _HOOK_DEST.write_bytes(_WINDOWS_SHIM)
        _HOOK_DEST.chmod(_HOOK_DEST.stat().st_mode | stat.S_IXUSR
                         | stat.S_IXGRP
                         | stat.S_IXOTH)
    else:
        _HOOK_DEST.symlink_to(_HOOK_SOURCE)
        _HOOK_SOURCE.chmod(_HOOK_SOURCE.stat().st_mode | stat.S_IXUSR
                           | stat.S_IXGRP | stat.S_IXOTH)

    print(f'Installed: {_HOOK_DEST} → {_HOOK_SOURCE}')
    return 0


def cmd_setup_alias() -> int:
    """Register 'git cr' as a local git alias pointing to this script.

    Writes to .git/config so no shell config files are touched.  The alias
    works on Linux, macOS, and Windows (Git for Windows) without any extra
    setup steps.
    """
    alias_value = f'!python3 "{_SCRIPT_PATH}"'
    result = subprocess.run(
        ['git', 'config', '--local', 'alias.cr', alias_value],
        capture_output=True,
        text=True,
        check=False,
        cwd=_BRAVE_CORE_ROOT,
    )
    if result.returncode != 0:
        raise UserValidationError(f'git_cr: {result.stderr.strip()}')
    print('Installed: git alias.cr')
    print('Run "git cr" for usage.')
    return 0


def cmd_commit(args: list[str]) -> int:
    """Parse commit flags and invoke git commit with injected environment."""
    _check_hook_ready()
    parser = _build_commit_parser()
    parsed, extra = parser.parse_known_args(args)
    return _run_commit(parsed.tagged, parsed.issue, parsed.culprit, extra)


def main() -> int:
    """Dispatch to the requested subcommand."""
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return 0

    try:
        subcmd, rest = args[0], args[1:]
        if subcmd == 'commit':
            return cmd_commit(rest)
        if subcmd == 'install-hook':
            return cmd_install_hook()
        if subcmd == 'setup-alias':
            return cmd_setup_alias()
        if subcmd == 'mv':
            import alias.mv
            return alias.mv.cmd_mv(rest)
        if subcmd == 'follow-renames':
            import alias.follow_renames
            return alias.follow_renames.cmd_follow_renames(rest)
    except UserValidationError as e:
        print(e, file=sys.stderr)
        return 1

    print(f"git_cr: unknown subcommand '{subcmd}'", file=sys.stderr)
    print(__doc__, file=sys.stderr)
    return 1


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
