#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Resolves the upstream Java names Brave hardcodes as strings, at two tags.

Brave reaches into upstream Java by name in `BraveReflectionUtil` calls and in
`BytecodeTest` assertions. No compiler checks those names, and neither fails
loudly when upstream renames a member or changes its parameters, so this
resolves each one against the upstream source at an old and a new tag, and
sorts it into a bucket:

    broke        resolves at the old tag, not at the new one: the lift broke it
    debt         resolves at neither: broken before the range
    review       matches only through a type variable, or only by name, in
                 a class whose file the range changed
    reviewed     the same, in a class whose file the range didn't change
    unresolvable its class isn't in the tree (e.g. Material, generated code)
    fixed        resolves at the new tag, not at the old one
    fine         resolves at both

Classes whose name starts with `Brave` resolve against the working tree, at
both ends. Declarations are read with ast-grep's Java grammar. Exits with 1
when anything broke.

    vpython3 tools/cr/codehealth/scripts/java_names.py [OLD..NEW]
        [--json|--plain]
"""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import asdict, dataclass
import json
from pathlib import Path
import re
import subprocess
import sys

import _boot  # noqa: F401  -- adds tools/cr to sys.path
from plaster import AST_GREP_BIN  # noqa: E402  -- after _boot
from rich.table import Table  # noqa: E402  -- after _boot
import repository  # noqa: E402  -- after _boot
from terminal import console  # noqa: E402  -- after _boot
from versioning import Version  # noqa: E402  -- after _boot

_BYTECODE_TEST = Path(
    'android/javatests/org/chromium/chrome/browser/BytecodeTest.java'
)

# Comments, text blocks, and string and char literals.
_COMMENT_OR_STRING = re.compile(
    r'//[^\n]*|/\*.*?\*/|"""(?:.|\n)*?"""|"(?:\\.|[^"\\\n])*"|'
    r"'(?:\\.|[^'\\\n])*'",
    re.DOTALL,
)

# A member's immediate owner: the declaration whose body holds it, so that
# members of anonymous classes are left out. Enum members sit one level
# deeper, in `enum_body_declarations`.
_OWNER = """
  inside:
    any:
      - kind: class_body
        inside:
          any:
            - kind: class_declaration
            - kind: interface_declaration
            - kind: record_declaration
          has: {field: name, pattern: $OWNER}
      - kind: interface_body
        inside:
          kind: interface_declaration
          has: {field: name, pattern: $OWNER}
      - kind: enum_body_declarations
        inside:
          kind: enum_body
          inside:
            kind: enum_declaration
            has: {field: name, pattern: $OWNER}
"""

# The declarations `JavaSource` reads, as ast-grep rules: types with their
# type parameters, methods and constructors, and field declarators.
_RULES = f"""
id: type
language: java
rule:
  any:
    - kind: class_declaration
    - kind: interface_declaration
    - kind: enum_declaration
    - kind: record_declaration
  has: {{field: name, pattern: $NAME}}
---
id: type_parameters
language: java
rule:
  kind: type_parameters
  pattern: $PARAMS
  inside:
    any:
      - kind: class_declaration
      - kind: interface_declaration
      - kind: record_declaration
    has: {{field: name, pattern: $OWNER}}
