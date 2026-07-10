#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""git_cr — brave-core git helper.

Subcommands:
  - commit          Commit with brave-core hook flags.
  - follow-renames  Repair brave-core artefacts after upstream Chromium
                    renames.
  - mv              Move a file/directory in brave-core and repair every
                    downstream artefact.
  - install-hook    Install the commit-msg hook from
                    tools/cr/alias/commit-msg.py.
  - setup-alias     Register the `git cr` alias in the local .git/config.

Getting `git cr`
----------------
  The recommended way is the bootstrap shims, which put a `git-cr` executable
  on $PATH so git resolves the `git cr` subcommand to it in every checkout:
    vpython3 tools/cr/bootstrap/bootstrap.py install

Installing the alias (alternative)
----------------------------------
  If you would rather not modify $PATH, register a per-repository alias.
  From inside the brave-core repository, run once:
    vpython3 tools/cr/alias/cmd.py setup-alias

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
from vpython_utils import VPYTHON3_PATH


def _to_alias_path(p: Path) -> str:
    """Returns `p` in a form suitable for embedding in a git !alias.

    Absolute paths are POSIX-normalised, with Windows drive-letter
    translation (C:/… → /c/…) for git's bundled bash. Relative paths — used
    for bare command names like `vpython3` — are passed through verbatim so
    bash resolves them via $PATH at alias-invocation time.
    """
    if not p.is_absolute():
        return p.as_posix()
    posix = p.as_posix()
    if platform.system() == 'Windows':
        posix = '/' + posix[0].lower() + posix[2:]
    return posix


# This script's path, in POSIX form for embedding in a git alias.
_SCRIPT_PATH: str = _to_alias_path(Path(__file__).resolve())

# `vpython3` invocation for the alias body. `VPYTHON3_PATH` is either a bare
# `Path('vpython3')` (when depot_tools is on PATH) or an absolute path to
# Chromium's bundled `third_party/depot_tools/vpython3` (when it isn't).
_VPYTHON3_PATH: str = _to_alias_path(VPYTHON3_PATH)


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
    # `!`-prefixed aliases run from the work-tree top by default, masking the
    # user's actual cwd. Restore it via $GIT_PREFIX (git's path-from-top hint)
    # so relative paths and `git rev-parse --show-cdup` behave as expected.
    alias_value = (f'!cd "${{GIT_PREFIX:-.}}" && '
                   f'"{_VPYTHON3_PATH}" "{_SCRIPT_PATH}"')
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
