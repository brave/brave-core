# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Uploads a `cr<N>` branch to Gerrit as a relation chain of CLs.

The upload is mechanical, with no agent involved, once a run has published
the branch to GitHub. It works from `origin`'s `cr<N>` and the base `publish`
left as its upstream, as the checkout has them, and changes nothing in it:

1. Checks that every commit of the chain (the fork point with the base to
   `cr<N>`) has exactly one Change-Id, and that no two share one.
2. Brings Gerrit's copy of the base up to date, when the base isn't a `cr<N>`
   branch: one is a chain of its own on Gerrit, whose branch stays at its own
   fork point. A base Gerrit has diverged from is left alone, and the upload
   stops.
3. Holds Gerrit's `cr<N>` at the fork point, which carries the chain: with the
   chain's commits on the branch, the push would find no new changes.
4. Restores the abandoned CLs whose Change-Id is back in the chain, as Gerrit
   takes no new patch sets on them.
5. Pushes the chain to `refs/for/cr<N>`, as new patch sets of the CLs its
   Change-Ids name.
6. Abandons the open CLs whose Change-Id left the chain: commits dropped from
   `cr<N>` at some point of a run.
"""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass
import json
from pathlib import Path
import re
import shlex
import subprocess
from urllib.parse import urlsplit

DEFAULT_URL = (
    'ssh://autolift-bot@gerrit-ssh.brave.com:29418/brave-browser/brave-core'
)

_CR_BRANCH = re.compile(r'cr\d+')


@dataclass(frozen=True)
class ChainCommit:
    sha: str
    subject: str
    change_ids: tuple[str, ...]


@dataclass(frozen=True)
class Change:
    """An open or abandoned CL of the branch on Gerrit."""

    change_id: str
    number: int
    patch_set: int
    status: str

    @property
    def revision(self) -> str:
        """The `<change>,<patch set>` that `gerrit review` takes."""
        return f'{self.number},{self.patch_set}'


def _git(cwd: Path, *args: str) -> str:
    return subprocess.run(
        ['git', '-C', str(cwd), *args],
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()


def chain_commits(cwd: Path, fork: str, head: str) -> list[ChainCommit]:
    """The commits from `fork` to `head`, oldest first, with the Change-Ids
    of their trailers, as git parses them: quoted ones aren't trailers."""
    log = _git(
        cwd,
        'log',
        '--reverse',
        '--format=%H%x00%s%x00'
        '%(trailers:key=Change-Id,valueonly,separator=%x2C)%x1e',
        f'{fork}..{head}',
    )
    commits = []
    for record in filter(None, (r.strip() for r in log.split('\x1e'))):
        sha, subject, change_ids = record.split('\x00')
        commits.append(
            ChainCommit(
                sha, subject, tuple(filter(None, change_ids.split(',')))
            )
        )
    return commits


def chain_problems(commits: list[ChainCommit]) -> list[str]:
    """Why the chain can't go to Gerrit, if it can't: a commit without a
    Change-Id, with several, or with one another commit has too."""
    problems = []
    seen: dict[str, ChainCommit] = {}
    for commit in commits:
        name = f'{commit.sha[:12]} ({commit.subject})'
        if not commit.change_ids:
            problems.append(f'{name} has no Change-Id.')
            continue
        if len(commit.change_ids) > 1:
            problems.append(f'{name} has {len(commit.change_ids)} Change-Ids.')
        change_id = commit.change_ids[0]
        if other := seen.get(change_id):
            problems.append(
                f'{name} has the Change-Id of {other.sha[:12]} '
                f'({other.subject}): {change_id}.'
            )
        else:
            seen[change_id] = commit
    return problems


