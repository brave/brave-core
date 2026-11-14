#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Lists, for each code health rule, the upstream commits it applies to.

The rules live in `rules/<sweep>/*.md`, one per mechanism, and each sweep's
`COMMON_METADATA` is a knowledge base saying which upstream commits they apply
to: see `tools/cr/knowledge_base.py` for its fields. A group whose `tool` runs
a command lists that command's output instead, with `{old}` and `{new}` for
the tags, and a group that asks nothing of a commit has its rules read the
whole range.

Writes the range's dump files and one `<rule>.candidates` file per rule under
the output directory, and prints how many candidates each rule has.

    vpython3 tools/cr/codehealth/scripts/sweep.py OLD..NEW
        [--out DIR] [--sweep NAME]
"""

from __future__ import annotations

import argparse
import shlex
import subprocess
import sys
from collections import defaultdict
from collections.abc import Iterator
from collections.abc import Set as AbstractSet
from dataclasses import dataclass
from pathlib import Path

import _boot  # noqa: F401  -- adds tools/cr to sys.path
import repository
from knowledge_base import Change, KnowledgeBase, Match, Source
from terminal import console

RULES_DIR = Path(__file__).resolve().parents[1] / 'rules'

# What a candidate list shows of each commit, so that a rule's list stays
# readable: the commit is there, and `git show` has the rest.
_FILES_PER_COMMIT = 3
_LINES_PER_FILE = 3
_LINE_WIDTH = 160


@dataclass(frozen=True)
class Commit:
    """A range commit."""

    sha: str
    change: Change

    @property
    def subject(self) -> str:
        return self.change.subject


@dataclass(frozen=True)
class RangeDump:
    """The range's commits and their changes, read once with `git log -p`."""

    old: str
    new: str
    out: Path

    # Ends a commit's body: a body can quote a diff.
    _BODY_END = '@@@ body end'

    def write_files(self) -> None:
        """The dump files the review steps read: subjects, files, bodies, the
        range's diff, and each commit's changed lines (`changes`)."""
        chromium = repository.chromium
        dumps = {
            'subjects': ['log', '--abbrev=12', '--format=%h %s'],
            'files': ['log', '--format=@@@ %h %s', '--name-only'],
            'bodies': ['log', '--format=@@@ %H%n%B'],
        }
        for name, args in dumps.items():
            text = chromium.run_git(
                *args, f'{self.old}..{self.new}', no_trim=True
            )
            (self.out / name).write_text(text, encoding='utf-8', newline='')
        diff = chromium.run_git('diff', '-U0', self.old, self.new, no_trim=True)
        (self.out / 'range.diff').write_text(diff, encoding='utf-8', newline='')
        # `<hash> <path>:<+/- line>` per changed line, so that a grep hit
        # names its commit, which `range.diff` can't.
        with (self.out / 'changes').open(
            'w', encoding='utf-8', newline=''
        ) as file:
            for commit in self.commits():
                for path, change in commit.change.files.items():
                    for line in change.lines:
                        file.write(f'{commit.sha[:12]} {path}:{line}\n')

    def commits(self) -> Iterator[Commit]:
        """Each commit of the range, with its changed lines."""
        root = Path(repository.chromium.root)
        with subprocess.Popen(
            [
                'git',
                '-C',
                str(root),
                'log',
                '-p',
                '-U0',
                '--no-color',
                '--no-ext-diff',
                f'--format=@@@ %H %s%n%b{self._BODY_END}',
                f'{self.old}..{self.new}',
            ],
            stdout=subprocess.PIPE,
            text=True,
            errors='replace',
        ) as process:
            header: tuple[str, str] | None = None
            body: list[str] = []
            diff: list[str] = []
            in_body = False

            def commit() -> Commit:
                sha, subject = header
                return Commit(
                    sha,
                    Change.parse(
                        '\n'.join(diff),
                        subject=subject,
                        body=body,
                        source=Source(root, sha),
                    ),
                )

            for line in process.stdout:
                line = line.rstrip('\n')
                if in_body:
                    if line == self._BODY_END:
                        in_body = False
                    else:
                        body.append(line)
                elif line.startswith('@@@ '):
                    if header:
                        yield commit()
                    sha, _, subject = line[4:].partition(' ')
                    header, body, diff, in_body = (sha, subject), [], [], True
                else:
                    diff.append(line)
            if header:
                yield commit()


