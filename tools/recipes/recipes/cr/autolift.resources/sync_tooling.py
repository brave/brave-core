#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Brings the lift tooling of cr<N> up to date with the newer cr branches.

A maintenance branch would otherwise run its lift with the skill and tools it
had when the newer branch above it was cut. For each newer cr<M>, oldest first,
this walks M's own commits that touch the tooling (`tools/cr/`, `build/` and
the CI recipes in `tools/recipes/`), leaving out those cr<N> already has
(`git cherry`):

    picked   a commit only under the tooling paths that cites no
             upstream culprit is cherry-picked, always without the milestone
             trackers (`tools/cr/codehealth/M<NNN>.md`), which belong to their
             own branch; a fixup goes by its target's culprit, and one with
             only a tracker is skipped
    pending  a commit that conflicts, one citing a culprit (tied to the newer
             branch's Chromium), one touching other paths, and a Brockit
             marker on a picked commit are left for an agent to judge and
             reconcile

A commit whose culprits this branch's Chromium (its `package.json` tag, in the
Chromium checkout above the repository) has none of is declined, as it follows
a Chromium change cr<N> doesn't get. It writes what it did, and the pending
commits with what to do about them, to `.autolift/cr<N>/sync-tooling.md`, and
records the synced HEAD for `prepare` to keep. It exits 2 when commits are
pending. Recipe changes are picked like the rest, but CI runs the recipes from
master, so they take effect only once merged there.

It ships with the `cr/autolift` recipe, rather than being read from a cr
branch, so that CI only runs code from master. It uses only the standard
library and git, and works on the repository it is given:

    python3 sync_tooling.py --repo src/brave --milestone 158
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass, field
import json
from pathlib import Path
import re
import subprocess
import sys

_TRACKER = re.compile(r'^tools/cr/codehealth/M\d+\.md$')
# A "Chromium changes:" link, as `git cr commit --culprit` writes it.
_CULPRIT = re.compile(
    r'chromium\.googlesource\.com/chromium/src/\+/([0-9a-f]{7,40})'
)
# `fixup!`/`amend!`/`squash!` commits, and the subject or hash they target.
_AUTOSQUASH = re.compile(r'^(?:(?:fixup|amend|squash)! )+(.*)$')
# Brockit's `drop!<hash>! <subject>` and `reassign!` markers, which act on a
# commit by hash, or by subject.
_MARKER = re.compile(r'^(?:drop|reassign)!([0-9a-f]*)! (.*)$')
_CANDIDATE_PATHS = ('tools/cr/', 'build/', 'tools/recipes/')
# Pinned subjects many commits share, which can't tell one pick from another.
_REPEATED_SUBJECT = re.compile(
    r'^(?:\[[^\]]*\])*\s*(?:Version lift updates|Version CodeHealth tracker|'
    r'IWYU fixes|Update Lit mangler snapshot)\.?$'
)
# The trailer each pick carries, as `git cherry-pick -x` writes it.
_PICKED_FROM = re.compile(
    r'^\(cherry picked from commit ([0-9a-f]{40})\)$', re.M
)
_BRANCH = re.compile(r'refs/heads/cr(\d+)$')

PENDING_EXIT = 2

