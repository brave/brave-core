#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Code review knowledge bases, and which of their rules a change needs.

A knowledge base is a directory of Markdown rules, one convention each, and
the `COMMON_METADATA` files whose `agentic_review` capability groups say which
changes each rule applies to. It is the format of Chromium's
`agents/knowledge_base`, defined by the `AgenticReview` message of
`dir_metadata.proto` (infra/go/src/infra/tools/dirmd/proto/):

    agentic_review {
      capability_groups {
        path_expression: "(?:.*/)?[^/]*\\.(?:cc|h|mm)"
        content_patterns: "\\bRunUntilIdle\\b"
        rules: "docs/knowledge_base/rules/testing_async/no-run-until-idle.md"
      }
    }

A group applies to a change when one file the change touches meets all of the
group's file fields. Paths are relative to the root of the repository holding
the metadata file. Upstream's fields:

    path_expression            the file's path fully matches one of these
    path_expression_exclusion  ...and none of these
    content_patterns           one of these is found in a line the change
                               adds to the file
    rules                      the rules the group applies
    review_capability_skills   SKILL.md files a reviewer loads with them

A group with no file field applies to every change. Brave adds:

    diff_patterns              like `content_patterns`, but searched in the
                               file's changed lines with their diff marker:
                               `^-` finds a removal, `^\\+` an addition
    required_diff_patterns     each must be found in a changed line too
    ast_patterns               ast-grep patterns, one of which must match
                               code spanning an added line, in the file as
                               the change leaves it (C++ is read through
                               plaster's macro blanking, `.h` as C++, GN with
                               plaster's grammar, JSON as JS); an entry
                               written as a YAML mapping, `{...}`, is a whole
                               ast-grep rule, with `kind`, `has`, `inside` or
                               the `context` and `selector` a C++ call needs
                               to parse as one
    ast_language               the patterns' language, when not the file's
    overridden_by_brave        only upstream files Brave overrides
    bytecode_adapted           only Java files whose class Brave's bytecode
                               adapters name
    brave_mentions             a regex whose first group, found in a matched
                               line or the path, must be a word Brave's
                               sources spell
    subject_patterns,          the change's subject, or a line of its body,
    body_patterns              must match one of these
    subject_pattern_exclusion  ...and its subject none of these
    tool                       a command whose output lists the rules'
                               candidates instead; the group matches no change

`content_patterns`, `diff_patterns` and `ast_patterns` select the lines a
match reports; a file with none selected doesn't count. Patterns are Python
regexes, compatible with the RE2 syntax upstream writes: write flags scoped,
`(?i:…)`, anywhere but at the start.

A repository's DIR_METADATA files scope `agentic_review` to their directory,
inheriting from their ancestors, importing `mixins` and dropping
`exclude_rules`, as `dirmd` computes them. Brave reviews its changes with two
knowledge bases: its own, `docs/knowledge_base/rules`, which applies to every
Brave file, and Chromium's, `agents/knowledge_base/rules`, which applies to
each Brave file as `brave/<path>`, and to the upstream file it overrides
(`chromium_src`, `rewrite/` plasters and `patches/`, through the lines the
patch adds) under the upstream file's path and DIR_METADATA.

    vpython3 tools/cr/knowledge_base.py match [DIFF] [--head REV]
    vpython3 tools/cr/knowledge_base.py lint
"""

from __future__ import annotations

import argparse
import functools
import json
import re
import subprocess
import sys
from collections.abc import Callable, Iterable
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath
from typing import ClassVar

import repository
import yaml

# Each repository's own knowledge base, which applies to all of its files.
_BRAVE_RULES = 'docs/knowledge_base/rules'
_CHROMIUM_RULES = 'agents/knowledge_base/rules'

# Where a rule is read online, by the repository holding it.
_BRAVE_URL = 'https://github.com/brave/brave-core/blob/master/'
_CHROMIUM_URL = 'https://chromium.googlesource.com/chromium/src/+/main/'

# The severities rules use, most severe first, and the rubrics.
SEVERITIES = ('blocking', 'should_fix', 'nit', 'optional')
RUBRICS = (
    'correctness',
    'safety',
    'security',
    'design',
    'readability',
    'testing',
    'perf',
    'change_scope',
)

_CXX_SUFFIXES = ('.cc', '.h', '.mm', '.cpp')

# ast-grep's language for a path, always passed explicitly: left to guess, it
# reads `.h` as C. `gn` is the custom grammar plaster's sgconfig registers, and
# JSON parses fine as JS.
_AST_LANGUAGES = {
    **{suffix: 'cxx' for suffix in _CXX_SUFFIXES},
    '.gn': 'gn',
    '.gni': 'gn',
    '.java': 'java',
    '.kt': 'kotlin',
    '.js': 'js',
    '.json': 'js',
    '.json5': 'js',
    '.py': 'python',
    '.ts': 'ts',
    '.tsx': 'tsx',
    '.swift': 'swift',
}


def _log(message: str) -> None:
    print(message, file=sys.stderr)


class _TextProto:
    """The protobuf text format, as DIR_METADATA files write it.

    Every field parses to a list of its values, in order: strings, words
    (`true` and `false` as booleans, enums and numbers as written) and
    messages, which parse to dicts the same way.
    """

    _TOKEN = re.compile(
        r'''\s+|\#[^\n]*
        |(?P<string>"(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n])*')
        |(?P<word>[-+]?[\w.]+)
        |(?P<punct>[{}\[\]<>:,;])''',
        re.VERBOSE,
    )
    _ESCAPES: ClassVar[dict[str, str]] = {
        'n': '\n',
        't': '\t',
        'r': '\r',
        '\\': '\\',
        '"': '"',
        "'": "'",
    }

    def __init__(self, text: str, name: str):
        self._name = name
        self._tokens: list[tuple[str, str]] = []
        pos = 0
        while pos < len(text):
            match = self._TOKEN.match(text, pos)
            if match is None:
                raise ValueError(f'{name}: cannot parse at {text[pos:][:20]!r}')
            pos = match.end()
            if match.lastgroup:
                self._tokens.append((match.lastgroup, match.group()))
        self._pos = 0

    @classmethod
    def parse(cls, text: str, name: str) -> dict[str, list]:
        """The fields of `text`, a whole file.

        Raises:
            ValueError: on text that isn't the text format.
        """
        return cls(text, name)._fields(end=None)

    @classmethod
    def unescape(cls, body: str) -> str:
        """A string literal's body as text. A backslash the text format gives
        no meaning stays: upstream writes a regex's escaped dot both doubled
        and single."""
        return re.sub(
            r'\\(.)',
            lambda m: cls._ESCAPES.get(m.group(1), m.group()),
            body,
        )

    def _peek(self) -> tuple[str, str] | None:
        return (
            self._tokens[self._pos] if self._pos < len(self._tokens) else None
        )

    def _next(self) -> tuple[str, str]:
        token = self._peek()
        if token is None:
            raise ValueError(f'{self._name}: unexpected end of file')
        self._pos += 1
        return token

    def _fields(self, end: str | None) -> dict[str, list]:
        fields: dict[str, list] = {}
        while True:
            token = self._peek()
            if token is None:
                if end is not None:
                    raise ValueError(f'{self._name}: missing {end!r}')
                return fields
            if token == ('punct', end):
                self._pos += 1
                return fields
            if token[1] in (',', ';'):
                self._pos += 1
                continue
            kind, name = self._next()
            if kind != 'word':
                raise ValueError(f'{self._name}: expected a field, got {name}')
            if self._peek() == ('punct', ':'):
                self._pos += 1
            fields.setdefault(name, []).extend(self._values())

    def _values(self) -> list:
        token = self._peek()
        if token == ('punct', '['):
            self._pos += 1
            values = []
            while self._peek() != ('punct', ']'):
                if self._peek() == ('punct', ','):
                    self._pos += 1
                    continue
                values.extend(self._values())
            self._pos += 1
            return values
        return [self._value()]

    def _value(self):
        kind, text = self._next()
        if (kind, text) in (('punct', '{'), ('punct', '<')):
            return self._fields(end='}' if text == '{' else '>')
        if kind == 'string':
            value = self.unescape(text[1:-1])
            # Adjacent literals concatenate.
            while self._peek() and self._peek()[0] == 'string':
                value += self.unescape(self._next()[1][1:-1])
            return value
        if kind == 'word':
            return {'true': True, 'false': False}.get(text, text)
        raise ValueError(f'{self._name}: unexpected {text!r}')


def _mapping(text: str, what: str) -> dict:
    """`text`, a YAML mapping.

    Raises:
        ValueError: for text that isn't one.
    """
    try:
        value = yaml.safe_load(text)
    except yaml.YAMLError as e:
        raise ValueError(f'{what}: {e}') from e
    if not isinstance(value, dict):
        # The text is a bad value, whatever its type parsed to.
        raise ValueError(f'{what}: not a YAML mapping')  # noqa: TRY004
    return value


@dataclass(frozen=True)
class Rule:
    """A knowledge base rule: one convention, and how to check it."""

    # The rule's file, relative to the root of `repository`.
    path: str
    repository: Path
    name: str
    rubric: str
    severity: str
    description: str
    # Everything after the frontmatter.
    body: str

    _FRONTMATTER = re.compile(r'\A---\n(.*?)\n---\n', re.DOTALL)

    @classmethod
    def read(cls, root: Path, path: str) -> Rule:
        """The rule at `path` in the repository at `root`.

        Raises:
            ValueError: for a file without frontmatter, or whose frontmatter
                isn't a YAML mapping.
            OSError: for a file that can't be read.
        """
        text = (root / path).read_bytes().decode('utf-8')
        match = cls._FRONTMATTER.match(text)
        if not match:
            raise ValueError(f'{path}: no `---` frontmatter')
        meta = _mapping(match.group(1), f'{path}: frontmatter')
        return cls(
            path=path,
            repository=root,
            name=str(meta.get('name', '')),
            rubric=str(meta.get('rubric', '')),
            severity=str(meta.get('severity', '')),
            description=' '.join(str(meta.get('description', '')).split()),
            body=text[match.end() :],
        )

    @property
    def file(self) -> Path:
        return self.repository / self.path

    @property
    def text(self) -> str:
        """The whole file, as a reviewer reads it."""
        return self.file.read_bytes().decode('utf-8')

    @property
    def summary(self) -> str:
        """One sentence saying what the rule asks: its `description`, or
        the first sentence of its **Rule.** paragraph."""
        if self.description:
            return self.description
        match = re.search(
            r'\*\*Rule\.\*\*\s+(.*?)(?:\n\n|$)', self.body, re.DOTALL
        )
        paragraph = ' '.join(match.group(1).split()) if match else ''
        sentence = re.match(r'(.+?[.!?])(?:\s|$)', paragraph)
        return sentence.group(1) if sentence else paragraph

    @property
    def url(self) -> str:
        """Where the rule is read online, to cite it in a review."""
        root = self.repository.resolve()
        if root == Path(repository.brave.root).resolve():
            return _BRAVE_URL + self.path
        if root == Path(repository.chromium.root).resolve():
            return _CHROMIUM_URL + self.path
        return self.path


@dataclass(frozen=True)
class Source:
    """Where a change's files are read, as the change leaves them."""

    root: Path
    # The commit to read them at, or None for the files on disk.
    revision: str | None = None

    def read(self, path: str) -> str | None:
        """The file's text, or None when the change deletes it."""
        if self.revision is None:
            try:
                return (
                    (self.root / path).read_bytes().decode('utf-8', 'replace')
                )
            except OSError:
                return None
        result = subprocess.run(
            ['git', '-C', str(self.root), 'show', f'{self.revision}:{path}'],
            capture_output=True,
            check=False,
        )
        if result.returncode != 0:
            return None
        return result.stdout.decode('utf-8', 'replace')


@dataclass
class FileChange:
    """What a change does to one file."""

    # The lines it adds or removes, with their `+`/`-` marker, in diff order.
    lines: list[str] = field(default_factory=list)
    # The lines it adds, by their line number in the new file.
    added: dict[int, str] = field(default_factory=dict)
    # The file's path before a rename.
    old_path: str | None = None


@dataclass
class Change:
    """A change to a repository: its message and the files it touches."""

    files: dict[str, FileChange] = field(default_factory=dict)
    subject: str = ''
    body: list[str] = field(default_factory=list)
    source: Source | None = None

    _HUNK = re.compile(r'^@@ -\d+(?:,\d+)? \+(\d+)(?:,\d+)? @@')

    @classmethod
    def parse(
        cls,
        diff: str,
        *,
        subject: str = '',
        body: Iterable[str] = (),
        source: Source | None = None,
    ) -> Change:
        """The change a unified diff (`git diff`, `gh pr diff`) shows."""
        change = cls(subject=subject, body=list(body), source=source)
        current: FileChange | None = None
        in_hunk, line_number = False, 0
        # Only `\n` ends a line: a file's lines can hold `\r` or `\f`.
        for line in diff.split('\n'):
            if line.startswith('diff --git '):
                # The new path, which a deletion keeps as well.
                path = line.split(' b/', 1)[-1]
                current = change.files.setdefault(path, FileChange())
                in_hunk = False
            elif current is None:
                continue
            elif match := cls._HUNK.match(line):
                in_hunk, line_number = True, int(match.group(1))
            elif not in_hunk:
                if line.startswith('rename from '):
                    current.old_path = line[len('rename from ') :]
            elif line.startswith('+'):
                current.lines.append(line)
                current.added[line_number] = line[1:]
                line_number += 1
            elif line.startswith('-'):
                current.lines.append(line)
            elif line.startswith(' '):
                line_number += 1
        return change

    def read(self, path: str) -> str | None:
        """A file as the change leaves it, or None if nothing says where."""
        return self.source.read(path) if self.source else None


class BraveTree:
    """What a capability group can ask about Brave's own sources, and how a
    Brave file corresponds to the upstream file it overrides."""

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
    _PATCH_HEADER = re.compile(r'^\+?diff --git a/(\S+)', re.MULTILINE)

    @functools.cached_property
    def overridden(self) -> dict[str, set[str]]:
        """`upstream path -> kinds` of the upstream files Brave overrides,
        kinds being `chromium_src`, `lit_mangler`, `patch` and `plaster`."""
        root = Path(repository.brave.root)
        kinds: dict[str, set[str]] = {}
        for top in ('chromium_src', 'patches', 'rewrite'):
            for path in (root / top).rglob('*'):
                if path.is_file():
                    override = self.override(path.relative_to(root).as_posix())
                    if override:
                        kinds.setdefault(override[0], set()).add(override[1])
        return kinds

    def override(
        self, path: str, lines: Iterable[str] = ()
    ) -> tuple[str, str] | None:
        """`(upstream path, kind)` for a Brave file that overrides one, read
        from `lines` (a patch's added lines) or from the tree."""
        top, _, rest = path.partition('/')
        name = PurePosixPath(path).name
        if top == 'chromium_src':
            # Brave's own presubmits, which override nothing upstream.
            if name.startswith('PRESUBMIT'):
                return None
            if rest.endswith('.lit_mangler.ts'):
                return rest.removesuffix('.lit_mangler.ts'), 'lit_mangler'
            return rest, 'chromium_src'
        if top == 'rewrite' and rest.endswith('.yaml'):
            return rest.removesuffix('.yaml'), 'plaster'
        if top == 'patches' and name.endswith('.patch'):
            repo = PurePosixPath(rest).parent.as_posix()
            target = self._patch_target(path, lines)
            if target is None:
                return None
            if repo != '.':
                target = f'{repo}/{target}'
            return target, 'patch'
        return None

    def _patch_target(self, path: str, lines: Iterable[str]) -> str | None:
        """The file a patch applies to, relative to its repository: named by
        its `diff --git` header, else by its file name, whose `-`s stand for
        `/`s or for themselves."""
        for line in lines:
            if match := self._PATCH_HEADER.match(line):
                return match.group(1)
        patch = Path(repository.brave.root) / path
        try:
            with patch.open(encoding='utf-8', errors='replace') as file:
                if match := self._PATCH_HEADER.match(file.readline()):
                    return match.group(1)
        except OSError:
            pass
        repo = PurePosixPath(path).parent.relative_to('patches')
        base = Path(repository.chromium.root) / repo
        return self._decode(PurePosixPath(path).stem.split('-'), base)

    @classmethod
    def _decode(cls, parts: list[str], base: Path) -> str | None:
        for end in range(1, len(parts) + 1):
            component = base / '-'.join(parts[:end])
            if end == len(parts):
                if component.is_file():
                    return component.name
            elif component.is_dir():
                rest = cls._decode(parts[end:], component)
                if rest:
                    return f'{component.name}/{rest}'
        return None

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
        root = Path(repository.brave.root)
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


def _merge(merged: dict[str, list[str]], name: str, lines: list[str]) -> None:
    """Adds a file's lines to `merged`: as they are the first time, and the
    lines it doesn't have yet when another group or view matches it too."""
    if name not in merged:
        merged[name] = list(lines)
        return
    kept = merged[name]
    seen = set(kept)
    kept += [line for line in lines if line not in seen]


@functools.cache
def brave_tree() -> BraveTree:
    """The one `BraveTree` a run needs: Brave's sources don't change under
    it."""
    return BraveTree()


@dataclass(frozen=True)
class _File:
    """A file a group is matched against: its path as the group's repository
    names it, the path a match reports, and its change."""

    path: str
    shown: str
    change: FileChange
    # Reads the file as the change leaves it, or None when nothing can.
    read: Callable[[], str | None] | None = None


@dataclass(frozen=True)
class CapabilityGroup:
    """One `capability_groups` entry: when its rules apply.

    The fields are the textproto's (see the module docstring). The patterns
    are compiled on construction; one that doesn't compile is a `problem`,
    and keeps the group from matching.
    """

    rules: tuple[str, ...] = ()
    review_capability_skills: tuple[str, ...] = ()
    path_expression: tuple[str, ...] = ()
    path_expression_exclusion: tuple[str, ...] = ()
    content_patterns: tuple[str, ...] = ()
    diff_patterns: tuple[str, ...] = ()
    required_diff_patterns: tuple[str, ...] = ()
    ast_patterns: tuple[str, ...] = ()
    ast_language: str = ''
    overridden_by_brave: bool = False
    bytecode_adapted: bool = False
    brave_mentions: str = ''
    subject_patterns: tuple[str, ...] = ()
    body_patterns: tuple[str, ...] = ()
    subject_pattern_exclusion: tuple[str, ...] = ()
    tool: str = ''
    # Fields the group has that this module doesn't know.
    unknown: tuple[str, ...] = ()

    problems: list[str] = field(
        default_factory=list, init=False, repr=False, compare=False
    )
    _compiled: dict[str, list[re.Pattern]] = field(
        default_factory=dict, init=False, repr=False, compare=False
    )

    _PATTERNS = (
        'path_expression',
        'path_expression_exclusion',
        'content_patterns',
        'diff_patterns',
        'required_diff_patterns',
        'subject_patterns',
        'body_patterns',
        'subject_pattern_exclusion',
    )
    _FLAGS = ('overridden_by_brave', 'bytecode_adapted')
    _SINGULAR = ('ast_language', 'brave_mentions', 'tool')
    # Known, and read by no Brave tool: subagent manifests are Google's.
    _IGNORED = ('review_capability_subagents',)
    _FILE_FIELDS = (
        'path_expression',
        'path_expression_exclusion',
        'content_patterns',
        'diff_patterns',
        'required_diff_patterns',
        'ast_patterns',
        'overridden_by_brave',
        'bytecode_adapted',
        'brave_mentions',
    )
    # A Java source's class, as bytecode names it.
    _JAVA_CLASS = re.compile(r'(?:^|/)(org/chromium/[\w/]+)\.java$')

    def __post_init__(self) -> None:
        for name in (*self._PATTERNS, 'brave_mentions'):
            values = getattr(self, name)
            compiled = []
            for pattern in (values,) if isinstance(values, str) else values:
                if not pattern:
                    continue
                try:
                    compiled.append(re.compile(pattern))
                except re.error as e:
                    self.problems.append(f'{name} {pattern!r}: {e}')
            self._compiled[name] = compiled

    @classmethod
    def from_fields(cls, fields: dict[str, list]) -> CapabilityGroup:
        """A group from its parsed textproto block."""
        values: dict[str, object] = {}
        unknown = []
        known = set(cls.__dataclass_fields__) - {'unknown', 'problems'}
        for name, items in fields.items():
            if name in cls._IGNORED:
                continue
            if name not in known or name.startswith('_'):
                unknown.append(name)
            elif name in cls._FLAGS:
                values[name] = items[-1] in (True, 'true')
            elif name in cls._SINGULAR:
                values[name] = str(items[-1])
            else:
                values[name] = tuple(str(item) for item in items)
        return cls(**values, unknown=tuple(unknown))

    @property
    def asks_files(self) -> bool:
        """Whether the group asks anything of the files a change touches."""
        return any(getattr(self, name) for name in self._FILE_FIELDS)

    @property
    def targeted(self) -> bool:
        """Whether the group finds the lines its rules concern, rather than
        only the files they can apply to."""
        return bool(
            self.content_patterns or self.diff_patterns or self.ast_patterns
        )

    @property
    def unconditional(self) -> bool:
        """Whether the group applies to every change."""
        return not self.asks_files and not (
            self.subject_patterns
            or self.body_patterns
            or self.subject_pattern_exclusion
            or self.tool
        )

    def apply(
        self, change: Change, files: Iterable[_File], brave: BraveTree
    ) -> tuple[dict[str, list[str]], list[str]] | None:
        """`(files, body lines)` that matched, or None when the group doesn't
        apply. `files` is empty for a group that asks nothing of them."""
        if self.problems or self.tool:
            return None
        message = self._message(change)
        if message is None:
            return None
        if not self.asks_files:
            return {}, message
        matched: dict[str, list[str]] = {}
        for file in files:
            lines = self._lines(file, brave)
            if lines is not None:
                _merge(matched, file.shown, lines)
        return (matched, message) if matched else None

    def _message(self, change: Change) -> list[str] | None:
        """The body lines that matched, or None when the message rules the
        change out."""
        if any(
            p.search(change.subject)
            for p in self._compiled['subject_pattern_exclusion']
        ):
            return None
        subject, body = (
            self._compiled['subject_patterns'],
            self._compiled['body_patterns'],
        )
        if not subject and not body:
            return []
        hits = [
            line for line in change.body if any(p.search(line) for p in body)
        ]
        if hits or any(p.search(change.subject) for p in subject):
            return hits
        return None

    def _lines(self, file: _File, brave: BraveTree) -> list[str] | None:
        """The lines of `file` the group selects, or None if it doesn't
        apply to the file. Every changed line when it selects none."""
        change = file.change
        # A renamed file also counts by its old path, which is the one Brave
        # overrode before the rebase moved the override along.
        names = [file.path] + ([change.old_path] if change.old_path else [])
        if not any(self._names(name, brave) for name in names):
            return None
        if not all(
            any(p.search(line) for line in change.lines)
            for p in self._compiled['required_diff_patterns']
        ):
            return None
        content, diff = (
            self._compiled['content_patterns'],
            self._compiled['diff_patterns'],
        )
        lines = change.lines
        if content or diff or self.ast_patterns:
            lines = [
                line
                for line in change.lines
                if (
                    line.startswith('+')
                    and any(p.search(line[1:]) for p in content)
                )
                or any(p.search(line) for p in diff)
            ]
            if self.ast_patterns:
                spanned = self._ast_lines(file)
                lines += [
                    f'+{change.added[n]}'
                    for n in spanned
                    if f'+{change.added[n]}' not in lines
                ]
            if not lines:
                return None
        mentions = self._compiled['brave_mentions']
        if mentions and not self._mentioned(file.path, brave):
            lines = [line for line in lines if self._mentioned(line, brave)]
            if not lines:
                return None
        return lines

    def _names(self, path: str, brave: BraveTree) -> bool:
        include = self._compiled['path_expression']
        if include and not any(p.fullmatch(path) for p in include):
            return False
        if any(
            p.fullmatch(path)
            for p in self._compiled['path_expression_exclusion']
        ):
            return False
        if self.bytecode_adapted:
            java_class = self._JAVA_CLASS.search(path)
            if not java_class or java_class.group(1) not in brave.adapted:
                return False
        return not self.overridden_by_brave or path in brave.overridden

    def _mentioned(self, text: str, brave: BraveTree) -> bool:
        return any(
            m.group(1) in brave.words
            for p in self._compiled['brave_mentions']
            for m in p.finditer(text)
        )

    def _ast_lines(self, file: _File) -> list[int]:
        """The added lines that code matching `ast_patterns` spans."""
        if not file.change.added or file.read is None:
            return []
        language = self.ast_language or self.language(file.path)
        if not language:
            return []
        # A pattern applies to the files of the languages it parses in: a
        # group can hold a GN pattern and a C++ one.
        patterns = tuple(
            p for p in self.ast_patterns if _ast_parses(language, p) is None
        )
        text = file.read() if patterns else None
        if text is None:
            return []
        try:
            spans = _ast_spans(language, patterns, file.path, text)
        except ValueError as e:
            _log(f'WARNING: ast_patterns on {file.path}: {e}')
            return []
        return sorted(
            n for n in file.change.added if any(a <= n <= b for a, b in spans)
        )

    @staticmethod
    def language(path: str) -> str | None:
        """ast-grep's language for a file."""
        name = PurePosixPath(path).name
        return _AST_LANGUAGES.get(
            name if name == '.gn' else PurePosixPath(name).suffix
        )


def _ast_rule(entry: str) -> dict:
    """An `ast_patterns` entry as an ast-grep rule: a YAML mapping (`{...}`)
    is one, as when a C++ pattern needs a `context` and `selector` to parse
    as an expression; anything else is a `pattern`.

    Raises:
        ValueError: for a mapping YAML can't read.
    """
    if not entry.lstrip().startswith('{'):
        return {'pattern': entry}
    return _mapping(entry, repr(entry))


@functools.cache
def _ast_parses(language: str, entry: str) -> str | None:
    """Why ast-grep can't read an `ast_patterns` entry in `language`, or
    None when it can."""
    try:
        _ast_spans(language, (entry,), '', '')
    except ValueError as e:
        return str(e)
    return None


@functools.lru_cache(maxsize=4096)
def _ast_spans(
    language: str, patterns: tuple[str, ...], path: str, text: str
) -> tuple[tuple[int, int], ...]:
    """The 1-based line ranges of the code in `text` matching one of
    `patterns`.

    Raises:
        ValueError: for a pattern ast-grep rejects.
    """
    # Imported here: plaster is large, and only AST patterns need it.
    import plaster  # pylint: disable=import-outside-toplevel

    source = text
    if path.endswith(_CXX_SUFFIXES):
        source = plaster.CxxMacrosEraser(
            plaster.BlankForParseOptions(
                macros=True,
                string_adjacent_macros=True,
                metadata_header_macros=True,
            )
        ).erase(text)
    rule = yaml.safe_dump({'any': [_ast_rule(p) for p in patterns]})
    try:
        matches = plaster.run_ast_grep(
            language=language, rule_body=rule, source=source
        )
    except plaster.AstGrepError as e:
        raise ValueError(str(e)) from e
    data = source.encode('utf-8')
    return tuple(
        (
            data.count(b'\n', 0, m.start) + 1,
            data.count(b'\n', 0, max(m.start, m.start + m.length - 1)) + 1,
        )
        for m in matches
    )


@dataclass(frozen=True)
class _Metadata:
    """The `agentic_review` of one DIR_METADATA or COMMON_METADATA file."""

    groups: tuple[CapabilityGroup, ...] = ()
    exclude_rules: tuple[str, ...] = ()
    mixins: tuple[str, ...] = ()

    @classmethod
    def read(cls, path: Path) -> _Metadata:
        """The file's metadata; none if it doesn't exist.

        Raises:
            ValueError: on a file that isn't the text format.
        """
        try:
            text = path.read_bytes().decode('utf-8')
        except OSError:
            return cls()
        fields = _TextProto.parse(text, str(path))
        groups, excluded = [], []
        for block in fields.get('agentic_review', []):
            if not isinstance(block, dict):
                continue
            for group in block.get('capability_groups', []):
                if isinstance(group, dict):
                    groups.append(CapabilityGroup.from_fields(group))
            excluded += [str(r) for r in block.get('exclude_rules', [])]
        return cls(
            groups=tuple(groups),
            exclude_rules=tuple(excluded),
            mixins=tuple(
                str(m).removeprefix('//') for m in fields.get('mixins', [])
            ),
        )


@dataclass
class Match:
    """A rule a change needs, and why."""

    rule: Rule
    # The changed lines of each file the rule applies to, or nothing when the
    # rule applies to the change as a whole.
    files: dict[str, list[str]] = field(default_factory=dict)
    # The lines of the change's body that matched.
    message: list[str] = field(default_factory=list)
    # Whether a group found the lines the rule concerns (see
    # `CapabilityGroup.targeted`): a rule only scoped to the files can be
    # about any of their lines.
    targeted: bool = False


@dataclass(frozen=True)
class _Scoped:
    """A group as a file's directory inherits it."""

    group: CapabilityGroup
    # The repository its paths are relative to.
    root: Path


class KnowledgeBase:
    """A directory of rules, and the `COMMON_METADATA` files under it whose
    groups apply them to any change."""

    def __init__(self, root: Path):
        self._root = Path(root)
        self._repository = _repository_root(self._root)
        self._groups: list[CapabilityGroup] = []
        for path in sorted(self._root.rglob('COMMON_METADATA')):
            try:
                self._groups += _Metadata.read(path).groups
            except ValueError as e:
                _log(f'WARNING: {e}')

    @property
    def groups(self) -> list[CapabilityGroup]:
        return self._groups

    @property
    def rules(self) -> list[str]:
        """The rule files under the root, relative to their repository."""
        return sorted(
            p.relative_to(self._repository).as_posix()
            for p in self._root.rglob('*.md')
        )

    def match(self, change: Change) -> list[Match]:
        """The rules `change` needs, in the order the groups name them."""
        files = [
            (_File(path, path, fc, functools.partial(change.read, path)), _NONE)
            for path, fc in change.files.items()
        ]
        return _apply(
            change,
            [(_Scoped(g, self._repository), files) for g in self._groups],
            brave_tree(),
        )


def _repository_root(path: Path) -> Path:
    """The root of the git repository holding `path`."""
    path = Path(path)
    for directory in (path, *path.parents):
        if (directory / '.git').exists():
            return directory
    return path


_RULE_CACHE: dict[tuple[Path, str], Rule | None] = {}


def _rule(root: Path, path: str) -> Rule | None:
    """The rule at `path` in the repository at `root`, or None (logged) when
    it can't be read."""
    key = (root, path)
    if key not in _RULE_CACHE:
        try:
            _RULE_CACHE[key] = Rule.read(root, path)
        except (OSError, ValueError) as e:
            _log(f'WARNING: rule {path}: {e}')
            _RULE_CACHE[key] = None
    return _RULE_CACHE[key]


# No rules excluded.
_NONE: frozenset[tuple[Path, str]] = frozenset()

# A group, and the files in its scope with the rules each excludes, as
# `(repository root, path)` pairs.
_Scope = tuple[_Scoped, list[tuple[_File, frozenset[tuple[Path, str]]]]]


def _apply(
    change: Change, scopes: list[_Scope], brave: BraveTree
) -> list[Match]:
    """The rules the groups give, each matched against the files in its
    scope, merged by rule."""
    matches: dict[tuple[Path, str], Match] = {}
    for scoped, scope in scopes:
        result = scoped.group.apply(change, (f for f, _ in scope), brave)
        if result is None:
            continue
        matched, message = result
        for path in scoped.group.rules:
            key = (scoped.root, path)
            # A file whose directory excludes the rule doesn't count for it.
            kept = [f.shown for f, excluded in scope if key not in excluded]
            shown = {name: matched[name] for name in kept if name in matched}
            if (matched and not shown) or (scope and not kept):
                continue
            rule = _rule(scoped.root, path)
            if rule is None:
                continue
            found = matches.setdefault(key, Match(rule))
            found.targeted |= scoped.group.targeted
            for name, lines in shown.items():
                _merge(found.files, name, lines)
            found.message += [m for m in message if m not in found.message]
    return list(matches.values())


class _Tree:
    """The `agentic_review` a file inherits from the DIR_METADATA of its
    directories, from the source root down, as `dirmd` computes it: each
    repository on the way (a directory with `.git`) resolves its own paths and
    mixins, and imports its own knowledge base at its root."""

    def __init__(self) -> None:
        self._src = Path(repository.chromium.root)
        self._brave = Path(repository.brave.root)
        self._metadata: dict[Path, _Metadata] = {}
        self._imported: dict[Path, list[CapabilityGroup]] = {}

    def scope(
        self, path: str
    ) -> tuple[list[_Scoped], frozenset[tuple[Path, str]], dict[Path, str]]:
        """For a path relative to the source root: the groups it inherits,
        the rules it excludes, and its path in each repository on the way."""
        groups: list[_Scoped] = []
        excluded: set[tuple[Path, str]] = set()
        names: dict[Path, str] = {}
        parts = PurePosixPath(path).parts
        root = self._src
        for depth in range(len(parts)):
            directory = self._src.joinpath(*parts[:depth])
            if depth and (directory / '.git').exists():
                root = directory
            if root not in names:
                names[root] = '/'.join(parts[depth:])
                groups += [_Scoped(g, root) for g in self._import(root)]
            metadata = self._read(directory / 'DIR_METADATA')
            for mixin in metadata.mixins:
                groups += [
                    _Scoped(g, root) for g in self._read(root / mixin).groups
                ]
            groups += [_Scoped(g, root) for g in metadata.groups]
            excluded.update((root, rule) for rule in metadata.exclude_rules)
        return _unique(groups), frozenset(excluded), names

    def _import(self, root: Path) -> list[CapabilityGroup]:
        """A repository's own knowledge base, which all its files import."""
        if root not in self._imported:
            rules = None
            if root.resolve() == self._brave.resolve():
                rules = _BRAVE_RULES
            elif root.resolve() == self._src.resolve():
                rules = _CHROMIUM_RULES
            groups: list[CapabilityGroup] = []
            if rules:
                for path in sorted((root / rules).rglob('COMMON_METADATA')):
                    groups += self._read(path).groups
            self._imported[root] = groups
        return self._imported[root]

    def _read(self, path: Path) -> _Metadata:
        if path not in self._metadata:
            try:
                self._metadata[path] = _Metadata.read(path)
            except ValueError as e:
                _log(f'WARNING: {e}')
                self._metadata[path] = _Metadata()
        return self._metadata[path]


def _unique(groups: list[_Scoped]) -> list[_Scoped]:
    """`groups` without repeats: a mixin both imported and mixed in counts
    once."""
    seen: set[tuple[int, Path]] = set()
    unique = []
    for scoped in groups:
        key = (id(scoped.group), scoped.root)
        if key not in seen:
            seen.add(key)
            unique.append(scoped)
    return unique


def review(change: Change) -> list[Match]:
    """The Brave and upstream rules a change to brave-core needs, most
    severe first.

    Each file is matched as Brave's tree sees it, and as `brave/<path>` in
    Chromium's; a file overriding an upstream one also as that file, under
    its DIR_METADATA, with the lines a patch adds to it. Matches name the
    Brave file."""
    tree = _Tree()
    brave = brave_tree()
    scopes: dict[tuple[int, Path], _Scope] = {}
    for path, fc in change.files.items():
        read = functools.partial(change.read, path)
        views = [(f'{repository.BRAVE_DIR_NAME}/{path}', fc, read)]
        override = brave.override(path, fc.lines)
        if override:
            upstream, kind = override
            if kind == 'patch':
                views.append((upstream, _patched(fc), None))
            else:
                # A plaster's lines are YAML: no AST to read.
                views.append(
                    (upstream, fc, None if kind == 'plaster' else read)
                )
        for view, view_change, view_read in views:
            groups, excluded, names = tree.scope(view)
            for scoped in groups:
                file = _File(names[scoped.root], path, view_change, view_read)
                key = (id(scoped.group), scoped.root)
                scopes.setdefault(key, (scoped, []))[1].append((file, excluded))
    matches = _apply(change, list(scopes.values()), brave)
    order = {s: i for i, s in enumerate(SEVERITIES)}
    brave_root = Path(repository.brave.root).resolve()
    return sorted(
        matches,
        key=lambda m: (
            order.get(m.rule.severity, len(order)),
            m.rule.repository.resolve() != brave_root,
            m.rule.name,
        ),
    )


def _patched(patch: FileChange) -> FileChange:
    """The change a patch's added lines make to the file it patches."""
    inner = FileChange()
    for line in patch.lines:
        if not line.startswith('+') or line.startswith(('++++', '+---')):
            continue
        text = line[1:]
        if text.startswith(('+', '-')):
            inner.lines.append(text)
            if text.startswith('+'):
                inner.added[len(inner.added) + 1] = text[1:]
    return inner


class _Lint:
    """The problems of Brave's knowledge bases, and of the upstream metadata
    Brave's reviews load."""

    def __init__(self) -> None:
        self._brave = Path(repository.brave.root)
        self._src = Path(repository.chromium.root)
        self.problems: list[str] = []

    def run(self) -> list[str]:
        # Untracked files too, so that a new rule is linted before it's added.
        files = set(
            repository.brave.run_git(
                'ls-files', '-z', '--cached', '--others', '--exclude-standard'
            ).split('\0')
        )
        metadata = sorted(
            p
            for p in files
            if PurePosixPath(p).name in ('COMMON_METADATA', 'DIR_METADATA')
        )
        named: set[str] = set()
        # A knowledge base's root holds a directory of rules per topic, each
        # with its COMMON_METADATA.
        roots: set[PurePosixPath] = set()
        for path in metadata:
            named |= self._metadata(self._brave, path)
            if PurePosixPath(path).name == 'COMMON_METADATA':
                roots.add(PurePosixPath(path).parent.parent)
        for path in sorted(
            (self._src / _CHROMIUM_RULES).rglob('COMMON_METADATA')
        ):
            self._metadata(self._src, path.relative_to(self._src).as_posix())
        names: dict[str, str] = {}
        for path in sorted(files):
            if path.endswith('.md') and any(
                PurePosixPath(path).is_relative_to(root) for root in roots
            ):
                self._rule(path, named, names)
        return self.problems

    def _problem(self, where: str, what: str) -> None:
        self.problems.append(f'{where}: {what}')

    def _metadata(self, root: Path, path: str) -> set[str]:
        """Checks one metadata file; returns the rules it names."""
        try:
            parsed = _Metadata.read(root / path)
        except ValueError as e:
            self._problem(path, str(e))
            return set()
        named = set()
        for i, group in enumerate(parsed.groups):
            where = f'{path}: capability_groups[{i}]'
            for name in group.unknown:
                self._problem(where, f'unknown field {name}')
            for problem in group.problems:
                self._problem(where, problem)
            if group.ast_patterns:
                self._ast(where, group)
            if not group.rules and not group.review_capability_skills:
                self._problem(where, 'names no rules or skills')
            for kind, paths, suffix in (
                ('rules', group.rules, '.md'),
                (
                    'review_capability_skills',
                    group.review_capability_skills,
                    'SKILL.md',
                ),
            ):
                if len(set(paths)) != len(paths):
                    self._problem(where, f'duplicate {kind}')
                for named_path in paths:
                    if not named_path.endswith(suffix):
                        self._problem(
                            where, f'{named_path}: not a {suffix} file'
                        )
                    elif not (root / named_path).is_file():
                        self._problem(where, f'{named_path}: no such file')
            named.update(group.rules)
        for rule in parsed.exclude_rules:
            if not (root / rule).is_file():
                self._problem(path, f'exclude_rules {rule}: no such file')
        return named

    def _ast(self, where: str, group: CapabilityGroup) -> None:
        """Checks that ast-grep reads each of the group's patterns, in its
        `ast_language` or, without one, in a language files can have."""
        languages = (
            [group.ast_language]
            if group.ast_language
            else sorted(set(_AST_LANGUAGES.values()))
        )
        for entry in group.ast_patterns:
            errors = [_ast_parses(language, entry) for language in languages]
            if all(errors):
                self._problem(
                    where, f'ast_patterns {entry!r}: {"; ".join(errors)}'
                )

    def _rule(self, path: str, named: set[str], names: dict[str, str]) -> None:
        try:
            rule = Rule.read(self._brave, path)
        except (OSError, ValueError) as e:
            self._problem(path, str(e))
            return
        for key in ('name', 'rubric', 'severity'):
            if not getattr(rule, key):
                self._problem(path, f'no `{key}`')
        if rule.name.rsplit('/', 1)[-1] != PurePosixPath(path).stem:
            self._problem(
                path, f'name {rule.name} does not end in the file stem'
            )
        if rule.name in names:
            self._problem(path, f'name {rule.name} is also {names[rule.name]}')
        names[rule.name] = path
        if path not in named:
            self._problem(path, 'no capability group names the rule')
        for part in ('**Rule.**', '**Why.**'):
            if part not in rule.body:
                self._problem(path, f'no {part} paragraph')


def lint() -> list[str]:
    """The problems of every knowledge base in brave-core, and of the upstream
    metadata Brave's reviews load: unparseable files, unknown fields, patterns
    that don't compile, groups naming missing files, rules no group names,
    duplicate rule names, and rules missing `name`, `rubric`, `severity` or
    their **Rule.** and **Why.** paragraphs."""
    return _Lint().run()


def _main_match(args: argparse.Namespace) -> int:
    if args.diff:
        text = (
            sys.stdin.read()
            if args.diff == '-'
            else Path(args.diff).read_bytes().decode('utf-8', 'replace')
        )
    else:
        base = repository.brave.run_git('merge-base', 'origin/master', 'HEAD')
        text = repository.brave.run_git('diff', base, no_trim=True)
    change = Change.parse(
        text, source=Source(Path(repository.brave.root), args.head)
    )
    matches = review(change)
    json.dump(
        [
            {
                'name': m.rule.name,
                'severity': m.rule.severity,
                'rubric': m.rule.rubric,
                'description': m.rule.description,
                'file': str(m.rule.file.resolve()),
                'url': m.rule.url,
                'summary': m.rule.summary,
                'targeted': m.targeted,
                'files': m.files,
            }
            for m in matches
        ],
        sys.stdout,
        indent=1,
    )
    print()
    return 0


def _main_lint(_: argparse.Namespace) -> int:
    problems = lint()
    for problem in problems:
        print(problem)
    print(f'{len(problems)} problems', file=sys.stderr)
    return 1 if problems else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    commands = parser.add_subparsers(dest='command', required=True)
    match = commands.add_parser(
        'match',
        help='Print, as JSON, the rules a brave-core change needs.',
    )
    match.add_argument(
        'diff',
        nargs='?',
        help='A unified diff, or - for stdin; by default the working tree '
        'against its merge base with origin/master.',
    )
    match.add_argument(
        '--head',
        help='The commit the diff leads to, to read its files at; by default '
        'the working tree.',
    )
    match.set_defaults(run=_main_match)
    commands.add_parser(
        'lint', help='Print the problems of the knowledge bases.'
    ).set_defaults(run=_main_lint)
    args = parser.parse_args()
    return args.run(args)


if __name__ == '__main__':
    sys.exit(main())
