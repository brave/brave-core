#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Lists, for each code health rule, the upstream commits it applies to.

The rules live in `rules/<sweep>/*.md`, one per mechanism, and each sweep's
`COMMON_METADATA` says which changes they apply to, in the format of
Chromium's `agents/knowledge_base`:

    agentic_review {
      capability_groups {
        path_expression: "chrome/android/.*\\.java"
        content_patterns: "\\bVERSION_CODES\\b"
        rules: "tools/cr/codehealth/rules/android_ios/some-rule.md"
      }
    }

A commit is a candidate for a group's rules when a file it touches matches
`path_expression` (the whole path), and a line it adds or removes in that
file matches one of the `content_patterns`, if there are any. Brave adds:

    exclude_path_expression  files that never count (build files, strings)
    overridden_by_brave      only files Brave overrides: `chromium_src`,
                             patches, plasters and Lit manglers
    bytecode_adapted         only Java files whose class Brave's bytecode
                             adapters name
    required_patterns        each must match a changed line of the file too,
                             e.g. the `+` half of a flip
    brave_mentions           a regex whose first group, found in a changed
                             line or the path, must be a word Brave's sources
                             spell: upstream names Brave doesn't use never
                             count
    ast_pattern              an ast-grep pattern the file must hold at the
                             new tag, for what a regex can't tell (C++ is read
                             with plaster's macro blanking first)
    ast_language             the pattern's language; by default the file's,
                             with `.h` read as C++
    tool                     a command run instead, with `{old}` and `{new}`
                             for the tags; its output is the candidate list
    subject_patterns,        regexes the commit's subject, or a line of its
    body_patterns            body, must match; with no file fields, they
                             alone make a commit a candidate
    exclude_subject_expression  subjects that never count (`[ash]`, rolls)

Patterns are Python regexes: write flags scoped, `(?i:…)`, anywhere but at
the start.

A rule no group names applies to the whole range, and is listed as such.
Writes the range's dump files and one `<rule>.candidates` file per rule under
the output directory, and prints how many candidates each rule has.

    vpython3 tools/cr/codehealth/scripts/sweep.py OLD..NEW
        [--out DIR] [--sweep NAME]