# What the agent reconciling the pending commits is told.
_INSTRUCTIONS = """\
Each commit below is from a newer cr branch, and was left for judgement. In
order, for each one:

1. `git cherry-pick -x <commit>`: the trailer `-x` adds tells later syncs the
   commit is here, whatever reconciling it changed. Leave out
   `tools/cr/codehealth/M<NNN>.md` trackers: they belong to their own branch
   (`git rm --cached` a new one, or `git checkout HEAD -- <file>` a changed
   one, before continuing).
2. A conflict in `tools/cr/` is two branches improving the same tooling, often
   a `Version lift updates` or `Version CodeHealth tracker` squash: keep both
   sides' changes, merged into one text, and the newer side where they say
   different things about the same rule.
3. A commit that cites an upstream culprit (or a fixup of one) this branch's
   Chromium has, or that the script couldn't check, follows a Chromium change:
   it applies only if this branch needs it too (a bytecode redirect, a GN arg,
   a Brockit change for that upstream change). Those whose culprits this
   branch's Chromium lacks are declined already. Changes outside `tools/cr/`,
   `build/` and `tools/recipes/` are the newer branch's own code, and don't
   belong here.
   Keep what applies, and drop the rest; a commit with nothing left is skipped
   (`git cherry-pick --skip`). A Brockit `drop!` or `reassign!` marker on a
   commit picked here gets the same treatment as on its own branch: picked,
   if its target should go here too.
4. `git cherry-pick --continue`, keeping the message.

Commit nothing else, and don't push. When done, `git status` must be clean,
and no cherry-pick may be in progress. Then run `{rerun}` again, which picks
whatever these unblocked, and repeat until it exits 0, or lists as pending only
commits you decided not to take; say which, and why, in the run's report.
"""


def _git(repo: Path, *args: str, check: bool = True) -> str:
    result = subprocess.run(
        ['git', '-C', str(repo), *args],
        capture_output=True,
        text=True,
        encoding='utf-8',
        errors='replace',
        check=False,
    )
    if check and result.returncode:
        raise subprocess.CalledProcessError(
            result.returncode, args, result.stdout, result.stderr
        )
    return result.stdout


@dataclass
class Outcome:
    """What the sync did: `(branch, commit, subject, note)` per commit."""

    picked: list[tuple[str, str, str, str]] = field(default_factory=list)
    skipped: list[tuple[str, str, str, str]] = field(default_factory=list)
    pending: list[tuple[str, str, str, str]] = field(default_factory=list)
    declined: list[tuple[str, str, str, str]] = field(default_factory=list)


def classify(files: list[str], has_culprit: bool) -> tuple[str, list[str]]:
    """`(kind, trackers)` for a commit: `none` when it touches no tooling,
    `pick` when it touches only the tooling paths without citing an
    upstream culprit, and `judge` otherwise: a culprit ties the change to the
    newer branch's Chromium, and other paths are that branch's own code.
    `trackers` are the milestone trackers to leave out."""
    trackers = [f for f in files if _TRACKER.match(f)]
    rest = [f for f in files if f not in trackers]
    if not any(f.startswith(_CANDIDATE_PATHS) for f in rest):
        return 'none', trackers
    if all(f.startswith(_CANDIDATE_PATHS) for f in rest) and not has_culprit:
        return 'pick', trackers
    return 'judge', trackers


