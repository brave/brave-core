#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Lists what `chromium_src` Python overrides need upstream that a range drops.

A Python override runs inside the upstream module that inlines it
(`inline_chromium_src_override`), so it breaks where upstream drops a name it
binds to, and stops matching where upstream drops text it looks for. Neither
fails until something imports the module, which the build may never do. Only
what the range changes is read. For an override whose upstream module the range
changes:

- `name`: a function or method it overrides, or an upstream global it reads,
  that the module defined at the old tag and doesn't at the new one;
- `params`: an overridden function or method whose parameters changed;
- `text`: a string literal of the override that leaves the module.

And for every override, `text` also lists a distinctive literal the module never
held (text it matches in another upstream file) that the range removes from
every file it changes under the override's top two directories.

    vpython3 tools/cr/codehealth/scripts/python_reach.py OLD..NEW
"""

from __future__ import annotations

import argparse
import ast
from collections import defaultdict
from collections.abc import Iterable
from dataclasses import dataclass, field
import json
from pathlib import Path
import re
import subprocess
import symtable
import sys

import _boot  # noqa: F401  -- adds tools/cr to sys.path
from plaster import AST_GREP_BIN  # noqa: E402  -- after _boot
import repository  # noqa: E402  -- after _boot

# What `PythonSource` reads, as ast-grep rules: functions decorated with an
# `override_utils` helper, module-level functions and classes, the methods of
# module-level classes, and string literals other than docstrings and keyword
# arguments (`encoding='utf-8'` names no upstream text).
_RULES = """
id: override
language: python
rule:
  kind: decorated_definition
  all:
    - has:
        kind: decorator
        has:
          kind: call
          all:
            - has:
                field: function
                regex: '(^|\\.)override_(function|method)$'
            - has: {field: arguments, pattern: $ARGS}
    - has:
        field: definition
        kind: function_definition
        has: {field: name, pattern: $NAME}
---
id: def
language: python
rule:
  kind: function_definition
  all:
    - has: {field: name, pattern: $NAME}
    - has: {field: parameters, pattern: $PARAMS}
  not:
    inside:
      stopBy: end
      any: [{kind: function_definition}, {kind: class_definition}]
---
id: method
language: python
rule:
  kind: function_definition
  all:
    - has: {field: name, pattern: $NAME}
    - has: {field: parameters, pattern: $PARAMS}
    - inside:
        stopBy: end
        kind: class_definition
        has: {field: name, pattern: $OWNER}
  not:
    any:
      - inside: {stopBy: end, kind: function_definition}
      - inside:
          stopBy: end
          kind: class_definition
          inside: {stopBy: end, kind: class_definition}
---
id: class
language: python
rule:
  kind: class_definition
  has: {field: name, pattern: $NAME}
  not:
    inside:
      stopBy: end
      any: [{kind: function_definition}, {kind: class_definition}]
---
id: literal
language: python
rule:
  kind: string
  pattern: $TEXT
  not:
    any:
      - has: {kind: interpolation}
      - inside: {kind: expression_statement}
      - inside: {kind: keyword_argument}
