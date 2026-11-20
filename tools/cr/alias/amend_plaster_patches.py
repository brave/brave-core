# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""git_cr amend-plaster-patches — re-applies the plasters HEAD adds or
modifies, and amends their patches into HEAD.

Usage
-----
  git cr amend-plaster-patches
"""

from __future__ import annotations

import argparse
from pathlib import Path

import _boot  # noqa: F401
import repository
from alias.base import UserValidationError


def cmd_amend_plaster_patches(args: list[str]) -> int:
    """Re-apply HEAD's plasters, and amend their patches into HEAD."""
    argparse.ArgumentParser(
        prog='git cr amend-plaster-patches',
        description='Re-apply the plasters HEAD adds or modifies, and amend '
        'their patches into HEAD.',
    ).parse_args(args)

    # Imported lazily: plaster is a heavy module.
    import plaster

    changed = repository.brave.run_git(
        'diff-tree',
        '--no-commit-id',
        '--name-only',
        '--no-renames',
        '--diff-filter=AM',
        '-r',
        'HEAD',
        '--',
        f'{plaster.PLASTER_FILES_PATH.name}/',
    ).splitlines()
    patches: list[Path] = []
    for path in changed:
        if not path.endswith(plaster.PLASTER_EXTENSION):
            continue
        plaster_file = repository.brave.root / path
        try:
            plaster.PlasterFile(plaster_file).apply()
        except (plaster.PlasterError, ValueError) as e:
            raise UserValidationError(f'git_cr: {e}') from e
        target = plaster.PlasterTarget.resolve(plaster_file)
        patches += [target.patch.resolve(), target.patchinfo.resolve()]

    if not patches:
        return 0
    repository.brave.run_git('add', '--', *patches)
    if repository.brave.run_git('diff', '--cached', '--name-only'):
        repository.brave.run_git(
            'commit', '--amend', '--no-edit', '--no-verify'
        )
    return 0