class SyncTooling:
    """The sync of one checkout's cr<N> from the newer cr branches."""

    def __init__(
        self,
        repo: Path,
        milestone: int,
        remote: str = 'origin',
        chromium: Path | None = None,
    ):
        self.repo, self.milestone, self.remote = repo, milestone, remote
        # The Chromium checkout brave-core sits in, as `src/brave`.
        self.chromium = chromium or repo.parent
        self.outcome = Outcome()
        self._tag: str | None = None

    def tag(self) -> str | None:
        """The Chromium tag of cr<N>'s `package.json`, or None."""
        if self._tag is None:
            try:
                config = json.loads(
                    _git(self.repo, 'show', 'HEAD:package.json')
                )
                self._tag = config['config']['projects']['chrome']['tag']
            except (subprocess.CalledProcessError, KeyError, ValueError):
                self._tag = ''
        return self._tag or None

    def has_culprits(self, culprits: list[str]) -> bool | None:
        """Whether cr<N>'s Chromium has any of `culprits`: None when there
        are none, or it can't tell (no tag, or a commit the checkout lacks)."""
        tag = self.tag()
        if not culprits or tag is None:
            return None
        for culprit in culprits:
            result = subprocess.run(
                [
                    'git',
                    '-C',
                    str(self.chromium),
                    'merge-base',
                    '--is-ancestor',
                    culprit,
                    tag,
                ],
                capture_output=True,
                check=False,
            )
            if result.returncode == 0:
                return True
            if result.returncode != 1:
                return None
        return False

    def newer_branches(self) -> list[int]:
        out = _git(
            self.repo, 'ls-remote', '--heads', self.remote, 'refs/heads/cr*'
        )
        majors = sorted(
            int(m.group(1))
            for line in out.splitlines()
            if (m := _BRANCH.search(line))
        )
        return [major for major in majors if major > self.milestone]

    def _ref(self, major: int) -> str:
        return f'{self.remote}/cr{major}'

    def own_commits(
        self, major: int, majors: list[int]
    ) -> list[tuple[str, bool]]:
        """cr<major>'s own commits, oldest first, with whether HEAD already has
        an equivalent: those after where it forks from the branch below it
        (cr<N> for the first newer branch)."""
        below = max([m for m in majors if m < major] + [self.milestone])
        base = _git(
            self.repo, 'merge-base', self._ref(major), self._ref(below)
        ).strip()
        out = _git(self.repo, 'cherry', 'HEAD', self._ref(major), base)
        # A commit picked before with changes (a resolved conflict, a tracker
        # left out) has another patch id, but its pick cites it; once the newer
        # branch is rebased, the pick cites its old hash, but keeps its
        # subject.
        fork = _git(self.repo, 'merge-base', 'HEAD', self._ref(major)).strip()
        cited, cited_subjects = set(), set()
        log = _git(self.repo, 'log', '--format=%s%x00%B%x01', f'{fork}..HEAD')
        for entry in log.split('\x01'):
            subject, _, body = entry.strip('\n').partition('\x00')
            if picked := _PICKED_FROM.findall(body):
                cited.update(picked)
                cited_subjects.add(subject)
        commits = []
        for line in out.splitlines():
            commit = line[2:]
            subject = self._subject(commit)
            present = (
                line[0] == '-'
                or commit in cited
                or (
                    subject in cited_subjects
                    and not _REPEATED_SUBJECT.match(subject)
                    and not _AUTOSQUASH.match(subject)
                )
                or self._in_sync(commit, self._ref(major))
            )
            commits.append((commit, present))
        return commits

    def _in_sync(self, commit: str, tip: str) -> bool:
        """Whether every file `commit` touches is on HEAD as at `tip`, so
        that it brings nothing, whatever form it took here (fixups squashed
        away, a rebase that changed its hash)."""
        files = [f for f in self._files(commit) if not _TRACKER.match(f)]
        return (
            bool(files)
            and not subprocess.run(
                [
                    'git',
                    '-C',
                    str(self.repo),
                    'diff',
                    '--quiet',
                    'HEAD',
                    tip,
                    '--',
                    *files,
                ],
                capture_output=True,
                check=False,
            ).returncode
        )

    @staticmethod
    def _target(
        ref: str, seen: dict[str, tuple[str, bool, bool]]
    ) -> tuple[str, bool, bool] | None:
        """The commit an autosquash `ref` (a subject or a hash prefix) names
        among those seen, as git's `--autosquash` would find it."""
        if ref in seen:
            return seen[ref]
        if len(ref) >= 4 and re.fullmatch(r'[0-9a-f]+', ref):
            return next(
                (v for v in seen.values() if v[0].startswith(ref)), None
            )
        return None

    def _on_head(self, ref: str) -> tuple[str, bool, bool] | None:
        """A target already in HEAD's history, below where the newer branch
        forked (cr<N>'s own commit, which the newer one inherited), as
        `_target` returns it."""
        if re.fullmatch(r'[0-9a-f]{4,40}', ref):
            found = _git(
                self.repo,
                'rev-parse',
                '--verify',
                '-q',
                f'{ref}^{{commit}}',
                check=False,
            ).strip()
            if (
                found
                and not subprocess.run(
                    [
                        'git',
                        '-C',
                        str(self.repo),
                        'merge-base',
                        '--is-ancestor',
                        found,
                        'HEAD',
                    ],
                    capture_output=True,
                    check=False,
                ).returncode
            ):
                message = _git(self.repo, 'log', '-1', '--format=%B', found)
                return found, True, bool(_CULPRIT.search(message))
            return None
        out = _git(
            self.repo,
            'log',
            '--format=%H%x00%s',
            '--fixed-strings',
            f'--grep={ref}',
            'HEAD',
        )
        for line in out.splitlines():
            commit, _, subject = line.partition('\x00')
            if subject == ref:
                message = _git(self.repo, 'log', '-1', '--format=%B', commit)
                return commit, True, bool(_CULPRIT.search(message))
        return None

    def _subject(self, commit: str) -> str:
        return _git(self.repo, 'log', '-1', '--format=%s', commit).strip()

    def _files(self, commit: str) -> list[str]:
        return _git(
            self.repo,
            'diff-tree',
            '--no-commit-id',
            '--name-only',
            '-r',
            commit,
        ).split()

    def _pick(self, commit: str, trackers: list[str]) -> str | None:
        """Cherry-picks `commit` without `trackers`, committed with its
        message. Returns why it couldn't, or None."""
        result = subprocess.run(
            ['git', '-C', str(self.repo), 'cherry-pick', '--no-commit', commit],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode:
            conflicted = _git(
                self.repo, 'diff', '--name-only', '--diff-filter=U'
            ).split()
            _git(self.repo, 'cherry-pick', '--abort', check=False)
            _git(self.repo, 'reset', '--hard', 'HEAD')
            return f'conflicts in {", ".join(conflicted) or "the index"}'
        for tracker in trackers:
            if _git(self.repo, 'ls-tree', 'HEAD', '--', tracker).strip():
                _git(self.repo, 'checkout', 'HEAD', '--', tracker)
            else:
                _git(self.repo, 'rm', '-q', '--cached', '-f', '--', tracker)
                (self.repo / tracker).unlink(missing_ok=True)
        if not _git(self.repo, 'diff', '--cached', '--name-only').strip():
            _git(self.repo, 'cherry-pick', '--abort', check=False)
            _git(self.repo, 'reset', '--hard', 'HEAD')
            return 'empty'
        message = _git(self.repo, 'log', '-1', '--format=%B', commit).rstrip()
        _git(
            self.repo,
            'commit',
            '--no-verify',
            '-q',
            '-m',
            f'{message}\n\n(cherry picked from commit {commit})',
        )
        return None

    def run(self) -> Outcome:
        if _git(self.repo, 'status', '--porcelain', '--untracked-files=no'):
            raise RuntimeError('Tracked files have changes; nothing synced.')
        majors = self.newer_branches()
        if majors:
            _git(
                self.repo,
                'fetch',
                '-q',
                self.remote,
                *(f'cr{m}' for m in majors),
            )
        # The files of pending commits: a later commit touching one of them
        # depends on how that one gets reconciled, so it waits too.
        held: set[str] = set()
        # Subjects declined in this sync, whose fixups go with them.
        declined: set[str] = set()
        for major in majors:
            branch = f'cr{major}'
            # This branch's commits seen so far: subject -> (hash, whether it
            # was picked, whether it cites a culprit), for autosquash targets.
            seen: dict[str, tuple[str, bool, bool]] = {}
            for commit, present in self.own_commits(major, majors):
                subject = self._subject(commit)
                message = _git(self.repo, 'log', '-1', '--format=%B', commit)
                culprit = bool(_CULPRIT.search(message))
                if present:
                    # Already here: a target fixups and markers can act on.
                    seen.setdefault(subject, (commit, True, culprit))
                    continue
                files = self._files(commit)
                entry = (branch, commit[:12], subject)
                squash = _AUTOSQUASH.match(subject)
                if marker := _MARKER.match(subject):
                    # Only one acting on a commit picked here concerns cr<N>.
                    target = self._target(
                        marker.group(1), seen
                    ) or self._target(marker.group(2), seen)
                    if target and target[1]:
                        self.outcome.pending.append(
                            (*entry, 'a Brockit marker on a picked commit')
                        )
                    continue
                why = None
                # A fixup carries no culprit of its own: it follows its target.
                if squash:
                    target = self._target(
                        squash.group(1), seen
                    ) or self._on_head(squash.group(1))
                    if target is None or not target[1]:
                        why = "its target wasn't picked"
                    culprit = culprit or bool(target and target[2])
                kind, trackers = classify(files, culprit)
                if kind == 'none':
                    seen.setdefault(subject, (commit, False, culprit))
                    if trackers and len(trackers) == len(files):
                        self.outcome.skipped.append(
                            (*entry, 'only its tracker')
                        )
                    continue
                why_declined = None
                if squash and squash.group(1) in declined:
                    why_declined = 'its target was declined'
                elif self.has_culprits(_CULPRIT.findall(message)) is False:
                    why_declined = f'cites no culprit {self.tag()} has'
                if why_declined:
                    seen[subject] = (commit, False, culprit)
                    declined.add(subject)
                    self.outcome.declined.append((*entry, why_declined))
                    continue
                if why is not None:
                    pass
                elif held.intersection(files):
                    why = 'touches files of an earlier pending commit'
                elif kind == 'judge':
                    why = (
                        'cites a culprit, or touches paths outside the tooling'
                    )
                else:
                    why = self._pick(commit, trackers)
                seen.setdefault(subject, (commit, why is None, culprit))
                if why is None:
                    note = 'without its tracker' if trackers else ''
                    self.outcome.picked.append((*entry, note))
                elif why == 'empty':
                    self.outcome.skipped.append(
                        (*entry, 'nothing left: already here, or its tracker')
                    )
                else:
                    self.outcome.pending.append((*entry, why))
                    held.update(f for f in files if f not in trackers)
        return self.outcome


def format_outcome(outcome: Outcome, milestone: int, rerun: str) -> str:
    lines = [f'# Tooling sync of cr{milestone}', '']
    if not (
        outcome.picked or outcome.skipped or outcome.pending or outcome.declined
    ):
        lines.append(
            'Nothing to sync: the newer branches have no tooling '
            'changes this branch lacks.'
        )
    for title, entries in (
        ('Picked', outcome.picked),
        ('Skipped', outcome.skipped),
        ('Declined', outcome.declined),
        ('Pending', outcome.pending),
    ):
        if not entries:
            continue
        lines += [f'## {title}', '']
        for branch, commit, subject, note in entries:
            lines.append(
                f'- {branch} {commit} {subject}'
                + (f' ({note})' if note else '')
            )
        lines.append('')
    if outcome.pending:
        lines += [
            '## What to do',
            '',
            _INSTRUCTIONS.format(rerun=rerun),
        ]
    return '\n'.join(lines).rstrip() + '\n'


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--milestone', type=int, required=True)
    parser.add_argument('--remote', default='origin')
    args = parser.parse_args()
    repo = args.repo.resolve()
    sync = SyncTooling(repo, args.milestone, args.remote)
    try:
        outcome = sync.run()
    except RuntimeError as error:
        print(error, file=sys.stderr)
        return 1
    run_dir = repo / '.autolift' / f'cr{args.milestone}'
    run_dir.mkdir(parents=True, exist_ok=True)
    rerun = ' '.join(
        [
            'python3',
            str(Path(__file__).resolve()),
            '--repo',
            str(repo),
            '--milestone',
            str(args.milestone),
        ]
    )
    text = format_outcome(outcome, args.milestone, rerun)
    (run_dir / 'sync-tooling.md').write_text(text, encoding='utf-8')
    # `prepare` keeps the commits made here, rather than resetting them away.
    (run_dir / 'synced-head').write_text(
        _git(repo, 'rev-parse', 'HEAD'), encoding='utf-8'
    )
    print(text)
    return PENDING_EXIT if outcome.pending else 0


if __name__ == '__main__':
    sys.exit(main())