"""


@dataclass(frozen=True)
class Override:
    """A function an override replaces: `name` in `scope`, which is
    `globals()` or the name of the class it patches."""

    scope: str
    name: str


@dataclass(frozen=True)
class Literal:
    """A string literal: its value, and its text between the quotes, which
    differ when it has escapes."""

    value: str
    body: str

    def count(self, text: str) -> int:
        """How often upstream `text` holds the literal, written either way."""
        return max(text.count(self.value), text.count(self.body))


class PythonSource:
    """The declarations and literals of one Python file, read with
    ast-grep."""

    def __init__(self, text: str) -> None:
        self.text = text
        self.overrides: list[Override] = []
        self.functions: dict[str, str] = {}
        self.methods: dict[tuple[str, str], str] = {}
        self.classes: set[str] = set()
        self.literals: list[Literal] = []
        for match in self._scan(text):
            captured = {
                name: variable['text']
                for name, variable in match['metaVariables']['single'].items()
            }
            rule = match['ruleId']
            if rule == 'override':
                if override := self._override(captured):
                    self.overrides.append(override)
            elif rule == 'def':
                self.functions[captured['NAME']] = self._params(
                    captured['PARAMS']
                )
            elif rule == 'method':
                self.methods[(captured['OWNER'], captured['NAME'])] = (
                    self._params(captured['PARAMS'])
                )
            elif rule == 'class':
                self.classes.add(captured['NAME'])
            elif literal := self._literal(captured['TEXT']):
                if literal not in self.literals:
                    self.literals.append(literal)

    @staticmethod
    def _scan(text: str) -> list[dict]:
        output = subprocess.run(
            [
                str(AST_GREP_BIN),
                'scan',
                '--inline-rules',
                _RULES,
                '--stdin',
                '--json=compact',
            ],
            input=text,
            capture_output=True,
            text=True,
            check=True,
        ).stdout
        return json.loads(output or '[]')

    @staticmethod
    def _override(captured: dict[str, str]) -> Override | None:
        """The function a decorator replaces: its first argument is the scope,
        and `name=` renames the target."""
        call = ast.parse(f'f{captured["ARGS"]}', mode='eval').body
        if not call.args:
            return None
        name = captured['NAME']
        for keyword in call.keywords:
            if keyword.arg == 'name' and isinstance(
                keyword.value, ast.Constant
            ):
                name = keyword.value.value
        return Override(ast.unparse(call.args[0]), name)

    @staticmethod
    def _params(text: str) -> str:
        """Parameters with their layout normalised: `(a, b)`."""
        text = re.sub(r'\s+', ' ', text)
        return re.sub(r',? ?\)$', ')', text.replace('( ', '('))

    @staticmethod
    def _literal(text: str) -> Literal | None:
        """A literal worth looking for upstream: a one-line `str` of 3 or more
        characters."""
        try:
            value = ast.literal_eval(text)
        except (ValueError, SyntaxError):
            return None
        if not isinstance(value, str) or len(value) < 3 or '\n' in value:
            return None
        quote = re.match(r'[A-Za-z]*("""|\'\'\'|"|\')', text).group(1)
        body = text[text.index(quote) + len(quote) : -len(quote)]
        return Literal(value, body)

    def module_names(self) -> set[str] | None:
        """The names the module binds at its top level; None if it doesn't
        parse."""
        try:
            table = symtable.symtable(self.text, '<upstream>', 'exec')
        except SyntaxError:
            return None
        return {
            s.get_name()
            for s in table.get_symbols()
            if s.is_assigned() or s.is_imported() or s.is_namespace()
        }

    def globals_read(self) -> set[str]:
        """The module-level names the file reads, in any scope, whether or
        not it also binds them."""
        names = set()
        tables = [symtable.symtable(self.text, '<override>', 'exec')]
        while tables:
            table = tables.pop()
            module = table.get_type() == 'module'
            for symbol in table.get_symbols():
                if symbol.is_referenced() and (module or symbol.is_global()):
                    names.add(symbol.get_name())
            tables.extend(table.get_children())
        return names


@dataclass(frozen=True)
class Anchor:
    """Something an override needs from upstream that the range drops."""

    override: str
    kind: str  # 'name', 'params' or 'text'
    anchor: str
    old: str
    new: str
    # For a literal matched in other files: the files that held it.
    where: tuple[str, ...] = field(default=())

    def __str__(self) -> str:
        line = (
            f'{self.kind:<6} {self.anchor}: {self.old} -> {self.new}'
            f'  {self.override}'
        )
        if self.where:
            line += f'  (in {", ".join(self.where)})'
        return line


_KINDS = ('name', 'params', 'text')

# A literal looked for across the range: long, and not a plain word, which
# matches too many files to tell anything.
_DISTINCTIVE = re.compile(r'(?=.{8})(?=.*[^\w])')


