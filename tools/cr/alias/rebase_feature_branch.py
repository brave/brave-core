# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""git_cr rebase-feature-branch — rebases with brockit's conflict answers.

The branch is rebased from its merge-base with the branch it is rebased onto,
as `git rebase` does, and the conflicts `brockit rebase` has a fixed answer
for are resolved as the rebase goes.

Usage
-----
  git cr rebase-feature-branch [--to-ref REF]
  git cr rebase-feature-branch --continue
  git cr rebase-feature-branch --skip
  git cr rebase-feature-branch --dry-run [--to-ref REF]
"""

from __future__ import annotations

import argparse
import subprocess

import _boot  # noqa: F401
import repository
from alias.base import UserValidationError


def _resolve_to_ref(to_ref: str | None) -> str:
    """The branch to rebase onto: `to_ref`, or the current branch's
    upstream."""
    if to_ref is not None:
        return to_ref
    try:
        return repository.brave.run_git(
            'rev-parse', '--abbrev-ref', '--symbolic-full-name', '@{upstream}'
        )
    except subprocess.CalledProcessError as e:
        raise UserValidationError(
            'git_cr: the current branch has no upstream. Pass --to-ref, or '
            'set one with `git branch --set-upstream-to`.'
        ) from e


def cmd_rebase_feature_branch(args: list[str]) -> int:
    """Parse rebase arguments and run brockit's rebase."""
    parser = argparse.ArgumentParser(
        prog='git cr rebase-feature-branch',
        description='Rebase a feature branch onto --to-ref, resolving the '
        'conflicts brockit has a fixed answer for.',
    )
    action = parser.add_mutually_exclusive_group()
    action.add_argument(
        '--continue',
        action='store_true',
        dest='is_continuation',
        help='Continue the rebase once the conflicts it stopped on are '
        'resolved and staged',
    )
    action.add_argument(
        '--skip',
        action='store_true',
        help='Skip the commit the rebase stopped on, and continue',
    )
    action.add_argument(
        '--dry-run',
        action='store_true',
        help='Rebase nothing, and exit with an error if a rebase is needed',
    )
    parser.add_argument(
        '--to-ref',
        help="The branch to rebase onto. Defaults to the current branch's "
        'upstream.',
    )
    parsed = parser.parse_args(args)
    if parsed.to_ref is not None and (parsed.is_continuation or parsed.skip):
        parser.error('--to-ref cannot be combined with --continue or --skip')

    # Imported lazily: brockit is a heavy module.
    import brockit

    try:
        if parsed.is_continuation or parsed.skip:
            brockit.Rebase().run(
                is_continuation=parsed.is_continuation, skip=parsed.skip
            )
            return 0
        to_ref = _resolve_to_ref(parsed.to_ref)
        if parsed.dry_run:
            brockit.Rebase().run(to_ref=to_ref, dry_run=True)
            return 0
        merge_base = repository.brave.run_git('merge-base', to_ref, 'HEAD')
        brockit.Rebase().run(from_ref=merge_base, to_ref=to_ref)
    except brockit.RebaseNeededException:
        return brockit.RebaseNeededException.EXIT_CODE
    except (
        brockit.InvalidInputException,
        brockit.BadOutcomeException,
        brockit.ActionNeededException,
    ):
        return 1
    return 0