"""

from __future__ import annotations

import argparse
from collections import defaultdict
from collections.abc import Iterator
from collections.abc import Set as AbstractSet
import dataclasses
from dataclasses import dataclass, field
import functools
import json
from pathlib import Path, PurePosixPath
import re
import shlex
import subprocess
import sys

import _boot  # noqa: F401  -- adds tools/cr to sys.path
from plaster import (  # noqa: E402  -- after _boot
    AST_GREP_BIN,
    AST_GREP_SGCONFIG,
    BlankForParseOptions,
    CxxMacrosEraser,
)
from overlap import OverrideIndex  # noqa: E402  -- after _boot
import repository  # noqa: E402  -- after _boot
from terminal import console  # noqa: E402  -- after _boot

RULES_DIR = Path(__file__).resolve().parents[1] / 'rules'

# What a candidate list shows of each commit, so that a rule's list stays
# readable: the commit is there, and `git show` has the rest.
_FILES_PER_COMMIT = 3
_LINES_PER_FILE = 3
_LINE_WIDTH = 160

_CXX_SUFFIXES = ('.cc', '.h', '.mm', '.cpp')

# ast-grep's language for a path, always passed explicitly: left to guess, it
# reads `.h` as C. `gn` is the custom grammar plaster's sgconfig registers, and
# JSON parses fine as JS.
_AST_LANGUAGES = {
    **{suffix: 'cpp' for suffix in _CXX_SUFFIXES},
    '.gn': 'gn',
    '.gni': 'gn',
    '.java': 'java',
    '.js': 'js',
    '.json': 'js',
    '.json5': 'js',
    '.py': 'python',
    '.ts': 'ts',
}


@dataclass(frozen=True)
class CapabilityGroup:
    """One `capability_groups` entry: when its rules apply.

    The fields are the textproto's (see the module docstring); the patterns
    are compiled on construction, so a bad one fails the parse.
    """

    rules: list[str] = field(default_factory=list)
    path_expression: str = '.*'
    exclude_path_expression: str = ''
    content_patterns: list[str] = field(default_factory=list)
    required_patterns: list[str] = field(default_factory=list)
    overridden_by_brave: bool = False
    bytecode_adapted: bool = False
    brave_mentions: str = ''
    ast_language: str = ''
    ast_pattern: str = ''
    tool: str = ''
    subject_patterns: list[str] = field(default_factory=list)
    body_patterns: list[str] = field(default_factory=list)
    exclude_subject_expression: str = ''

    _path: re.Pattern = field(init=False, repr=False, compare=False)
    _exclude: re.Pattern | None = field(init=False, repr=False, compare=False)
    _content: list[re.Pattern] = field(init=False, repr=False, compare=False)
    _required: list[re.Pattern] = field(init=False, repr=False, compare=False)
    _mentions: re.Pattern | None = field(init=False, repr=False, compare=False)
    _subject: list[re.Pattern] = field(init=False, repr=False, compare=False)
    _body: list[re.Pattern] = field(init=False, repr=False, compare=False)
    _exclude_subject: re.Pattern | None = field(
        init=False, repr=False, compare=False
    )

    _REPEATED = (
        'rules',
        'content_patterns',
        'required_patterns',
        'subject_patterns',
        'body_patterns',
    )
    _FILE_FIELDS = (
        'exclude_path_expression',
        'content_patterns',
        'required_patterns',
        'overridden_by_brave',
        'bytecode_adapted',
        'brave_mentions',
        'ast_pattern',
    )
    _FLAGS = ('overridden_by_brave', 'bytecode_adapted')
    # A Java source's class, as bytecode names it.
    _JAVA_CLASS = re.compile(r'(?:^|/)(org/chromium/[\w/]+)\.java$')

    def __post_init__(self) -> None:
        compiled = {
            '_path': re.compile(self.path_expression),
            '_exclude': self._optional(self.exclude_path_expression),
            '_content': [re.compile(p) for p in self.content_patterns],
            '_required': [re.compile(p) for p in self.required_patterns],
            '_mentions': self._optional(self.brave_mentions),
            '_subject': [re.compile(p) for p in self.subject_patterns],
            '_body': [re.compile(p) for p in self.body_patterns],
            '_exclude_subject': self._optional(self.exclude_subject_expression),
        }
        for name, value in compiled.items():
            object.__setattr__(self, name, value)

    @classmethod
    def from_fields(cls, fields: dict[str, str | list[str]]) -> CapabilityGroup:
        """A group from the `key: value` pairs of its textproto block.

        Raises:
            ValueError: for a field the group doesn't have.
        """
        known = {f.name for f in dataclasses.fields(cls) if f.init}
        values: dict[str, str | bool | list[str]] = {}
        for key, value in fields.items():
            if key not in known:
                raise ValueError(f'COMMON_METADATA: unknown field {key}')
            values[key] = value == 'true' if key in cls._FLAGS else value
        return cls(**values)

    @staticmethod
    def _optional(pattern: str) -> re.Pattern | None:
        return re.compile(pattern) if pattern else None

    @property
    def matches_files(self) -> bool:
        """Whether the group asks anything of the files a commit touches."""
        return self.path_expression != '.*' or any(
            getattr(self, f) for f in self._FILE_FIELDS
        )

    def message(self, commit: Commit) -> list[str] | None:
        """The body lines that matched, or None when the message rules the
        commit out."""
        if self._exclude_subject and self._exclude_subject.search(
            commit.subject
        ):
            return None
        if not self._subject and not self._body:
            return []
        hits = [
            line
            for line in commit.body
            if any(p.search(line) for p in self._body)
        ]
        if hits or any(p.search(commit.subject) for p in self._subject):
            return hits
        return None

    def files(self, commit: Commit, brave: BraveTree) -> dict[str, list[str]]:
        """The files of `commit` this group applies to, with the changed
        lines that matched (all of them when there are no patterns)."""
        matched: dict[str, list[str]] = {}
        for path, lines in commit.changes.items():
            # A renamed file also counts by its old path, which is the one
            # Brave overrode before the rebase moved the override along.
            names = [path, *commit.renamed.get(path, ())]
            if not any(self._names(name, brave) for name in names):
                continue
            if not all(
                any(p.search(line) for line in lines) for p in self._required
            ):
                continue
            if self._content:
                lines = [
                    line
                    for line in lines
                    if any(p.search(line) for p in self._content)
                ]
                if not lines:
                    continue
            if self._mentions and not self._mentioned(path, brave):
                lines = [line for line in lines if self._mentioned(line, brave)]
                if not lines:
                    continue
            matched[path] = lines
        return matched

    def _names(self, path: str, brave: BraveTree) -> bool:
        if not self._path.fullmatch(path):
            return False
        if self._exclude and self._exclude.fullmatch(path):
            return False
        if self.bytecode_adapted:
            java_class = self._JAVA_CLASS.search(path)
            if not java_class or java_class.group(1) not in brave.adapted:
                return False
        return not self.overridden_by_brave or path in brave.overridden

    def _mentioned(self, text: str, brave: BraveTree) -> bool:
        return any(
            m.group(1) in brave.words for m in self._mentions.finditer(text)
        )


class BraveTree:
    """What a group can ask about Brave's own sources."""

    # The sources whose words count as Brave mentioning an upstream name.
    _MENTION_SUFFIXES = (
        '.java',
        '.kt',
        '.flags',
        '.xml',
        '.patch',
        '.yaml',
        '.h',
        '.cc',
        '.mm',
        '.swift',
        '.gn',
        '.gni',
        '.ts',
        '.json5',
    )
    _WORD = re.compile(r'[A-Za-z_]\w+')
    # How a bytecode adapter names the upstream class it rewrites.
    _ADAPTED = re.compile(r'"(org/chromium/[\w/$]+)"')

    @functools.cached_property
    def overridden(self) -> set[str]:
        """The upstream files Brave overrides: `chromium_src`, patches,
        plasters and Lit manglers."""
        return set(OverrideIndex(Path(repository.brave.root)).kinds)

    @functools.cached_property
    def adapted(self) -> set[str]:
        """The upstream Java classes Brave's bytecode adapters name, as
        `org/chromium/.../Class`: an inner class counts as its file's."""
        adapters = Path(repository.brave.root) / 'build/android/bytecode'
        return {
            name.split('$', 1)[0]
            for path in adapters.rglob('*.java')
            for name in self._ADAPTED.findall(
                path.read_bytes().decode('utf-8', 'replace')
            )
        }

    @functools.cached_property
    def words(self) -> set[str]:
        """Every identifier Brave's tracked sources spell."""
        root = repository.brave.root
        words: set[str] = set()
        for path in repository.brave.run_git('ls-files', '-z').split('\0'):
            if path.startswith(('third_party/', 'vendor/')):
                continue
            if not path.endswith(self._MENTION_SUFFIXES):
                continue
            try:
                text = (root / path).read_bytes().decode('utf-8', 'replace')
            except OSError:
                continue
            words.update(self._WORD.findall(text))
        return words