---
id: method
language: java
rule:
  kind: method_declaration
  all:
    - has: {{field: name, pattern: $NAME}}
    - has: {{field: type, pattern: $TYPE}}
    - has: {{field: parameters, pattern: $PARAMS}}
    - any:
        - has: {{field: type_parameters, pattern: $TYPE_PARAMS}}
        - kind: method_declaration
          not: {{has: {{field: type_parameters, kind: type_parameters}}}}
{_OWNER}
---
id: constructor
language: java
rule:
  kind: constructor_declaration
  all:
    - has: {{field: name, pattern: $NAME}}
    - has: {{field: parameters, pattern: $PARAMS}}
    - any:
        - has: {{field: type_parameters, pattern: $TYPE_PARAMS}}
        - kind: constructor_declaration
          not: {{has: {{field: type_parameters, kind: type_parameters}}}}
{_OWNER}
---
id: field
language: java
rule:
  kind: variable_declarator
  has: {{field: name, pattern: $NAME}}
  inside:
    kind: field_declaration
    pattern: $DECLARATION
    has: {{field: type, pattern: $TYPE}}
{_OWNER.replace(chr(10) + '  ', chr(10) + '    ')}
"""


@dataclass(frozen=True)
class Member:
    """A method, constructor or field a Java type declares."""

    kind: str  # 'method', 'constructor' or 'field'
    name: str
    # Simple type names; `?` for a type variable, which erases to its bound.
    params: tuple[str, ...] = ()
    returns: str | None = None
    static: bool = False


class JavaSource:
    """The declarations of the types in one Java file, read with ast-grep."""

    _TYPE_RULES = frozenset({'type', 'type_parameters'})

    def __init__(self, text: str) -> None:
        self._types: set[str] = set()
        self._type_vars: dict[str, frozenset[str]] = {}
        self._members: dict[str, list[Member]] = {}
        matches = self._scan(text)
        # Types and their parameters first: members need their type variables,
        # and ast-grep doesn't report matches in source order across rules.
        for match in sorted(
            matches, key=lambda m: m['ruleId'] not in self._TYPE_RULES
        ):
            self._add(match)

    def members(self, nested: list[str]) -> list[Member] | None:
        """The members of the type `nested[-1]`, nested in the ones before
        it. None if the file has no such type."""
        if not set(nested) <= self._types:
            return None
        return self._members.get(nested[-1], [])

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

    def _add(self, match: dict) -> None:
        captured = {
            name: variable['text']
            for name, variable in match['metaVariables']['single'].items()
        }
        rule = match['ruleId']
        if rule == 'type':
            self._types.add(captured['NAME'])
            return
        owner = captured['OWNER']
        if rule == 'type_parameters':
            self._type_vars[owner] = self._type_var_names(
                captured['PARAMS'][1:-1]
            )
            return
        type_vars = self._type_vars.get(owner, frozenset())
        name = captured['NAME']
        if rule == 'field':
            # A declarator alone has no modifiers: they are its declaration's.
            declaration = self._without_annotations(captured['DECLARATION'])
            static = 'static' in declaration.split('=', 1)[0].split()
            member = Member(
                'field',
                name,
                returns=self._simple(captured['TYPE'], type_vars),
                static=static,
            )
        else:
            text = self._without_annotations(match['text'])
            head = text[: text.find(name)].split()
            static = 'static' in head
            # `<T> T get(...)` declares its own type variables; a generic
            # return type, as in `List<String> get(...)`, declares none.
            if own := captured.get('TYPE_PARAMS'):
                type_vars |= self._type_var_names(own[1:-1])
            params = tuple(
                self._parameter(p, type_vars)
                for p in self._split_top(
                    self._without_annotations(captured['PARAMS'])[1:-1]
                )
                if p.strip()
            )
            returns = captured.get('TYPE')
            member = Member(
                rule,
                name,
                params,
                returns and self._simple(returns, type_vars),
                static,
            )
        self._members.setdefault(owner, []).append(member)

    @staticmethod
    def _without_annotations(text: str) -> str:
        out, i = [], 0
        for match in re.finditer(r'@(?!interface\b)[\w.]+', text):
            if match.start() < i:
                continue
            out.append(text[i : match.start()])
            i = match.end()
            rest = text[i:].lstrip()
            if rest.startswith('('):
                start = len(text) - len(rest)
                i = JavaSource._closing(text, start) + 1
        out.append(text[i:])
        return ''.join(out)

    @staticmethod
    def _closing(text: str, start: int) -> int:
        """The index of the `)` closing the `(` at `start`."""
        depth = 0
        for i in range(start, len(text)):
            if text[i] == '(':
                depth += 1
            elif text[i] == ')':
                depth -= 1
                if depth == 0:
                    return i
        return len(text) - 1

    @staticmethod
    def _split_top(text: str) -> list[str]:
        """`text` split at its commas outside brackets."""
        parts, depth, start = [], 0, 0
        for i, c in enumerate(text):
            if c in '(<[{':
                depth += 1
            elif c in ')>]}':
                depth -= 1
            elif c == ',' and depth == 0:
                parts.append(text[start:i])
                start = i + 1
        parts.append(text[start:])
        return parts

    @staticmethod
    def _type_var_names(declaration: str) -> frozenset[str]:
        return frozenset(
            part.split()[0]
            for part in JavaSource._split_top(declaration)
            if part.split()
        )

    @staticmethod
    def _without_generics(text: str) -> str:
        out, depth = [], 0
        for c in text:
            if c == '<':
                depth += 1
            elif c == '>':
                depth = max(depth - 1, 0)
            elif depth == 0:
                out.append(c)
        return ''.join(out)

    @staticmethod
    def _simple(type_name: str, type_vars: frozenset[str]) -> str:
        # Type-use annotations can sit inside a qualified name, as in
        # `MediaNotificationInfo.@Nullable Builder`.
        bare = JavaSource._without_generics(
            JavaSource._without_annotations(type_name)
        ).replace(' ', '')
        base = bare.rstrip('[]')
        simple = base.rsplit('.', 1)[-1]
        if simple in type_vars:
            return '?'
        return simple + bare[len(base) :]

    @staticmethod
    def _parameter(text: str, type_vars: frozenset[str]) -> str:
        tokens = [
            t
            for t in JavaSource._without_generics(text).split()
            if t != 'final'
        ]
        if len(tokens) < 2:
            return '?'
        type_name, name = ' '.join(tokens[:-1]), tokens[-1]
        type_name = type_name.replace('...', '[]')
        type_name += '[]' * name.count('[]')
        return JavaSource._simple(type_name, type_vars)


@dataclass(frozen=True)
class Literal:
    """A name Brave hardcodes as a string, and what it expects of it."""

    family: str  # 'reflection' or 'bytecode'
    kind: str  # 'method', 'constructor', 'field' or 'class'
    # The owning class, dotted, with `$` before nested types.
    owner: str
    where: str
    member: str = ''
    # Simple type names; `?` for one the call site computes.
    params: tuple[str, ...] = ()
    returns: str | None = None
    static: bool | None = None
    # From an `assertFalse` assertion: Brave's bytecode deletes the member from
    # the built class, so the source must still have it for that to apply.
    deleted_by_bytecode: bool = False

    @property
    def brave(self) -> bool:
        """Whether the owner is Brave's, resolved in the working tree."""
        return self.owner.rsplit('.', 1)[-1].startswith('Brave')

    def describe(self) -> str:
        """The literal as Java would spell it, e.g. `Foo.go(Tab, int)`."""
        simple = self.owner.rsplit('.', 1)[-1]
        if self.kind == 'class':
            return simple
        if self.kind == 'field':
            return f'{simple}.{self.member}'
        params = ', '.join(self.params)
        name = (
            simple if self.kind == 'constructor' else f'{simple}.{self.member}'
        )
        return f'{name}({params})'

    def check(self, members: list[Member]) -> str:
        """'ok', 'review', or what doesn't match, against `members`."""
        if self.kind == 'class':
            return 'ok'
        named = [
            m
            for m in members
            if m.kind == self.kind
            and (self.kind == 'constructor' or m.name == self.member)
        ]
        if self.kind == 'constructor' and not named and not self.params:
            # The implicit default constructor.
            return 'ok'
        if not named:
            return 'member missing'
        if self.kind == 'field':
            if self.returns is None or any(
                m.returns in (self.returns, '?') for m in named
            ):
                return 'ok'
            return 'type differs'
        same_arity = [m for m in named if len(m.params) == len(self.params)]
        if not same_arity:
            return 'arity differs'
        matches = [
            m
            for m in same_arity
            if all(
                '?' in (p, q) or p == q for p, q in zip(m.params, self.params)
            )
        ]
        if not matches:
            return 'parameters differ'
        fitting = [
            m
            for m in matches
            if (self.static is None or m.static == self.static)
            and (self.returns is None or m.returns in (self.returns, '?'))
        ]
        if not fitting:
            return 'modifier or return type differs'
        # A type variable erases to its bound, which the source doesn't say,
        # and a computed class isn't known until it runs.
        exact = [
            m
            for m in fitting
            if '?' not in (*m.params, *self.params)
            and (self.returns is None or m.returns != '?')
        ]
        return 'ok' if exact else 'review'