class Sweep:
    """Matches the rules of each sweep against the range."""

    def __init__(self, dump: RangeDump, sweeps: list[str] | None):
        self._dump = dump
        self._bases: dict[str, KnowledgeBase] = {
            metadata.parent.name: KnowledgeBase(metadata.parent)
            for metadata in sorted(RULES_DIR.glob('*/COMMON_METADATA'))
            if not sweeps or metadata.parent.name in sweeps
        }

    def run(self) -> dict[str, dict[str, list[str]]]:
        """`{sweep: {rule: candidate entries}}`."""
        found: dict[str, dict[str, list[str]]] = {
            sweep: defaultdict(list) for sweep in self._bases
        }
        # Rules that read the whole range aren't listed by commit.
        unlisted = {
            rule
            for base in self._bases.values()
            for group in base.groups
            if group.unconditional
            for rule in group.rules
        }
        commits = list(self._dump.commits())
        subjects = {c.subject for c in commits}
        for commit in commits:
            for sweep, base in self._bases.items():
                for match in base.match(commit.change):
                    if match.rule.path not in unlisted:
                        found[sweep][match.rule.path].append(
                            self._entry(commit, match, subjects)
                        )
        for sweep, base in self._bases.items():
            for group in base.groups:
                if group.tool:
                    output = self._run_tool(group.tool)
                    for rule in group.rules:
                        found[sweep][rule].append(output)
        return found

    @staticmethod
    def _entry(
        commit: Commit, match: Match, subjects: AbstractSet[str] = frozenset()
    ) -> str:
        # A change and its revert cancel out: say so on both.
        note = ''
        if f'Revert "{commit.subject}"' in subjects:
            note = ' (reverted in range)'
        elif (
            commit.subject.startswith('Revert "')
            and commit.subject.endswith('"')
            and commit.subject[len('Revert "') : -1] in subjects
        ):
            note = ' (reverts a commit in range)'
        lines = [f'{commit.sha[:12]} {commit.subject}{note}']
        lines += [
            f'    > {line[:_LINE_WIDTH]}'
            for line in match.message[:_LINES_PER_FILE]
        ]
        for path, changed in list(match.files.items())[:_FILES_PER_COMMIT]:
            lines.append(f'    {path}')
            lines += [
                f'      {line[:_LINE_WIDTH]}'
                for line in changed[:_LINES_PER_FILE]
            ]
        if len(match.files) > _FILES_PER_COMMIT:
            lines.append(
                f'    ... {len(match.files) - _FILES_PER_COMMIT} more files'
            )
        return '\n'.join(lines)

    def _run_tool(self, command: str) -> str:
        command = command.format(old=self._dump.old, new=self._dump.new)
        result = subprocess.run(
            shlex.split(command),
            cwd=repository.brave.root,
            capture_output=True,
            text=True,
            check=False,
        )
        return (
            f'$ {command}\n(exit {result.returncode})\n'
            f'{result.stdout}{result.stderr}'
        )

    def write(
        self, found: dict[str, dict[str, list[str]]]
    ) -> list[tuple[str, str, str]]:
        """Writes each rule's candidates; returns the summary rows."""
        rows: list[tuple[str, str, str]] = []
        for sweep, base in self._bases.items():
            directory = self._dump.out / 'rules' / sweep
            directory.mkdir(parents=True, exist_ok=True)
            lists = []
            for rule in base.rules:
                name = Path(rule).stem
                entries = found[sweep].get(rule, [])
                groups = [g for g in base.groups if rule in g.rules]
                if any(g.unconditional for g in groups):
                    status = 'whole range'
                    text = 'Read the whole range.\n'
                elif any(g.tool for g in groups):
                    status = 'tool'
                    text = '\n\n'.join(entries) + '\n'
                else:
                    status = f'{len(entries)} commits'
                    text = '\n\n'.join(entries) + '\n' if entries else ''
                lists.append((name, status, text))
            # Rules sharing a group share a list, which is read only once,
            # under the first rule the group names (`overridden-files` holds
            # the overlap list), and otherwise the first rule.
            leads = [Path(g.rules[0]).stem for g in base.groups if g.rules]
            owners: dict[str, str] = {}
            for name, _, text in sorted(
                lists, key=lambda item: item[0] not in leads
            ):
                if text:
                    owners.setdefault(text, name)
            for name, status, text in lists:
                owner = owners.get(text, name)
                if owner != name:
                    status += f', as {owner}'
                    text = f'Same as {owner}.candidates.\n'
                (directory / f'{name}.candidates').write_text(
                    text, encoding='utf-8', newline=''
                )
                rows.append((sweep, name, status))
        return rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('range', help='OLD..NEW Chromium tags.')
    parser.add_argument(
        '--out', type=Path, help='Output directory; /tmp/codehealth_<NEW>.'
    )
    parser.add_argument(
        '--sweep', action='append', help='Only this sweep (repeatable).'
    )
    args = parser.parse_args()
    old, _, new = args.range.partition('..')
    if not old or not new:
        parser.error('expected OLD..NEW')
    out = args.out or Path(f'/tmp/codehealth_{new}')
    out.mkdir(parents=True, exist_ok=True)

    dump = RangeDump(old, new, out)
    dump.write_files()
    sweep = Sweep(dump, args.sweep)
    rows = sweep.write(sweep.run())
    for row in rows:
        console.print(f'{row[0]:<14} {row[1]:<50} {row[2]}')
    console.print(f'Candidates in {out}/rules/.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