class Metadata:
    """A `COMMON_METADATA` file: the subset of textproto it uses."""

    _TOKEN = re.compile(
        r'\s*(?:(#[^\n]*)|(\w+)\s*\{|(\})|(\w+)\s*:\s*("(?:\\.|[^"\\])*"|\w+))'
    )

    @classmethod
    def parse(cls, text: str) -> list[CapabilityGroup]:
        """The capability groups of a `COMMON_METADATA` text.

        Raises:
            ValueError: on text it can't parse, or an unknown field.
        """
        groups: list[CapabilityGroup] = []
        group: dict[str, str | list[str]] | None = None
        depth, pos = 0, 0
        while pos < len(text.rstrip()):
            match = cls._TOKEN.match(text, pos)
            if match is None:
                raise ValueError(f'COMMON_METADATA: cannot parse at {pos}')
            pos = match.end()
            comment, block, close, key, value = match.groups()
            if comment:
                continue
            if block:
                depth += 1
                if block == 'capability_groups':
                    group = {}
                continue
            if close:
                depth -= 1
                if group is not None and depth == 1:
                    groups.append(CapabilityGroup.from_fields(group))
                    group = None
                continue
            if group is None:
                continue
            parsed = json.loads(value) if value.startswith('"') else value
            if key in CapabilityGroup._REPEATED:
                group.setdefault(key, []).append(parsed)
            else:
                group[key] = parsed
        return groups