class LiteralCollector:
    """Collects the literals from Brave's working tree."""

    _REFLECTION = re.compile(
        r'BraveReflectionUtil\.(invokeMethod|getField|setField)\s*\('
    )
    _ASSERTION = re.compile(
        r'\b(methodExists|fieldExists|constructorsMatch|classExists)\s*\('
    )
    _MODIFIER = {'MethodModifier.STATIC': True, 'MethodModifier.REGULAR': False}

    def __init__(self, root: Path) -> None:
        self._root = root

    def collect(self) -> list[Literal]:
        """Every literal in Brave's Java sources."""
        literals: list[Literal] = []
        for path in self._java_files():
            text = (self._root / path).read_bytes().decode('utf-8')
            if path == _BYTECODE_TEST:
                literals += self._assertions(path, text)
            if 'BraveReflectionUtil.' in text:
                literals += self._reflections(path, text)
        return literals

    def _java_files(self) -> list[Path]:
        output = repository.brave.run_git('ls-files', '--', '*.java')
        return [Path(p) for p in output.splitlines()]

    @staticmethod
    def _code_only(text: str) -> str:
        """`text` without comments, keeping strings and offsets."""
        return _COMMENT_OR_STRING.sub(LiteralCollector._blank_comment, text)

    @staticmethod
    def _blank_comment(match: re.Match) -> str:
        """A comment as spaces, its newlines kept; a string as it is."""
        text = match.group()
        return re.sub(r'[^\n]', ' ', text) if text.startswith('/') else text

    @staticmethod
    def _arguments(text: str, open_paren: int) -> list[str]:
        close = JavaSource._closing(text, open_paren)
        return [
            ' '.join(a.split())
            for a in JavaSource._split_top(text[open_paren + 1 : close])
        ]

    @staticmethod
    def _class_token(argument: str) -> str | None:
        """`Foo` for `Foo.class`, `int[]` for `int[].class`."""
        match = re.fullmatch(r'([\w.]+(?:\[\])*)\.class', argument)
        return match and match.group(1).rsplit('.', 1)[-1]

    @classmethod
    def _param_token(cls, argument: str) -> str:
        """A parameter's simple type name, or `?` when the argument computes
        the class (e.g. `BraveFoo.getHiddenTypeClass()`)."""
        if token := cls._class_token(argument):
            return token
        named = re.fullmatch(r'getClassForPath\(\s*"([^"]+)"\s*\)', argument)
        if named:
            return re.split(r'[/$]', named.group(1))[-1]
        return '?'

    @staticmethod
    def _string(argument: str) -> str | None:
        match = re.fullmatch(r'"([^"]*)"', argument)
        return match and match.group(1)

    @staticmethod
    def _where(path: Path, code: str, offset: int) -> str:
        return f'{path}:{code.count(chr(10), 0, offset) + 1}'

    def _reflections(self, path: Path, text: str) -> list[Literal]:
        code = self._code_only(text)
        imports = {
            name.rsplit('.', 1)[-1]: name
            for name in re.findall(r'^import\s+([\w.]+);', code, re.M)
        }
        package = re.search(r'^package\s+([\w.]+);', code, re.M)
        literals: list[Literal] = []
        for match in self._REFLECTION.finditer(code):
            args = self._arguments(code, match.end() - 1)
            owner_token = re.fullmatch(r'([\w.]+)\.class', args[0])
            if owner_token is None:
                continue
            outer, *inner = owner_token.group(1).split('.')
            owner = imports.get(outer) or (
                f'{package.group(1)}.{outer}' if package else outer
            )
            owner = '$'.join([owner, *inner])
            where = self._where(path, code, match.start())
            if match.group(1) == 'invokeMethod':
                name = self._string(args[2]) if len(args) > 2 else None
                params = tuple(self._param_token(a) for a in args[3::2])
                if name is not None:
                    literals.append(
                        Literal(
                            'reflection', 'method', owner, where, name, params
                        )
                    )
            elif len(args) > 1 and (name := self._string(args[1])):
                literals.append(
                    Literal('reflection', 'field', owner, where, name)
                )
        return literals

    def _assertions(self, path: Path, text: str) -> list[Literal]:
        code = self._code_only(text)
        literals: list[Literal] = []
        for match in self._ASSERTION.finditer(code):
            before = code[max(0, match.start() - 60) : match.start()]
            if re.search(r'\b(?:boolean|void)\s*$', before):
                continue  # The helper's own definition.
            deleted = bool(re.search(r'assertFalse\s*\(\s*$', before))
            args = self._arguments(code, match.end() - 1)
            where = self._where(path, code, match.start())
            literals += self._assertion(match.group(1), args, where, deleted)
        return literals

    def _assertion(
        self, helper: str, args: list[str], where: str, deleted: bool
    ) -> list[Literal]:
        strings = [self._string(a) for a in args]
        if not strings or strings[0] is None:
            return []
        owner = strings[0].replace('/', '.')
        if helper == 'classExists':
            return [Literal('bytecode', 'class', owner, where)]
        if helper == 'fieldExists':
            return [
                Literal(
                    'bytecode',
                    'field',
                    owner,
                    where,
                    strings[1] or '',
                    returns=(
                        self._class_token(args[3]) if len(args) > 3 else None
                    ),
                    deleted_by_bytecode=deleted,
                )
            ]
        if helper == 'constructorsMatch':
            params = tuple(self._param_token(a) for a in args[2:])
            return [
                Literal(
                    'bytecode',
                    'constructor',
                    s.replace('/', '.'),
                    where,
                    params=params,
                )
                for s in strings[:2]
                if s
            ]
        return [
            Literal(
                'bytecode',
                'method',
                owner,
                where,
                strings[1] or '',
                tuple(self._param_token(a) for a in args[4:]),
                self._class_token(args[3]) if len(args) > 3 else None,
                self._MODIFIER.get(args[2] if len(args) > 2 else ''),
            )
        ]


