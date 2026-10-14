#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Lists the range's migration campaigns: commits clustered by `Bug:` id.

A cluster of three or more commits is listed in full when it touches what
Brave depends on: a file Brave overrides (whole-file `chromium_src`
replacements marked, since a scripted campaign only fixes upstream's text), or
a header Brave includes. Clusters whose subjects are mostly campaign verbs
("Migrate", "Replace", "Convert") but touch neither are listed one per line,
for the trend count. Platform-only clusters (ash, ChromeOS), rolls, and
clusters that only turn `DCHECK`s into non-fatal `CHECK`s are skipped.

    vpython3 tools/cr/codehealth/scripts/campaigns.py OLD..NEW
"""

from __future__ import annotations

import argparse
from collections import defaultdict
from dataclasses import dataclass, field
import functools
import re
import sys

import _boot  # noqa: F401  -- adds tools/cr to sys.path
import repository  # noqa: E402  -- after _boot
from sweep import BraveTree  # noqa: E402  -- after _boot


@dataclass
class Commit:
    """A range commit: the bugs it cites, and the files it touches."""

    sha: str
    subject: str
    bugs: list[str] = field(default_factory=list)
    files: list[str] = field(default_factory=list)


@dataclass
class Cluster:
    """The commits that share the same bugs, and what of Brave's they touch."""

    bugs: list[str]
    commits: list[Commit]
    # Brave-overridden files the cluster touches, `*` marking whole-file
    # replacements.
    overridden: list[str] = field(default_factory=list)
    included: list[str] = field(default_factory=list)
    # Hand-written commits that change a header Brave includes: the ones that
    # drop a parameter Brave's call sites still pass.
    by_hand: list[Commit] = field(default_factory=list)
    # Whether all it does to Brave is turn `DCHECK`s into non-fatal `CHECK`s.
    not_fatal_only: bool = False

    @property
    def name(self) -> str:
        return ', '.join(f'crbug.com/{b}' for b in self.bugs)

    @property
    def touches_brave(self) -> bool:
        return bool(self.overridden or self.included)


class CampaignIndex:
    """Clusters a range's commits by bug, against what Brave depends on."""

    MIN_COMMITS = 3
    _SHOWN = 3
    _BUG_LINE = re.compile(r'^(?:Bug|Fixed):\s*(.+)$', re.MULTILINE)
    _BUG_ID = re.compile(r'^(?:chromium:|b:|crbug\.com/|b/)?(\d{6,})$')
    _CAMPAIGN_VERB = re.compile(
        r'(?i)^(?:\[[^\]]+\]\s*)*(?:\w[\w ./-]*:\s*)?'
        r'(?:automatic|migrate|replace|convert|'
        r'remove|use|switch|rename|deprecate|move|clean ?up|spanify|'
        r'modernize|rewrite)\b'
    )
    _SCRIPTED = re.compile(
        r'(?i)\b(?:automatic|automated|mechanical|scripted|mass)\b|'
        r'\[\d+/\d+\]|\(\d+/(?:\d+|N)\)'
    )
    _SKIPPED = re.compile(
        r'(?i)^(?:\[(?:ash|chromeos|gardener)\]|(?:ash|chromeos):|roll )'
    )
    _REVERT = re.compile(r'^\W*Revert\b')
    _INCLUDE = re.compile(r'^\s*#\s*(?:include|import)\s*[<"]([^>"]+)[>"]')
    _SOURCE = ('.h', '.cc', '.mm', '.cpp')
    # Overridden files every campaign touches in passing.
    _NOISE = re.compile(
        r'(?:^|.*/)(?:DEPS|OWNERS|BUILD\.gn|[^/]*\.gni|[^/]*\.grdp?|'
        r'histograms\.xml|enums\.xml|about_flags\.cc|flag-metadata\.json)$'
    )
    _DCHECK = re.compile(r'\bDCHECK')
    _NOT_FATAL = re.compile(r',\s*base::NotFatalUntil::M\d+')
    _SPACE = re.compile(r'\s+')
    _COMMENT = re.compile(r'\s//.*$|^\s*//.*$')
    # The `DCHECK_IS_ON()` guard a conversion drops around its checks.
    _GUARD = re.compile(r'-\s*#\s*(?:if\s+DCHECK_IS_ON\(\)|endif\b)')
    _DIFF_FILE = re.compile(r'^diff --git a/(\S+)')

    def __init__(
        self, old: str, new: str, brave: BraveTree | None = None
    ) -> None:
        self.old, self.new = old, new
        self._brave = brave or BraveTree()

    def commits(self) -> list[Commit]:
        """The range's commits, read from `git log`."""
        log = repository.chromium.run_git(
            'log',
            '--format=@@@ %H %s%n%b@@@ files',
            '--name-only',
            f'{self.old}..{self.new}',
            no_trim=True,
        )
        return self.parse(log)

    @classmethod
    def parse(cls, log: str) -> list[Commit]:
        """Commits from the `git log` format `commits()` asks for."""
        commits = []
        for chunk in log.split('@@@ ')[1:]:
            if chunk.startswith('files'):
                commits[-1].files = [
                    f for f in chunk[len('files') :].split('\n') if f
                ]
                continue
            head, _, body = chunk.partition('\n')
            sha, _, subject = head.partition(' ')
            bugs = []
            for line in cls._BUG_LINE.findall(body):
                for token in re.split(r'[,\s]+', line):
                    match = cls._BUG_ID.match(token)
                    if match and match.group(1) not in bugs:
                        bugs.append(match.group(1))
            commits.append(Commit(sha, subject, bugs))
        return commits

    @functools.cached_property
    def _included(self) -> set[str]:
        """Every upstream path Brave's sources include or import."""
        root = repository.brave.root
        paths = set()
        for path in repository.brave.run_git('ls-files', '-z').split('\0'):
            if not path.endswith(self._SOURCE + ('.swift',)):
                continue
            if path.startswith(('third_party/', 'vendor/')):
                continue
            try:
                text = (root / path).read_bytes().decode('utf-8', 'replace')
            except OSError:
                continue
            for line in text.splitlines():
                match = self._INCLUDE.match(line)
                if match:
                    paths.add(match.group(1).removeprefix('src/'))
        return paths

    def _whole_file(self, path: str) -> bool:
        """Whether Brave's `chromium_src` file replaces upstream's outright,
        never including the original."""
        override = repository.brave.root / 'chromium_src' / path
        if not override.is_file():
            return False
        text = override.read_bytes().decode('utf-8', 'replace')
        # A WebUI override imports the original, which the build renames to
        # `<name>-chromium.<rest>` (`app.ts` imports `./app-chromium.js`).
        if path.endswith(('.ts', '.js')):
            first = override.name.split('.')[0]
            return not re.search(
                rf"""['"]\./{re.escape(first)}-chromium\.[\w.]*js['"]""", text
            )
        includes = (self._INCLUDE.match(line) for line in text.splitlines())
        return not any(
            match and match.group(1).removeprefix('src/') == path
            for match in includes
        )

    def _overridden(self, path: str) -> bool:
        return path in self._brave.overridden and not self._NOISE.match(path)

    def _included_header(self, path: str) -> bool:
        return path.endswith('.h') and path in self._included

    def _touches_brave(self, path: str) -> bool:
        return self._overridden(path) or self._included_header(path)

    def _diff(self, sha: str) -> str:
        return repository.chromium.run_git(
            'show', '--format=', '-U0', '--no-color', sha, no_trim=True
        )

    def not_fatal_files(self, commit: Commit) -> set[str]:
        """The files whose only change in the commit turns `DCHECK`s into
        `CHECK(..., base::NotFatalUntil::M<N>)`, which never stops a release
        build. Each hunk must read the same once the `D` and the
        `NotFatalUntil` argument go; includes, comments and the
        `DCHECK_IS_ON()` guards it drops don't count."""
        converted: set[str] = set()
        changed: set[str] = set()
        path = ''
        removed: list[str] = []
        added: list[str] = []

        def same() -> None:
            before, dchecks = self._DCHECK.subn('CHECK', ''.join(removed))
            after, not_fatal = self._NOT_FATAL.subn('', ''.join(added))
            removed.clear()
            added.clear()
            # Each `DCHECK` comes back with its own `NotFatalUntil`.
            if before != after or dchecks != not_fatal:
                changed.add(path)
            elif dchecks:
                converted.add(path)

        for line in self._diff(commit.sha).splitlines():
            if line.startswith(('---', '+++')):
                continue
            if line.startswith(('-', '+')):
                if self._GUARD.match(line):
                    converted.add(path)
                    continue
                if line[1:].lstrip().startswith('#include'):
                    continue
                text = self._SPACE.sub('', self._COMMENT.sub('', line[1:]))
                (removed if line[0] == '-' else added).append(text)
                continue
            same()
            if file := self._DIFF_FILE.match(line):
                path = file.group(1)
        same()
        return converted - changed

    def clusters(self, commits: list[Commit]) -> list[Cluster]:
        """The campaign clusters of `commits`, those touching Brave first."""
        by_bug: dict[str, list[Commit]] = defaultdict(list)
        for commit in commits:
            for bug in commit.bugs:
                by_bug[bug].append(commit)
        # One cluster per set of commits: a campaign often cites several bugs.
        by_set: dict[tuple[str, ...], list[str]] = defaultdict(list)
        members_of: dict[tuple[str, ...], list[Commit]] = {}
        for bug, members in by_bug.items():
            key = tuple(c.sha for c in members)
            by_set[key].append(bug)
            members_of[key] = members
        clusters = []
        for key, bugs in by_set.items():
            # Reverts last, so that the commits a cluster shows are the
            # campaign's own.
            members = sorted(
                members_of[key],
                key=lambda c: bool(self._REVERT.search(c.subject)),
            )
            if len(members) < self.MIN_COMMITS:
                continue
            skipped = sum(
                bool(self._SKIPPED.search(c.subject)) for c in members
            )
            if skipped * 2 > len(members):
                continue
            verbs = sum(
                bool(self._CAMPAIGN_VERB.search(c.subject)) for c in members
            )
            if verbs * 2 < len(members):
                continue
            # A file that a commit only makes `DCHECK`s non-fatal `CHECK`s in
            # costs Brave no more than a patch conflict, which the lift shows
            # anyway, so it doesn't count; one with a fatal `CHECK` does.
            exposed: dict[str, list[str]] = {}
            for c in members:
                files_of = c.files
                if any(map(self._touches_brave, files_of)):
                    not_fatal = self.not_fatal_files(c)
                    files_of = [f for f in files_of if f not in not_fatal]
                exposed[c.sha] = files_of
            files = sorted({f for fs in exposed.values() for f in fs})
            overridden = [
                f'{f} *' if self._whole_file(f) else f
                for f in files
                if self._overridden(f)
            ]
            overridden.sort(key=lambda f: not f.endswith(' *'))
            included = [f for f in files if self._included_header(f)]
            headers = set(included)
            by_hand = [
                c
                for c in members
                if not self._SCRIPTED.search(c.subject)
                and headers.intersection(exposed[c.sha])
            ]
            touched = any(
                map(self._touches_brave, (f for c in members for f in c.files))
            )
            not_fatal_only = touched and not overridden and not included
            clusters.append(
                Cluster(
                    bugs, members, overridden, included, by_hand, not_fatal_only
                )
            )
        clusters.sort(
            key=lambda c: (
                -len(c.overridden),
                -len(c.included),
                -len(c.commits),
            )
        )
        return clusters

    def report(self, clusters: list[Cluster]) -> str:
        """The clusters as the candidate list prints them."""
        lines = []
        listed = [c for c in clusters if c.touches_brave]
        lines.append(
            f'## {len(listed)} clusters touching what Brave overrides (* whole '
            f'file) or includes'
        )
        for cluster in listed:
            lines.append('')
            lines.append(f'{cluster.name}: {len(cluster.commits)} commits')
            for commit in cluster.commits[: self._SHOWN]:
                lines.append(f'  {commit.sha[:12]} {commit.subject}')
            if len(cluster.commits) > self._SHOWN:
                lines.append(f'  ... {len(cluster.commits) - self._SHOWN} more')
            if cluster.included:
                lines.append(
                    f'  Brave includes {len(cluster.included)} of its headers'
                )
            paths = cluster.overridden
            if paths:
                shown = ', '.join(paths[: self._SHOWN])
                more = (
                    f' (+{len(paths) - self._SHOWN})'
                    if len(paths) > self._SHOWN
                    else ''
                )
                lines.append(f'  Brave overrides: {shown}{more}')
            for commit in cluster.by_hand[: self._SHOWN]:
                lines.append(
                    f'  by hand, on a header Brave includes: '
                    f'{commit.sha[:12]} {commit.subject}'
                )
            if len(cluster.by_hand) > self._SHOWN:
                lines.append(
                    f'  ... {len(cluster.by_hand) - self._SHOWN} more by hand'
                )
        others = [
            c for c in clusters if not c.touches_brave and not c.not_fatal_only
        ]
        skipped = [c for c in clusters if c.not_fatal_only]
        if skipped:
            lines.append('')
            lines.append(
                'Skipped, only `DCHECK` -> `CHECK(..., NotFatalUntil)`: '
                + ', '.join(f'{c.name} ({len(c.commits)})' for c in skipped)
            )
        lines.append('')
        lines.append(
            f'## {len(others)} other campaigns: grep Brave for their idiom'
        )
        for cluster in others:
            lines.append(
                f'{cluster.name} ({len(cluster.commits)}): '
                f'{cluster.commits[0].sha[:12]} {cluster.commits[0].subject}'
            )
        return '\n'.join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('range', help='OLD..NEW Chromium tags.')
    args = parser.parse_args()
    old, _, new = args.range.partition('..')
    if not old or not new:
        parser.error('expected OLD..NEW')
    index = CampaignIndex(old, new)
    print(index.report(index.clusters(index.commits())))
    return 0


if __name__ == '__main__':
    sys.exit(main())
