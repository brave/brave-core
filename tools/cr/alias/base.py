# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Shared constants and helpers for git_cr subcommands."""

from __future__ import annotations

import subprocess
from pathlib import Path

import _boot  # noqa: F401
import repository


class UserValidationError(Exception):
    """Raised for user-facing validation failures.

    The message is printed to stderr as-is by the top-level handler.
    Raising this instead of printing-and-returning keeps command functions
    free of error-reporting boilerplate.
    """


# The commit-msg hook script that install-hook symlinks into .git/hooks/.
HOOK_SOURCE: Path = Path(__file__).resolve().parent / 'commit-msg.py'

# Where the hook must live to be picked up by git.
HOOK_DEST: Path = repository.brave.root / '.git' / 'hooks' / 'commit-msg'

# On Windows a bash shim is written instead of a symlink.  cmd_install_hook
# writes this exact content so the hook always delegates to the live source.
WINDOWS_SHIM: bytes = (
    f'#!/bin/sh\nexec python3 "{HOOK_SOURCE}" "$@"\n'.encode('utf-8'))


def check_hooks_path() -> None:
    """Raise UserValidationError if core.hooksPath redirects git away from
    .git/hooks/.

    When core.hooksPath is set to any directory other than .git/hooks/, git
    ignores .git/hooks/ entirely and neither install-hook nor commit will have
    any effect.
    """
    try:
        configured = repository.brave.run_git('config', 'core.hooksPath')
    except subprocess.CalledProcessError:
        return
    if not configured:
        return
    resolved = (Path(configured) if Path(configured).is_absolute() else
                repository.brave.root / configured).resolve()
    if resolved == HOOK_DEST.parent.resolve():
        return  # Points at .git/hooks/ — our hook will still run.
    raise UserValidationError(
        f'git_cr: core.hooksPath is set to "{configured}".\n'
        'git ignores .git/hooks/ while this is set.\n'
        'Unset core.hooksPath or point it to .git/hooks/.')