class SourceTree:
    """The Java sources of a tree: Chromium at a tag, or Brave's checkout."""

    def __init__(self, repo: repository.Repository, tag: str | None) -> None:
        self._repo = repo
        self._tag = tag
        self._paths: dict[str, list[str]] | None = None
        self._texts: dict[str, str] = {}
        self._sources: dict[str, JavaSource] = {}

    def members(self, owner: str) -> list[Member] | None:
        """The members of `owner`, or None if its file or type isn't here."""
        path = self._path(owner)
        if path is None:
            return None
        if path not in self._sources:
            self._sources[path] = JavaSource(self._text(path))
        outer, *nested = owner.split('$')
        return self._sources[path].members([outer.rpartition('.')[2], *nested])

    def text(self, owner: str) -> str | None:
        """The text of the file declaring `owner`, or None."""
        path = self._path(owner)
        return None if path is None else self._text(path)

    def _index(self) -> dict[str, list[str]]:
        if self._paths is None:
            # `ls-tree` takes literal paths, not globs, so it lists everything.
            if self._tag:
                output = self._repo.run_git(
                    'ls-tree', '-r', '--name-only', self._tag
                )
            else:
                output = self._repo.run_git('ls-files', '--', '*.java')
            self._paths = {}
            for path in output.splitlines():
                if path.endswith('.java'):
                    name = path.rsplit('/', 1)[-1]
                    self._paths.setdefault(name, []).append(path)
        return self._paths

    def _path(self, owner: str) -> str | None:
        package, _, simple = owner.split('$')[0].rpartition('.')
        suffix = f'{package.replace(".", "/")}/{simple}.java'
        paths = [
            p
            for p in self._index().get(f'{simple}.java', [])
            if p == suffix or p.endswith(f'/{suffix}')
        ]
        return paths[0] if paths else None

    def _text(self, path: str) -> str:
        if path not in self._texts:
            if self._tag:
                self._texts[path] = self._repo.run_git(
                    'show', f'{self._tag}:{path}', no_trim=True
                )
            else:
                self._texts[path] = (
                    (self._repo.root / path).read_bytes().decode('utf-8')
                )
        return self._texts[path]


