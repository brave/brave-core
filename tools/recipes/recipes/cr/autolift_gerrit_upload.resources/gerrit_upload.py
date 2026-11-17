#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Uploads a `cr<N>` branch's own commits to Gerrit as a relation chain of CLs.

Run after autolift has published `cr<N>` to GitHub and brave-core has been
mirrored into Gerrit, in the checkout the lift ran in. The chain is
`<start>..<head>`, the branch's own commits, `<start>` being where brockit finds
they start; what is below it is the base, `master` or `cr<N-1>`, which Gerrit
has already: in its `master`, or as `cr<N-1>`'s own chain. The upload:

1. checks that every commit of the chain has exactly one Change-Id, and that no
   two share one;
2. checks that Gerrit has `<start>`, in its `master` or as a patch set of a CL,
   as pushing `<head>` would otherwise upload the base's commits as part of the
   chain;
3. restores the abandoned CLs, of any topic, whose Change-Id is in the chain,
   as Gerrit takes no new patch sets on them;
4. pushes `<head>` to `refs/for/master`, with topic `cr<N>` and the `--cc`
   users CC'd on its CLs. Gerrit skips the
   commits it has as patch sets already, so the base's CLs stay as they are,
   and only the chain's new or changed commits become new CLs or patch sets. A
   commit that keeps the Change-Id of another chain's CL, as a cherry-pick
   does, updates that CL, which moves to topic `cr<N>` with it;
5. abandons the open CLs of the topic whose Change-Id left the chain. A CL that
   moved to another chain is in that chain's topic by then;
6. with `--json-output`, writes the links of the CLs the push created, whose
   Change-Ids Gerrit had no CL of, and of the commits `--review` lists.

It uses only the standard library, git and ssh, and changes nothing in the
checkout:

    vpython3 gerrit_upload.py --url ssh://<user>@<host>/<project> \\
        --branch cr160 --start <sha> --head origin/cr160 [--dry-run]