@dataclass
class Commit:
    """A range commit, filled in as `git log -p` is read."""

    sha: str
    subject: str
    # path -> the lines the commit adds or removes there, `+`/`-` prefixed.
    changes: dict[str, list[str]] = field(default_factory=dict)
    body: list[str] = field(default_factory=list)
    # new path -> its old path, for renames.
    renamed: dict[str, list[str]] = field(default_factory=dict)


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
                for path, lines in commit.changes.items():
                    for line in lines:
                        file.write(f'{commit.sha[:12]} {path}:{line}\n')

    def commits(self) -> Iterator[Commit]:
        """Each commit of the range, with its changed lines."""
        with subprocess.Popen(
            [
                'git',
                '-C',
                str(repository.chromium.root),
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
            commit, path, in_header, in_body = None, None, False, False
            for line in process.stdout:
                line = line.rstrip('\n')
                if in_body:
                    if line == self._BODY_END:
                        in_body = False
                    else:
                        commit.body.append(line)
                elif line.startswith('@@@ '):
                    if commit:
                        yield commit
                    sha, _, subject = line[4:].partition(' ')
                    commit, path, in_header = Commit(sha, subject), None, False
                    in_body = True
                elif line.startswith('diff --git '):
                    # The new path, which a deletion keeps as well.
                    path = line.split(' b/', 1)[-1]
                    commit.changes.setdefault(path, [])
                    in_header = True
                elif in_header and line.startswith('rename from '):
                    commit.renamed[path] = [line[len('rename from ') :]]
                elif in_header and line.startswith(('--- ', '+++ ')):
                    continue
                elif line.startswith('@@'):
                    in_header = False
                elif not in_header and path and commit and line[:1] in '+-':
                    commit.changes[path].append(line)
            if commit:
                yield commit


class Sweep:
    """Matches the rules of each sweep against the range."""

    def __init__(self, dump: RangeDump, sweeps: list[str] | None):
        self._dump = dump
        self._groups: dict[str, list[CapabilityGroup]] = {}
        self._rules: dict[str, list[str]] = {}
        for metadata in sorted(RULES_DIR.glob('*/COMMON_METADATA')):
            sweep = metadata.parent.name
            if sweeps and sweep not in sweeps:
                continue
            text = metadata.read_bytes().decode('utf-8')
            self._groups[sweep] = Metadata.parse(text)
            self._rules[sweep] = sorted(
                f'tools/cr/codehealth/rules/{sweep}/{p.name}'
                for p in metadata.parent.glob('*.md')
            )
        self._ast_cache: dict[tuple[str, str, str], bool] = {}

    def run(self) -> dict[str, dict[str, list[str]]]:
        """`{sweep: {rule: candidate lines}}`."""
        brave = BraveTree()
        found: dict[str, dict[str, list[str]]] = {
            sweep: defaultdict(list) for sweep in self._groups
        }
        matchers = [
            (sweep, group)
            for sweep, groups in self._groups.items()
            for group in groups
            if not group.tool
        ]
        commits = list(self._dump.commits())
        subjects = {c.subject for c in commits}
        for commit in commits:
            listed: set[str] = set()
            for sweep, group in matchers:
                rules = [r for r in group.rules if r not in listed]
                if not rules:
                    continue
                message = group.message(commit)
                if message is None:
                    continue
                files: dict[str, list[str]] = {}
                if group.matches_files:
                    files = group.files(commit, brave)
                    if group.ast_pattern:
                        files = {
                            p: lines
                            for p, lines in files.items()
                            if self._holds(group, p)
                        }
                    if not files:
                        continue
                entry = self._entry(commit, files, message, subjects)
                for rule in rules:
                    found[sweep][rule].append(entry)
                listed.update(rules)
        for sweep, groups in self._groups.items():
            for group in groups:
                if group.tool:
                    output = self._run_tool(group.tool)
                    for rule in group.rules:
                        found[sweep][rule].append(output)
        return found

    def _holds(self, group: CapabilityGroup, path: str) -> bool:
        name = PurePosixPath(path).name
        suffix = name if name == '.gn' else PurePosixPath(name).suffix
        language = group.ast_language or _AST_LANGUAGES.get(suffix)
        if language is None:
            return False
        key = (language, group.ast_pattern, path)
        if key not in self._ast_cache:
            try:
                text = repository.chromium.run_git(
                    'show', f'{self._dump.new}:{path}', no_trim=True
                )
            except subprocess.CalledProcessError:
                text = ''  # Deleted by the range.
            if path.endswith(_CXX_SUFFIXES):
                text = CxxMacrosEraser(
                    BlankForParseOptions(
                        macros=True,
                        string_adjacent_macros=True,
                        metadata_header_macros=True,
                    )
                ).erase(text)
            output = subprocess.run(
                [
                    str(AST_GREP_BIN),
                    'run',
                    '--config',
                    str(AST_GREP_SGCONFIG),
                    '--lang',
                    language,
                    '--pattern',
                    group.ast_pattern,
                    '--stdin',
                    '--json=compact',
                ],
                input=text,
                capture_output=True,
                text=True,
                check=False,
            ).stdout
            self._ast_cache[key] = bool(json.loads(output or '[]'))
        return self._ast_cache[key]

    @staticmethod
    def _entry(
        commit: Commit,
        files: dict[str, list[str]],
        message: list[str],
        subjects: AbstractSet[str] = frozenset(),
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
            f'    > {line[:_LINE_WIDTH]}' for line in message[:_LINES_PER_FILE]
        ]
        for path, changed in list(files.items())[:_FILES_PER_COMMIT]:
            lines.append(f'    {path}')
            lines += [
                f'      {line[:_LINE_WIDTH]}'
                for line in changed[:_LINES_PER_FILE]
            ]
        if len(files) > _FILES_PER_COMMIT:
            lines.append(f'    ... {len(files) - _FILES_PER_COMMIT} more files')
        return '\n'.join(lines)

    def _run_tool(self, command: str) -> str:
        args = shlex.split(
            command.format(old=self._dump.old, new=self._dump.new)
        )
        result = subprocess.run(
            args,
            cwd=repository.brave.root,
            capture_output=True,
            text=True,
            check=False,
        )
        return (
            f'$ {command.format(old=self._dump.old, new=self._dump.new)}\n'
            f'(exit {result.returncode})\n{result.stdout}{result.stderr}'
        )

    def write(
        self, found: dict[str, dict[str, list[str]]]
    ) -> list[tuple[str, str, str]]:
        """Writes each rule's candidates; returns the summary rows."""
        rows: list[tuple[str, str, str]] = []
        for sweep, rules in self._rules.items():
            directory = self._dump.out / 'rules' / sweep
            directory.mkdir(parents=True, exist_ok=True)
            named = {r for g in self._groups[sweep] for r in g.rules}
            lists = []
            for rule in rules:
                name = Path(rule).stem
                entries = found[sweep].get(rule, [])
                if rule not in named:
                    status = 'whole range'
                    text = 'No capability group: read the whole range.\n'
                elif any(
                    rule in g.rules and g.tool for g in self._groups[sweep]
                ):
                    status = 'tool'
                    text = '\n\n'.join(entries) + '\n'
                else:
                    status = f'{len(entries)} commits'
                    text = '\n\n'.join(entries) + '\n' if entries else ''
                lists.append((name, status, text))
            # Rules sharing a group share a list, which is read only once,
            # under the first rule the group names (`overridden-files` holds
            # the overlap list), and otherwise the first rule.
            leads = [
                Path(g.rules[0]).stem for g in self._groups[sweep] if g.rules
            ]
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
