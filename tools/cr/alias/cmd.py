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
    vpython3 tools/cr/alias/cmd.py setup-alias
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

import platform
import stat
import subprocess
import sys
from pathlib import Path

import _boot  # noqa: F401
import repository
from alias.base import (HOOK_DEST, HOOK_SOURCE, WINDOWS_SHIM,
                        UserValidationError, check_hooks_path)
from alias.commit import cmd_commit

# This script's path in POSIX form, suitable for embedding in a git alias.
# Git aliases prefixed with '!' run via git's bundled bash on all platforms,
# including Git for Windows, which requires POSIX paths (C:/… → /c/…).
_SCRIPT_PATH: str = Path(__file__).resolve().as_posix()
if platform.system() == 'Windows':
    _SCRIPT_PATH = '/' + _SCRIPT_PATH[0].lower() + _SCRIPT_PATH[2:]


def cmd_install_hook() -> int:
    """Install the commit-msg hook from the source tree.

    On POSIX a symlink is created so that edits to commit-msg.py take effect
    immediately without re-running this command.  On Windows, where symlinks
    require elevated privileges, a small #!/bin/sh shim is written instead;
    Git for Windows uses its bundled bash to run hooks regardless of the host
    OS, so a shebang script works correctly.
    """
    check_hooks_path()
    HOOK_DEST.parent.mkdir(parents=True, exist_ok=True)

    if HOOK_DEST.exists() or HOOK_DEST.is_symlink():
        HOOK_DEST.unlink()

    if platform.system() == 'Windows':
        HOOK_DEST.write_bytes(WINDOWS_SHIM)
        HOOK_DEST.chmod(HOOK_DEST.stat().st_mode | stat.S_IXUSR
                        | stat.S_IXGRP
                        | stat.S_IXOTH)
    else:
        HOOK_DEST.symlink_to(HOOK_SOURCE)
        HOOK_SOURCE.chmod(HOOK_SOURCE.stat().st_mode | stat.S_IXUSR
                          | stat.S_IXGRP | stat.S_IXOTH)

    print(f'Installed: {HOOK_DEST} → {HOOK_SOURCE}')
    return 0


def cmd_setup_alias() -> int:
    """Register 'git cr' as a local git alias pointing to this script.

    Writes to .git/config so no shell config files are touched.  The alias
    works on Linux, macOS, and Windows (Git for Windows) without any extra
    setup steps.
    """
    alias_value = f'!vpython3 "{_SCRIPT_PATH}"'
    try:
        repository.brave.run_git('config', '--local', 'alias.cr', alias_value)
    except subprocess.CalledProcessError as e:
        raise UserValidationError(f'git_cr: {e.stderr.strip()}') from e
    print('Installed: git alias.cr')
    print('Run "git cr" for usage.')
    return 0


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
