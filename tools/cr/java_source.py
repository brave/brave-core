# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""Reads Java declarations out of source text, with no build and no checkout.

Only what `java_names.py` needs to answer "does this class still declare this
member": the declarations a class makes *itself*. That is the same rule the
reflection and bytecode-test call sites live by, since both go through
`getDeclaredMethods` / `getDeclaredFields`, which do not look at superclasses.
So resolving a literal never has to follow an `extends` chain, and parsing one
file per literal is enough.

The parser is deliberately not a Java front end. It blanks comments and string
literals, then walks `{`, `}` and `;` while tracking brace depth, recording the
declarations that sit directly in a type body. Anything it cannot read
confidently is left unrecorded rather than guessed at, and callers treat "no
declarations at all" as unresolvable instead of as a missing member.
"""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
import re
import subprocess

from repository import Repository

# Modifiers that may precede a return type, and so are never the return type
# themselves.
_MODIFIERS = frozenset(
    (
        'public',
        'protected',
        'private',
        'static',
        'final',
        'abstract',
        'synchronized',
        'native',
        'transient',
        'volatile',
        'default',
        'strictfp',
        'sealed',
        'non-sealed',
    )
)

# A type declaration inside a clause, e.g. `public static final class Foo`.
# `@interface` is matched by the `interface` alternative.
_TYPE_DECLARATION_RE = re.compile(r'\b(?:class|interface|enum|record)\s+(\w+)')

# `package a.b.c;`
_PACKAGE_RE = re.compile(r'^\s*package\s+([\w.]+)\s*;', re.MULTILINE)

# `import a.b.C;` / `import static a.b.C.D;`
_IMPORT_RE = re.compile(
    r'^\s*import\s+(?:static\s+)?([\w.]+)\s*;', re.MULTILINE
)

# An annotation, with or without arguments. Arguments are matched loosely
# because by the time this runs any string literal inside them is blank. The
# lookahead is what keeps `public @interface Foo` from being stripped whole,
# leaving nothing for `_TYPE_DECLARATION_RE` to find.
_ANNOTATION_RE = re.compile(r'@\s*(?!interface\b)[\w.]+\s*(?:\([^()]*\))?')

_IDENTIFIER_RE = re.compile(r'^\w+$')

# What may legally follow a declaration's parameter list.
_METHOD_TAIL_RE = re.compile(r'^(?:throws\s+[\w.,\s]+|default\s+.*)?$')


@dataclass(frozen=True)
class MethodDeclaration:
    """A method declared directly by `owner`."""

    # The declaring type, as `Outer` or `Outer$Inner`.
    owner: str
    name: str
    # Erased parameter type simple names, e.g. `('Activity', 'int[]')`.
    parameters: tuple[str, ...]
    # The return type simple name, erased. `''` for a constructor.
    return_type: str
    is_static: bool


@dataclass(frozen=True)
class FieldDeclaration:
    """A field declared directly by `owner`."""

    owner: str
    name: str


@dataclass(frozen=True)
class ConstructorDeclaration:
    """A constructor declared directly by `owner`."""

    owner: str
    parameters: tuple[str, ...]


def blank_comments_and_strings(text: str, *, keep_strings: bool = False) -> str:
    """Replaces comment and literal *contents* with spaces.

    Offsets and line numbers are preserved, so the result can be scanned for
    braces and identifiers without tripping over a `{` inside a string or a
    `//` comment. Quotes themselves are kept.

    With `keep_strings`, only comments are blanked. Callers that need both
    (structure from one, literal values from the other) can index into the two
    results interchangeably, since offsets line up with the original.
    """
    out = list(text)
    index = 0
    end = len(text)

    def blank(position: int) -> None:
        if text[position] != '\n':
            out[position] = ' '

    while index < end:
        if text.startswith('//', index):
            while index < end and text[index] != '\n':
                blank(index)
                index += 1
        elif text.startswith('/*', index):
            blank(index)
            blank(index + 1)
            index += 2
            while index < end and not text.startswith('*/', index):
                blank(index)
                index += 1
            for offset in range(index, min(index + 2, end)):
                blank(offset)
            index += 2
        elif text.startswith('"""', index):
            # A text block. Its content is blanked even with `keep_strings`,
            # since no literal this module's callers want is ever written in
            # one, and a `"` or a `{` inside it would otherwise desynchronise
            # the scan for the rest of the file.
            index += 3
            while index < end and not text.startswith('"""', index):
                if text[index] == '\\':
                    blank(index)
                    index += 1
                if index < end:
                    blank(index)
                    index += 1
            index += 3
        elif text[index] in '"\'':
            quote = text[index]
            index += 1
            while index < end and text[index] != quote:
                if text[index] == '\\':
                    if not keep_strings:
                        blank(index)
                    index += 1
                if index < end and not keep_strings:
                    blank(index)
                index += 1
            index += 1
        else:
            index += 1
    return ''.join(out)