@dataclass(frozen=True)
class Result:
    """A literal's status at the old and the new tag."""

    literal: Literal
    old: str
    new: str
    # Whether the range changed the file declaring the literal's class.
    changed: bool = True

    @property
    def bucket(self) -> str:
        """One of `NameCheck.COUNTED`."""
        resolves = ('ok', 'review')
        if self.old == self.new == 'class missing':
            return 'unresolvable'
        if self.old == self.new == 'ok':
            return 'fine'
        if self.new in resolves and self.old not in resolves:
            return 'fixed'
        if self.old in resolves and self.new not in resolves:
            return 'broke'
        if self.new == 'review':
            # A class the range didn't change was read in an earlier range.
            return 'review' if self.changed else 'reviewed'
        return 'debt'


class NameCheck:
    """Resolves every literal at both ends of a range."""

    # The buckets to list, in order: those that need someone.
    LISTED = ('broke', 'debt', 'review')
    COUNTED = (*LISTED, 'reviewed', 'unresolvable', 'fixed', 'fine')

    def __init__(self, old: str, new: str) -> None:
        brave = SourceTree(repository.brave, None)
        self._trees = {
            False: (
                SourceTree(repository.chromium, old),
                SourceTree(repository.chromium, new),
            ),
            True: (brave, brave),
        }

    @staticmethod
    def default_range() -> str:
        """The branch's range: its upstream's Chromium tag to HEAD's."""
        return f'{Version.from_git("@{upstream}")}..{Version.from_git("HEAD")}'

    def run(self, literals: list[Literal]) -> list[Result]:
        """Each literal's result."""
        results = []
        for literal in literals:
            old, new = self._trees[literal.brave]
            results.append(
                Result(
                    literal,
                    self._status(literal, old),
                    self._status(literal, new),
                    old.text(literal.owner) != new.text(literal.owner),
                )
            )
        return results

    @staticmethod
    def _status(literal: Literal, tree: SourceTree) -> str:
        members = tree.members(literal.owner)
        return 'class missing' if members is None else literal.check(members)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        'range',
        nargs='?',
        help='OLD..NEW Chromium tags; the branch range by default.',
    )
    output = parser.add_mutually_exclusive_group()
    output.add_argument('--json', action='store_true', help='Print JSON.')
    output.add_argument(
        '--plain',
        action='store_true',
        help='Print the listed rows one per line, tab separated.',
    )
    args = parser.parse_args()
    old, _, new = (args.range or NameCheck.default_range()).partition('..')
    if not old or not new:
        parser.error('expected OLD..NEW')

    results = NameCheck(old, new).run(
        LiteralCollector(repository.brave.root).collect()
    )
    counts = Counter(r.bucket for r in results)
    if args.json:
        print(
            json.dumps(
                [
                    {
                        **asdict(r.literal),
                        'old': r.old,
                        'new': r.new,
                        'bucket': r.bucket,
                    }
                    for r in results
                ],
                indent=2,
            )
        )
    elif args.plain:
        for bucket in NameCheck.LISTED:
            for r in results:
                if r.bucket == bucket:
                    print(
                        '\t'.join(
                            (
                                bucket,
                                r.literal.describe(),
                                r.literal.where,
                                r.old,
                                r.new,
                            )
                        )
                    )
        print(', '.join(f'{counts[b]} {b}' for b in NameCheck.COUNTED))
    else:
        table = Table(title=f'Java names in strings, {old}..{new}')
        for column in ('bucket', 'name', 'where', old, new):
            table.add_column(column)
        for bucket in NameCheck.LISTED:
            for r in results:
                if r.bucket == bucket:
                    table.add_row(
                        bucket,
                        r.literal.describe(),
                        r.literal.where,
                        r.old,
                        r.new,
                    )
        console.print(table)
        console.print(', '.join(f'{counts[b]} {b}' for b in NameCheck.COUNTED))
    return 1 if counts['broke'] else 0


if __name__ == '__main__':
    sys.exit(main())