def module_anchors(
    override: str, source: PythonSource, old: PythonSource, new: PythonSource
) -> list[Anchor]:
    """What the override needs from its upstream module that `new` dropped
    since `old`."""
    anchors = []
    old_names, new_names = old.module_names(), new.module_names()
    targets = set()
    for target in source.overrides:
        if target.scope == 'globals()':
            targets.add(target.name)
            label = key = target.name
            olds, news = old.functions, new.functions
            gone = (
                old_names is not None
                and new_names is not None
                and key in old_names
                and key not in new_names
            )
        elif target.scope in old.classes:
            label = f'{target.scope}.{target.name}'
            key = (target.scope, target.name)
            olds, news = old.methods, new.methods
            gone = key in olds and key not in news
        else:
            continue  # A class or module from elsewhere.
        if gone:
            anchors.append(Anchor(override, 'name', label, 'defined', 'gone'))
        elif key in olds and key in news and olds[key] != news[key]:
            anchors.append(
                Anchor(override, 'params', label, olds[key], news[key])
            )
    if old_names is not None and new_names is not None:
        for name in sorted(source.globals_read() - targets):
            if name in old_names and name not in new_names:
                anchors.append(
                    Anchor(override, 'name', name, 'defined', 'gone')
                )
    for literal in source.literals:
        before, after = literal.count(old.text), literal.count(new.text)
        if before and not after:
            anchors.append(
                Anchor(override, 'text', repr(literal.value), str(before), '0')
            )
    return anchors


def range_text_anchors(
    override: str,
    literals: list[Literal],
    texts: dict[str, tuple[str | None, str | None]],
) -> list[Anchor]:
    """The literals the range removes from every changed file that held
    them, given each file's old and new text."""
    anchors = []
    for literal in literals:
        before = {
            path: literal.count(old)
            for path, (old, _) in texts.items()
            if old and literal.count(old)
        }
        after = sum(literal.count(new) for _, new in texts.values() if new)
        if before and not after:
            anchors.append(
                Anchor(
                    override,
                    'text',
                    repr(literal.value),
                    str(sum(before.values())),
                    '0',
                    tuple(sorted(before)),
                )
            )
    return anchors


def ere_escape(text: str) -> str:
    """`text` as a POSIX extended regex, for `git diff -G`."""
    return re.sub(r'([\\^$.|?*+()\[\]{}])', r'\\\1', text)