def split_top_level(text: str, separator: str = ',') -> list[str]:
    """Splits on `separator`, ignoring anything nested in brackets.

    Generic argument lists are the reason this is needed: `Map<String, Integer>`
    is one parameter, not two.
    """
    parts: list[str] = []
    depth = 0
    current: list[str] = []
    for char in text:
        if char in '([<{':
            depth += 1
        elif char in ')]>}':
            depth -= 1
        if char == separator and depth == 0:
            parts.append(''.join(current))
            current = []
        else:
            current.append(char)
    parts.append(''.join(current))
    return parts


def _erase_generics(text: str) -> str:
    """Drops `<...>` argument lists, leaving the erased type."""
    out: list[str] = []
    depth = 0
    for char in text:
        if char == '<':
            depth += 1
        elif char == '>':
            depth = max(0, depth - 1)
        elif depth == 0:
            out.append(char)
    return ''.join(out)


def simple_type_name(text: str) -> str:
    """Normalises a type as written in source to a comparable simple name.

    Generics are erased, a package qualifier is dropped, and varargs become an
    array, so `java.util.List<Foo>` is `List` and `String...` is `String[]`.
    Simple names are the only form available on the other side of the
    comparison, where a literal reads `Foo.class`.
    """
    text = _erase_generics(text).strip()
    text = text.replace('...', '[]')
    # A type-use annotation may sit inside a qualified name, as in
    # `ActionMode.@Nullable Callback`, and stripping it leaves a gap around
    # the dot that would otherwise survive into the simple name.
    text = re.sub(r'\s*\.\s*', '.', text)
    suffix = ''
    while text.endswith('[]'):
        suffix += '[]'
        text = text[:-2].strip()
    return text.rsplit('.', 1)[-1] + suffix


def _strip_annotations(clause: str) -> str:
    previous = None
    while previous != clause:
        previous = clause
        clause = _ANNOTATION_RE.sub(' ', clause)
    return clause


def _normalise_clause(clause: str) -> str:
    """Flattens a clause to one line and closes up spaces inside generics."""
    clause = _strip_annotations(clause)
    clause = ' '.join(clause.split())
    out: list[str] = []
    depth = 0
    for char in clause:
        if char == '<':
            depth += 1
        elif char == '>':
            depth = max(0, depth - 1)
        if char == ' ' and depth > 0:
            continue
        out.append(char)
    return ''.join(out)


def _matching_paren(text: str, start: int) -> int:
    """Index of the `)` closing the `(` at `start`, or -1."""
    depth = 0
    for index in range(start, len(text)):
        if text[index] == '(':
            depth += 1
        elif text[index] == ')':
            depth -= 1
            if depth == 0:
                return index
    return -1


def _parse_parameters(text: str) -> tuple[str, ...]:
    """Parses a declaration's parameter list into erased simple names."""
    if not text.strip():
        return ()
    parameters: list[str] = []
    for part in split_top_level(text):
        part = _strip_annotations(part).strip()
        tokens = part.split()
        tokens = [token for token in tokens if token != 'final']
        if len(tokens) < 2:
            # A parameter with no name, which a declaration never has. Keep
            # whatever is there so arity stays right.
            parameters.append(simple_type_name(part))
            continue
        parameters.append(simple_type_name(' '.join(tokens[:-1])))
    return tuple(parameters)