"""

from __future__ import annotations

import argparse
from collections.abc import Callable, Iterable
from dataclasses import dataclass
import json
from pathlib import Path
import shlex
import subprocess
import sys
from urllib.parse import urlsplit

TARGET = 'master'

# Change-Ids per query: each adds a `change:` term to the command line.
CHANGE_IDS_PER_QUERY = 50


@dataclass(frozen=True)
class ChainCommit:
    sha: str
    subject: str
    change_ids: tuple[str, ...]


@dataclass(frozen=True)
class Change:
    """A CL on Gerrit."""

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


def _succeeds(cwd: Path, *args: str) -> bool:
    return (
        subprocess.run(
            ['git', '-C', str(cwd), *args], capture_output=True, check=False
        ).returncode
        == 0
    )


def chain_commits(cwd: Path, start: str, head: str) -> list[ChainCommit]:
    """The commits from `start` to `head`, oldest first, with the Change-Ids
    of their trailers, as git parses them: quoted ones aren't trailers."""
    log = _git(
        cwd,
        'log',
        '--reverse',
        '--format=%H%x00%s%x00'
        '%(trailers:key=Change-Id,valueonly,separator=%x2C)%x1e',
        f'{start}..{head}',
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


def _name(commit: ChainCommit) -> str:
    return f'{commit.sha[:12]} ({commit.subject})'


def chain_problems(commits: list[ChainCommit]) -> list[str]:
    """Why the chain can't go to Gerrit, if it can't: a commit without a
    Change-Id, with several, or with one another commit has too."""
    problems = []
    seen: dict[str, ChainCommit] = {}
    for commit in commits:
        if not commit.change_ids:
            problems.append(f'{_name(commit)} has no Change-Id.')
            continue
        if len(commit.change_ids) > 1:
            problems.append(
                f'{_name(commit)} has {len(commit.change_ids)} Change-Ids.'
            )
        change_id = commit.change_ids[0]
        if other := seen.get(change_id):
            problems.append(
                f'{_name(commit)} has the Change-Id of {_name(other)}: '
                f'{change_id}.'
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

    def _query(self, query: str, *options: str) -> list[dict]:
        """The CLs `query` finds, with their current patch set (and what
        `options` add), page by page."""
        rows: list[dict] = []
        while True:
            more = False
            for line in self._ssh(
                'query',
                '--format=JSON',
                '--current-patch-set',
                *options,
                f'--start={len(rows)}',
                f'project:{self.project} {query}',
            ).splitlines():
                row = json.loads(line)
                if row.get('type') == 'stats':
                    more = bool(row.get('moreChanges'))
                else:
                    rows.append(row)
            if not more:
                return rows

    def _changes(self, query: str) -> list[Change]:
        return [
            Change(
                change_id=row['id'],
                number=int(row['number']),
                patch_set=int(row['currentPatchSet']['number']),
                status=row['status'],
            )
            for row in self._query(f'branch:{TARGET} {query}')
        ]

    def open_changes(self, topic: str) -> list[Change]:
        """The open CLs of `topic`."""
        return self._changes(f'topic:{topic} status:open')

    def _changes_with_ids(
        self, change_ids: Iterable[str], terms: str = ''
    ) -> list[Change]:
        """The CLs with any of `change_ids` that `terms` also match, whatever
        their topic, a few queries' worth at a time."""
        change_ids = sorted(change_ids)
        changes = []
        for start in range(0, len(change_ids), CHANGE_IDS_PER_QUERY):
            ids = change_ids[start : start + CHANGE_IDS_PER_QUERY]
            changes += self._changes(
                f'{terms} '.lstrip()
                + f'({" OR ".join(f"change:{change_id}" for change_id in ids)})'
            )
        return changes

    def abandoned_changes(self, change_ids: Iterable[str]) -> list[Change]:
        """The abandoned CLs with any of `change_ids`, whatever their topic."""
        return self._changes_with_ids(change_ids, 'status:abandoned')

    def known_change_ids(self, change_ids: Iterable[str]) -> set[str]:
        """Which of `change_ids` have a CL, in any state."""
        return {
            change.change_id for change in self._changes_with_ids(change_ids)
        }

    def has_patch_set(self, sha: str) -> bool:
        """Whether `sha` is a patch set of a CL, current or not."""
        return bool(self._query(f'commit:{sha}'))

    def review_link(self, sha: str) -> tuple[int, int, str] | None:
        """`(change, patch set, URL)` of the patch set `sha` is, the URL
        showing what changed since the patch set before it; None when `sha`
        is no patch set."""
        for row in self._query(f'commit:{sha}', '--patch-sets'):
            for patch_set in row.get('patchSets', []):
                if patch_set.get('revision') == sha:
                    number = int(patch_set['number'])
                    url = row['url']
                    if number > 1:
                        url += f'/{number - 1}..{number}'
                    return int(row['number']), number, url
        return None

    def abandon(self, change: Change, message: str) -> None:
        self._ssh('review', '--abandon', '--message', message, change.revision)

    def restore(self, change: Change, message: str) -> None:
        self._ssh('review', '--restore', '--message', message, change.revision)

    def branch_tip(self, cwd: Path, branch: str) -> str | None:
        """The commit `branch` is at on Gerrit, or None if it has none."""
        output = _git(cwd, 'ls-remote', self.url, f'refs/heads/{branch}')
        return output.split()[0] if output else None


def _on_gerrit(cwd: Path, gerrit: Gerrit, sha: str) -> bool:
    """Whether Gerrit has `sha`: in its `master`, which may be ahead of the
    checkout's, or as a patch set."""
    if tip := gerrit.branch_tip(cwd, TARGET):
        if not _succeeds(cwd, 'cat-file', '-e', f'{tip}^{{commit}}'):
            # Only into FETCH_HEAD: no branch of the checkout moves.
            _git(
                cwd,
                'fetch',
                '--no-tags',
                '--quiet',
                gerrit.url,
                f'refs/heads/{TARGET}',
            )
        if _succeeds(cwd, 'merge-base', '--is-ancestor', sha, tip):
            return True
    return gerrit.has_patch_set(sha)


def upload(
    *,
    cwd: Path,
    gerrit: Gerrit,
    branch: str,
    start: str,
    head: str,
    dry_run: bool = False,
    log: Callable[[str], None] = print,
    created: list[ChainCommit] | None = None,
    ccs: Iterable[str] = (),
) -> int:
    """Uploads `start..head` as `branch`'s chain, as the module says.

    With `dry_run`, it only reads Gerrit, and says what it would change.
    Returns the exit status: 1 when it refused to upload. The commits whose
    CLs the push created, their Change-Ids new to Gerrit, go in `created`.
    """
    head_sha = _git(cwd, 'rev-parse', head)
    start_sha = _git(cwd, 'rev-parse', start)
    if not _succeeds(cwd, 'merge-base', '--is-ancestor', start_sha, head_sha):
        log(f'Not uploading {branch}: {start_sha[:12]} is not on {head}.')
        return 1
    commits = chain_commits(cwd, start_sha, head_sha)
    if problems := chain_problems(commits):
        log(f'Not uploading {branch}: its Change-Ids are not in order.')
        for problem in problems:
            log(f'    {problem}')
        return 1
    log(f'{branch}: {len(commits)} commits on {start_sha[:12]}.')

    if not _on_gerrit(cwd, gerrit, start_sha):
        log(
            f'Not uploading {branch}: Gerrit has no {start_sha[:12]}, where '
            'its own commits start, so the push would upload its base too. '
            'Upload the base first.'
        )
        return 1

    chain_ids = {commit.change_ids[0] for commit in commits}
    # Before the push, which moves CLs from other chains to this topic, and
    # creates the CLs of the Change-Ids Gerrit doesn't know.
    open_changes = gerrit.open_changes(branch)
    known = gerrit.known_change_ids(chain_ids)
    new = [commit for commit in commits if commit.change_ids[0] not in known]
    for change in gerrit.abandoned_changes(chain_ids):
        log(f'Restoring {change.number}: its commit is on {branch}.')
        if not dry_run:
            gerrit.restore(change, f'The commit is on {branch}.')

    refspec = f'{head_sha}:refs/for/{TARGET}'
    push = ['-o', f'topic={branch}', '-o', 'notify=NONE']
    for cc in ccs:
        push += ['-o', f'cc={cc}']
    push.append(refspec)
    if dry_run:
        log(f'Would run `git push {gerrit.url} {shlex.join(push)}`.')
        for commit in new:
            log(f'Would create a CL for {_name(commit)}.')
    else:
        log(f'Pushing the chain to refs/for/{TARGET}.')
        try:
            _git(cwd, 'push', gerrit.url, *push)
        except subprocess.CalledProcessError as e:
            if 'no new changes' not in e.stderr:
                log(f'The push to Gerrit failed:\n{e.stderr.strip()}')
                return 1
            log('Gerrit has every patch set already.')
        if created is not None:
            created.extend(new)

    for change in open_changes:
        if change.change_id not in chain_ids:
            log(f'Abandoning {change.number}: its commit left {branch}.')
            if not dry_run:
                gerrit.abandon(
                    change, f"The commit is no longer on GitHub's {branch}."
                )
    return 0


def review_links(
    gerrit: Gerrit,
    commits: list[dict],
    log: Callable[[str], None] = print,
) -> list[dict]:
    """The CL and patch set each of `commits` (each a `Commit`: its `sha`
    and `subject`) became, with a link to what changed in it."""
    links = []
    for commit in commits:
        found = gerrit.review_link(commit['sha'])
        if found is None:
            log(f'{commit["sha"][:12]} ({commit["subject"]}) is on no CL.')
            continue
        change, patch_set, url = found
        links.append(
            {
                'commit': commit,
                'change': change,
                'patch_set': patch_set,
                'url': url,
            }
        )
    return links


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        '--url', required=True, help='ssh://<user>@<host>/<project>'
    )
    parser.add_argument('--branch', required=True, help='e.g. cr160')
    parser.add_argument(
        '--start', required=True, help="Where the branch's own commits start."
    )
    parser.add_argument('--head', required=True, help='e.g. origin/cr160')
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Only read Gerrit, and say what would change.',
    )
    parser.add_argument(
        '--cc',
        action='append',
        default=[],
        help='A Gerrit user to CC on the chain\'s CLs; repeat for more.',
    )
    parser.add_argument(
        '--review',
        type=Path,
        help='A JSON list of the commits to review, each with its `sha` and '
        '`subject`.',
    )
    parser.add_argument(
        '--json-output',
        type=Path,
        help='Where to write, once uploaded, the CLs and links of the '
        'commits to review (`review`), and of the CLs the push created '
        '(`created`).',
    )
    args = parser.parse_args()
    gerrit = Gerrit(args.url)
    created: list[ChainCommit] = []
    returncode = upload(
        cwd=Path.cwd(),
        gerrit=gerrit,
        branch=args.branch,
        start=args.start,
        head=args.head,
        dry_run=args.dry_run,
        created=created,
        ccs=args.cc,
    )
    if args.json_output:
        commits = (
            json.loads(args.review.read_text(encoding='utf-8'))
            if args.review and not returncode
            else []
        )
        args.json_output.write_text(
            json.dumps(
                {
                    'review': review_links(gerrit, commits),
                    'created': review_links(
                        gerrit,
                        [
                            {'sha': commit.sha, 'subject': commit.subject}
                            for commit in created
                        ],
                    ),
                }
            ),
            encoding='utf-8',
        )
    return returncode


if __name__ == '__main__':
    sys.exit(main())