class Gerrit:
    """A Gerrit project, reached over SSH: `git push` and `gerrit`
    commands, with the user's SSH key."""

    def __init__(self, url: str):
        self.url = url
        parts = urlsplit(url)
        self._host = parts.hostname or ''
        self._user = parts.username
        self._port = parts.port or 29418
        self.project = parts.path.strip('/').removesuffix('.git')

    def ssh_argv(self, *args: str) -> list[str]:
        """The command running `gerrit <args>`, quoted for the server's
        shell-like parsing of the command line."""
        destination = f'{self._user}@{self._host}' if self._user else self._host
        return [
            'ssh',
            '-o',
            'BatchMode=yes',
            '-p',
            str(self._port),
            destination,
            'gerrit',
            *map(shlex.quote, args),
        ]

    def _ssh(self, *args: str) -> str:
        return subprocess.run(
            self.ssh_argv(*args), check=True, capture_output=True, text=True
        ).stdout

    def changes(self, branch: str) -> list[Change]:
        """The open and abandoned CLs of `branch`, page by page."""
        changes: list[Change] = []
        while True:
            more = False
            for line in self._ssh(
                'query',
                '--format=JSON',
                '--current-patch-set',
                f'--start={len(changes)}',
                f'project:{self.project} branch:{branch} '
                '(status:open OR status:abandoned)',
            ).splitlines():
                row = json.loads(line)
                if row.get('type') == 'stats':
                    more = bool(row.get('moreChanges'))
                    continue
                changes.append(
                    Change(
                        change_id=row['id'],
                        number=int(row['number']),
                        patch_set=int(row['currentPatchSet']['number']),
                        status=row['status'],
                    )
                )
            if not more:
                return changes

    def abandon(self, change: Change, message: str) -> None:
        self._ssh('review', '--abandon', '--message', message, change.revision)

    def restore(self, change: Change, message: str) -> None:
        self._ssh('review', '--restore', '--message', message, change.revision)

    def branch_tip(self, cwd: Path, branch: str) -> str | None:
        """The commit `branch` is at on Gerrit, or None if it has none."""
        output = _git(cwd, 'ls-remote', self.url, f'refs/heads/{branch}')
        return output.split()[0] if output else None


def upload(
    *,
    cwd: Path,
    gerrit: Gerrit,
    branch: str,
    base: str,
    head: str,
    dry_run: bool = False,
    log: Callable[[str], None] = print,
) -> int:
    """Uploads `base..head` as `branch`'s relation chain, as the module says.

    `base` and `head` are refs of the checkout at `cwd`, e.g. `origin/master`
    and `origin/cr160`. With `dry_run`, it only reads Gerrit, and says what
    it would change. Returns the exit status: 1 when it refused or stopped.
    """
    fork = _git(cwd, 'merge-base', base, head)
    head_sha = _git(cwd, 'rev-parse', head)
    commits = chain_commits(cwd, fork, head_sha)
    if problems := chain_problems(commits):
        log(f'Not uploading {branch}: its Change-Ids are not in order.')
        for problem in problems:
            log(f'    {problem}')
        return 1
    log(f'{branch}: {len(commits)} commits on {fork[:12]}.')

    def push(*args: str) -> None:
        if dry_run:
            log(f'Would run `git push {shlex.join(args)}`.')
            return
        _git(cwd, 'push', gerrit.url, *args)

    base_branch = base.split('/', 1)[-1]
    if not _CR_BRANCH.fullmatch(base_branch):
        base_sha = _git(cwd, 'rev-parse', base)
        if gerrit.branch_tip(cwd, base_branch) != base_sha:
            log(f'Bringing {base_branch} up to date with {base}.')
            try:
                push(f'{base_sha}:refs/heads/{base_branch}')
            except subprocess.CalledProcessError as e:
                log(
                    f"Gerrit's {base_branch} has diverged from {base}, so it "
                    f'is left alone:\n{e.stderr.strip()}'
                )
                return 1
    if gerrit.branch_tip(cwd, branch) != fork:
        log(f"Moving Gerrit's {branch} to the fork point, {fork[:12]}.")
        push('--force', f'{fork}:refs/heads/{branch}')

    chain_ids = {commit.change_ids[0] for commit in commits}
    changes = gerrit.changes(branch)
    for change in changes:
        if change.status == 'ABANDONED' and change.change_id in chain_ids:
            log(f'Restoring {change.number}: its commit is back on {branch}.')
            if not dry_run:
                gerrit.restore(change, f'The commit is back on {branch}.')

    log(f'Pushing the chain to refs/for/{branch}.')
    try:
        push(
            '-o',
            f'topic={branch}',
            '-o',
            'notify=NONE',
            f'{head_sha}:refs/for/{branch}',
        )
    except subprocess.CalledProcessError as e:
        if 'no new changes' not in e.stderr:
            log(f'The push to Gerrit failed:\n{e.stderr.strip()}')
            return 1
        log('Gerrit has every patch set already.')

    for change in changes:
        if change.status != 'ABANDONED' and change.change_id not in chain_ids:
            log(f'Abandoning {change.number}: its commit left {branch}.')
            if not dry_run:
                gerrit.abandon(
                    change, f'The commit is no longer on GitHub\'s {branch}.'
                )
    return 0