def _parse_field(head: str, owner: str) -> list[FieldDeclaration]:
    """Parses `private final Foo bar` (and `Foo a, b`) into declarations."""
    declarators = split_top_level(head)
    first = declarators[0].split()
    if len(first) < 2:
        return []
    names = [first[-1]] + [
        part.strip() for part in declarators[1:] if part.strip()
    ]
    fields: list[FieldDeclaration] = []
    for name in names:
        # Array brackets may trail the name rather than the type.
        name = name.replace('[]', '').strip()
        if _IDENTIFIER_RE.match(name):
            fields.append(FieldDeclaration(owner=owner, name=name))
    return fields


class _TypeStack:
    """The chain of type declarations currently open, innermost last."""

    def __init__(self) -> None:
        self._names: list[str] = []
        # For each open type, the brace depth its members sit at.
        self._member_depths: list[int] = []

    def __bool__(self) -> bool:
        return bool(self._names)

    @property
    def owner(self) -> str:
        return '$'.join(self._names)

    @property
    def member_depth(self) -> int:
        return self._member_depths[-1]

    def push(self, name: str, member_depth: int) -> None:
        self._names.append(name)
        self._member_depths.append(member_depth)

    def pop_closed(self, depth: int) -> None:
        while self._member_depths and depth < self._member_depths[-1]:
            self._names.pop()
            self._member_depths.pop()


@dataclass(frozen=True)
class JavaSource:
    """The declarations one Java file makes."""

    package: str
    # Simple name -> fully-qualified name, from the file's imports.
    imports: dict[str, str]
    # Declared types, as `Outer` and `Outer$Inner`.
    type_names: frozenset[str]
    methods: tuple[MethodDeclaration, ...]
    fields: tuple[FieldDeclaration, ...]
    constructors: tuple[ConstructorDeclaration, ...]

    @staticmethod
    def parse(text: str) -> JavaSource:
        """Reads declarations out of Java source text."""
        blanked = blank_comments_and_strings(text)

        package_match = _PACKAGE_RE.search(blanked)
        package = package_match.group(1) if package_match else ''
        imports = {
            fqn.rsplit('.', 1)[-1]: fqn for fqn in _IMPORT_RE.findall(blanked)
        }

        type_names: list[str] = []
        methods: list[MethodDeclaration] = []
        fields: list[FieldDeclaration] = []
        constructors: list[ConstructorDeclaration] = []

        stack = _TypeStack()
        depth = 0
        clause_start = 0

        for token in re.finditer(r'[{};]', blanked):
            clause = blanked[clause_start : token.start()]
            clause_start = token.end()
            character = token.group()

            if character == '}':
                depth -= 1
                stack.pop_closed(depth)
                continue

            at_member_level = bool(stack) and depth == stack.member_depth

            if character == '{':
                declared_type = _TYPE_DECLARATION_RE.search(
                    _strip_annotations(clause)
                )
                if declared_type is not None:
                    stack.push(declared_type.group(1), depth + 1)
                    type_names.append(stack.owner)
                    depth += 1
                    continue

            if at_member_level:
                JavaSource._record_member(
                    clause, stack.owner, methods, fields, constructors
                )

            if character == '{':
                depth += 1

        return JavaSource(
            package=package,
            imports=imports,
            type_names=frozenset(type_names),
            methods=tuple(methods),
            fields=tuple(fields),
            constructors=tuple(constructors),
        )

    @staticmethod
    def _record_member(
        clause: str,
        owner: str,
        methods: list[MethodDeclaration],
        fields: list[FieldDeclaration],
        constructors: list[ConstructorDeclaration],
    ) -> None:
        """Records whatever declaration `clause` holds, if any."""
        clause = _normalise_clause(clause)
        if not clause:
            return

        # An initialiser means a field, even when its value is a `new Foo() {`
        # expression that looks like a declaration.
        assignment = split_top_level(clause, '=')
        if len(assignment) > 1:
            fields.extend(_parse_field(assignment[0], owner))
            return

        paren = clause.find('(')
        if paren == -1:
            fields.extend(_parse_field(clause, owner))
            return

        closing = _matching_paren(clause, paren)
        if closing == -1:
            return
        if not _METHOD_TAIL_RE.match(clause[closing + 1 :].strip()):
            return

        head = clause[:paren].split()
        if not head or not _IDENTIFIER_RE.match(head[-1]):
            return
        name = head[-1]
        preceding = head[:-1]
        parameters = _parse_parameters(clause[paren + 1 : closing])

        return_types = [token for token in preceding if token not in _MODIFIERS]
        # A generic method's type parameters precede its return type.
        return_types = [
            token for token in return_types if not token.startswith('<')
        ]
        if not return_types:
            # No return type: a constructor, which must be named for its type.
            if name == owner.rsplit('$', 1)[-1]:
                constructors.append(
                    ConstructorDeclaration(owner=owner, parameters=parameters)
                )
            return

        methods.append(
            MethodDeclaration(
                owner=owner,
                name=name,
                parameters=parameters,
                return_type=simple_type_name(return_types[-1]),
                is_static='static' in preceding,
            )
        )