class PythonReach:
    """Checks each inlined Python override against what the range
    changes."""

    _DIFF_PATH = re.compile(r'^diff --git a/(\S+) ', re.M)
    _INLINE = 'inline_chromium_src_override'

    def __init__(self, old: str, new: str) -> None:
        self._old, self._new = old, new

    def inlined(self) -> dict[str, tuple[str, str]]:
        """The upstream files Brave inlines an override into, by their path
        under `src`: the sub-repository holding each (`''` for Chromium's
        own) and its path there."""
        brave = repository.brave.root
        inlined = {}
        patches = brave / 'patches'
        for patch in sorted(patches.rglob('*.patch')):
            text = patch.read_bytes().decode('utf-8', 'replace')
            if self._INLINE not in text:
                continue
            repo = patch.parent.relative_to(patches).as_posix()
            repo = '' if repo == '.' else repo
            for path in self._DIFF_PATH.findall(text):
                full = f'{repo}/{path}' if repo else path
                inlined[full] = (repo, path)
        rewrite = brave / 'rewrite'
        for spec in sorted(rewrite.rglob('*.yaml')):
            if self._INLINE in spec.read_bytes().decode('utf-8', 'replace'):
                path = spec.relative_to(rewrite).as_posix()[: -len('.yaml')]
                inlined.setdefault(path, ('', path))
        return inlined

    def run(self) -> list[Anchor]:
        """Everything the range drops that an override needs."""
        chromium_src = repository.brave.root / 'chromium_src'
        by_repo = defaultdict(dict)
        for full, (repo, path) in self.inlined().items():
            override = chromium_src / full
            if override.suffix == '.py' and override.is_file():
                by_repo[repo][path] = (
                    full,
                    PythonSource(override.read_text(encoding='utf-8')),
                )
        anchors = []
        for repo, overrides in sorted(by_repo.items()):
            anchors += self._check_repo(repo, overrides)
        return sorted(
            anchors, key=lambda a: (_KINDS.index(a.kind), a.override, a.anchor)
        )

    def _check_repo(
        self, repo: str, overrides: dict[str, tuple[str, PythonSource]]
    ) -> list[Anchor]:
        commits = [self._commit(repo, tag) for tag in (self._old, self._new)]
        if None in commits or commits[0] == commits[1]:
            return []
        old, new = commits
        git_dir = repository.chromium.root / repo
        changed = set(
            self._git(git_dir, 'diff', '--name-only', '--no-renames', old, new)
        )
        modules = _read(git_dir, [(c, p) for p in overrides for c in commits])
        anchors = []
        # Text an override matches in other files is looked for in its own
        # part of the tree: a generic pattern also matches unrelated files.
        wide = defaultdict(dict)
        for path, (full, source) in overrides.items():
            old_text, new_text = modules[(old, path)], modules[(new, path)]
            if path in changed and old_text and new_text:
                anchors += module_anchors(
                    full, source, PythonSource(old_text), PythonSource(new_text)
                )
            literals = [
                literal
                for literal in source.literals
                if _DISTINCTIVE.match(literal.value)
                and not (old_text and literal.count(old_text))
            ]
            if literals:
                wide['/'.join(path.split('/')[:2])][full] = literals
        for subtree, literals_by_override in sorted(wide.items()):
            texts = self._changed_texts(
                git_dir, old, new, subtree, literals_by_override.values()
            )
            for full, literals in literals_by_override.items():
                anchors += range_text_anchors(full, literals, texts)
        return anchors

    def _changed_texts(
        self,
        git_dir: Path,
        old: str,
        new: str,
        subtree: str,
        literals: Iterable[list[Literal]],
    ) -> dict[str, tuple[str | None, str | None]]:
        """The old and new text of the files under `subtree` whose changed
        lines hold one of `literals`."""
        wanted = {lit for lits in literals for lit in lits}
        pattern = '|'.join(
            ere_escape(text)
            for lit in sorted(wanted, key=lambda lit: lit.value)
            for text in dict.fromkeys((lit.value, lit.body))
        )
        files = self._git(
            git_dir,
            'diff',
            '--name-only',
            '--no-renames',
            '-G',
            pattern,
            old,
            new,
            '--',
            subtree,
        )
        blobs = _read(git_dir, [(c, f) for f in files for c in (old, new)])
        return {f: (blobs[(old, f)], blobs[(new, f)]) for f in files}

    def _commit(self, repo: str, tag: str) -> str | None:
        """The commit of `repo` that Chromium's `tag` pins, or `tag` itself
        for Chromium."""
        if not repo:
            return tag
        entry = self._git(repository.chromium.root, 'ls-tree', tag, '--', repo)
        fields = entry[0].split() if entry else []
        return fields[2] if len(fields) > 2 and fields[1] == 'commit' else None

    @staticmethod
    def _git(cwd: Path, *args: str) -> list[str]:
        return subprocess.run(
            ['git', '-C', str(cwd), *args],
            capture_output=True,
            text=True,
            check=True,
        ).stdout.splitlines()


def _read(
    git_dir: Path, wanted: list[tuple[str, str]]
) -> dict[tuple[str, str], str | None]:
    """`git cat-file --batch` for every `commit:path`, None when missing."""
    request = ''.join(f'{commit}:{path}\n' for commit, path in wanted).encode()
    output = subprocess.run(
        ['git', '-C', str(git_dir), 'cat-file', '--batch'],
        input=request,
        capture_output=True,
        check=True,
    ).stdout
    texts, pos = {}, 0
    for key in wanted:
        end = output.index(b'\n', pos)
        header = output[pos:end].split()
        pos = end + 1
        if header[-1] == b'missing':
            texts[key] = None
            continue
        size = int(header[2])
        texts[key] = output[pos : pos + size].decode('utf-8', 'replace')
        pos += size + 1
    return texts


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('range', help='OLD..NEW Chromium tags.')
    args = parser.parse_args()
    old, _, new = args.range.partition('..')
    if not old or not new:
        parser.error('expected OLD..NEW')
    anchors = PythonReach(old, new).run()
    print(f'kind   anchor: old -> new  override ({len(anchors)} dropped)')
    for anchor in anchors:
        print(anchor)
    return 0


if __name__ == '__main__':
    sys.exit(main())
