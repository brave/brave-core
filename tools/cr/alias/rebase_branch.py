# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""git_cr rebase — rebases a feature branch with brockit's conflict answers.

The branch is rebased from its merge-base with the branch it is rebased onto,
as `git rebase` does, and the conflicts `brockit rebase` has a fixed answer
for are resolved as the rebase goes. After each commit is applied, its plaster
patches are refreshed against the Chromium checkout with
`git cr amend-plaster-patches`.

Usage
-----
  git cr rebase [--to-ref REF]
  git cr rebase --continue
  git cr rebase --skip
  git cr rebase --dry-run [--to-ref REF]
"""

from __future__ import annotations

import argparse
import subprocess
import sys

import _boot  # noqa: F401
import repository
from alias.base import UserValidationError
from versioning import PACKAGE_FILE, Version, read_chromium_version_file


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


def _unsynced_chrome_tag(onto: str) -> Version | None:
    """The chrome tag of `onto`, when Chromium is synced to another one."""
    wanted = Version.from_git(onto)
    if read_chromium_version_file() != wanted:
        return wanted
    return None


def _require_chromium_synced(onto: str) -> None:
    wanted = _unsynced_chrome_tag(onto)
    if wanted is not None:
        raise UserValidationError(
            f'git_cr: Chromium is at {read_chromium_version_file()}, but the '
            f'branch is rebased onto Chromium {wanted}. Sync Chromium to '
            f'{wanted} first.'
        )


def _require_no_version_bumps(merge_base: str) -> None:
    """Refuses a branch with a commit of its own that changes the chrome tag,
    as Chromium stays synced to one version for the whole rebase."""
    for commit in repository.brave.run_git(
        'rev-list', f'{merge_base}..HEAD', '--', PACKAGE_FILE
    ).split():
        if Version.from_git(commit) != Version.from_git(f'{commit}~1'):
            subject = repository.brave.get_commit_short_description(commit)
            raise UserValidationError(
                f'git_cr: {commit[:12]} ({subject}) changes the Chromium '
                'version. `git cr rebase` only rebases branches that keep the '
                'version of the branch they are rebased onto.'
            )


def cmd_rebase(args: list[str]) -> int:
    """Parse rebase arguments and run brockit's rebase."""
    parser = argparse.ArgumentParser(
        prog='git cr rebase',
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

    rebase = brockit.Rebase(resume_command='git cr rebase')
    to_ref = None
    try:
        if parsed.is_continuation or parsed.skip:
            if repository.brave.is_rebase_in_progress():
                onto = repository.brave.rebase_merge_dir() / 'onto'
                _require_chromium_synced(onto.read_text().strip())
            rebase.run(is_continuation=parsed.is_continuation, skip=parsed.skip)
            return 0
        to_ref = _resolve_to_ref(parsed.to_ref)
        merge_base = repository.brave.run_git('merge-base', to_ref, 'HEAD')
        _require_no_version_bumps(merge_base)
        if parsed.dry_run:
            rebase.run(to_ref=to_ref, dry_run=True)
            return 0
        _require_chromium_synced(to_ref)
        rebase.run(
            from_ref=merge_base,
            to_ref=to_ref,
            exec_command='git cr amend-plaster-patches',
        )
    except brockit.RebaseNeededException:
        wanted = _unsynced_chrome_tag(to_ref)
        if wanted is not None:
            print(
                f'git_cr: a rebase is needed, but sync Chromium to {wanted} '
                'first.',
                file=sys.stderr,
            )
        return brockit.RebaseNeededException.EXIT_CODE
    except (
        brockit.InvalidInputException,
        brockit.BadOutcomeException,
        brockit.ActionNeededException,
    ):
        return 1
    return 0