@dataclass(frozen=True)
class Resolution:
    """Where a class key resolved to, in one revision of one repository."""

    # The repo-relative path of the file declaring it, if found.
    path: str | None
    # Every candidate path, when the key matched more than one file.
    ambiguous: tuple[str, ...] = ()

    @property
    def found(self) -> bool:
        return self.path is not None


class ClassIndex:
    """Maps a slash-separated class key to a file, for one revision.

    A key is a package path plus the class name, as the literals write it:
    `org/chromium/chrome/browser/ntp/NewTabPage`, optionally with a `$Inner`
    suffix. Chromium's Java is not laid out with the package path at the root
    of the tree, so lookups match on the path's tail rather than computing a
    directory from the package.

    Pass `revision=None` to read the working tree instead of a commit.
    """

    def __init__(self, repo: Repository, revision: str | None) -> None:
        self._repo = repo
        self._revision = revision
        self._by_class_name: dict[str, list[str]] = defaultdict(list)
        self._sources: dict[str, JavaSource | None] = {}
        for path in self._list_java_files():
            self._by_class_name[Path(path).stem].append(path)

    @property
    def revision(self) -> str:
        return self._revision or 'working tree'

    def _list_java_files(self) -> list[str]:
        if self._revision is None:
            listing = self._repo.run_git(
                'ls-files', '--', '*.java', no_trim=True
            )
        else:
            # `ls-tree` rejects a wildcard pathspec, so the whole tree is
            # listed and filtered below.
            listing = self._repo.run_git(
                'ls-tree',
                '-r',
                '--name-only',
                self._revision,
                no_trim=True,
            )
        return [line for line in listing.splitlines() if line.endswith('.java')]

    def resolve(self, class_key: str) -> Resolution:
        """Finds the file declaring `class_key`."""
        outer = class_key.split('$', 1)[0]
        candidates = [
            path
            for path in self._by_class_name.get(outer.rsplit('/', 1)[-1], ())
            if path.endswith(f'/{outer}.java') or path == f'{outer}.java'
        ]
        if len(candidates) == 1:
            return Resolution(path=candidates[0])
        if not candidates:
            return Resolution(path=None)
        return Resolution(path=None, ambiguous=tuple(sorted(candidates)))

    def source(self, path: str) -> JavaSource | None:
        """Parses a file at this revision, or returns None if unreadable."""
        if path not in self._sources:
            self._sources[path] = self._read_and_parse(path)
        return self._sources[path]

    def _read_and_parse(self, path: str) -> JavaSource | None:
        try:
            if self._revision is None:
                text = (self._repo.root / path).read_bytes().decode('utf-8')
            else:
                text = self._repo.read_file(path, commit=self._revision)
        except (subprocess.CalledProcessError, OSError, UnicodeDecodeError):
            return None
        return JavaSource.parse(text)
