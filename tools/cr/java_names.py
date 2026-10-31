#!/usr/bin/env vpython3
#
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

"""Resolves the upstream Java names Brave hardcodes as strings, at two tags.

Brave reaches into upstream Java by name in two places this checks:

- `BraveReflectionUtil.invokeMethod` / `getField`, where the member name is a
  string and the parameter types are an interleaved `type, arg` list;
- `BytecodeTest`'s `methodExists` / `fieldExists` / `constructorsMatch` /
  `classExists` assertions.

Neither is checked by a compiler. Reflection logs and returns null in release
builds, and `BytecodeTest` is an on-device instrumentation test, so when
upstream renames a method or changes a parameter list the lift builds green and
the Brave behaviour quietly disappears.

Two tags rather than one, because "this broke between the tags" is the
actionable bucket and "this has been dead for a while" is not. Both are read
from git with `git show`, so no checkout and no build is needed -- which is the
point, since bytecode ground truth would require an Android build at the *old*
tag.

Brave's own classes are read from the working tree in both passes, so they can
only ever land in the pre-existing bucket, never in the broke-between-tags one.

Every literal is checked as "upstream still declares this", including the few
`Assert.assertFalse(fieldExists(...))` ones. Those assert the state *after*
Brave's bytecode adapters ran, which source cannot see -- but a field Brave
deletes has to exist upstream first, so the same check still applies and
catches an adapter that has quietly become a no-op.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import argparse
import json
import logging
import re
import sys

from rich.table import Table

from java_source import (
    ClassIndex,
    JavaSource,
    blank_comments_and_strings,
)
from terminal import console, terminal, is_verbose
import repository
import versioning

# The test holding the bytecode assertions.
BYTECODE_TEST = Path(
    'android/javatests/org/chromium/chrome/browser/BytecodeTest.java'
)

# Helpers in `BytecodeTest` that take a class path as their first argument,
# mapped to the kind of literal they assert.
_ASSERTIONS = {
    'classExists': 'class',
    'fieldExists': 'field',
    'methodExists': 'method',
    'constructorsMatch': 'constructor',
}

_ASSERTION_CALL_RE = re.compile(r'\b(' + '|'.join(_ASSERTIONS) + r')\s*\(')

_REFLECTION_CALL_RE = re.compile(
    r'\bBraveReflectionUtil\.(invokeMethod|getField|setField)\s*\('
)

# `Foo.class`, `int.class`, `Foo.Bar.class`.
_CLASS_TOKEN_RE = re.compile(r'^([\w.]+(?:\[\])*)\.class$')

# `MethodModifier.STATIC`
_MODIFIER_TOKEN_RE = re.compile(r'^MethodModifier\.(STATIC|REGULAR)$')

# Stands in for a parameter type the call site does not spell out. Some
# `BytecodeTest` assertions pass a `Class<?>` returned by a helper rather than
# a `Foo.class` literal, which leaves the position but not the type to compare.
UNKNOWN_TYPE = '?'

# Buckets a literal can end up in, in reporting order.
BROKEN = 'broke-between-tags'
PRE_EXISTING = 'pre-existing'
UNRESOLVABLE = 'unresolvable'
FIXED = 'fixed-between-tags'
OK = 'ok'

_BUCKET_ORDER = (BROKEN, PRE_EXISTING, UNRESOLVABLE, FIXED, OK)


@dataclass(frozen=True)
class Literal:
    """One hardcoded reference to a Java member, as written in Brave's source.

    `parameters` is None when the call site says nothing about them, which
    means arity is not part of the comparison. The same goes for an empty
    `return_type` and a None `is_static`.
    """

    # 'bytecode-test' or 'reflection'.
    collector: str
    # 'class', 'field', 'method' or 'constructor'.
    kind: str
    # Slash-separated package path plus class name, `$` for a nested class.
    class_key: str
    # The Brave file the literal is written in, and the line it starts on.
    file: str
    line: int
    member: str = ''
    parameters: tuple[str, ...] | None = None
    return_type: str = ''
    is_static: bool | None = None

    @property
    def description(self) -> str:
        """A compact form of what this literal claims, for the tables.

        Deliberately short: upstream constructors run to forty parameters, so
        the arity stands in for the list and `--json` carries the whole thing.
        """
        target = self.class_key.rsplit('/', 1)[-1]
        if self.kind == 'constructor':
            target += '.<init>'
        elif self.member:
            target += f'.{self.member}'
        if self.parameters is not None:
            target += f'/{len(self.parameters)}'
        return target


@dataclass(frozen=True)
class Verdict:
    """What one revision has to say about one literal."""

    # None when the class could not be found at all, so the declaration is
    # unknown rather than absent.
    found: bool | None
    detail: str


@dataclass(frozen=True)
class Finding:
    """A literal, both verdicts on it, and the bucket they put it in."""

    literal: Literal
    old: Verdict
    new: Verdict
    bucket: str

    def as_dict(self) -> dict:
        return {
            'bucket': self.bucket,
            'collector': self.literal.collector,
            'kind': self.literal.kind,
            'class': self.literal.class_key,
            'member': self.literal.member,
            'parameters': self.literal.parameters,
            'file': self.literal.file,
            'line': self.literal.line,
            'old': {'found': self.old.found, 'detail': self.old.detail},
            'new': {'found': self.new.found, 'detail': self.new.detail},
        }


class Revision:
    """One side of the comparison: upstream at a tag, plus Brave as it is.

    Brave's working tree stands in for Brave at both revisions. A literal
    naming a Brave class therefore gets the same verdict on both sides, which
    is what keeps Brave's own churn out of the broke-between-tags bucket.
    """

    def __init__(self, name: str, chromium_revision: str) -> None:
        self._name = name
        self._indexes = (
            ClassIndex(repository.chromium, chromium_revision),
            ClassIndex(repository.brave, None),
        )

    @property
    def name(self) -> str:
        return self._name

    def lookup(self, class_key: str) -> tuple[JavaSource | None, str]:
        """Finds the source declaring `class_key`, and why if it did not."""
        for index in self._indexes:
            resolution = index.resolve(class_key)
            if resolution.ambiguous:
                return None, (
                    f'ambiguous in {index.revision}: '
                    f'{", ".join(resolution.ambiguous)}'
                )
            if not resolution.found:
                continue
            source = index.source(resolution.path)
            if source is None or not source.type_names:
                return None, f'could not read {resolution.path}'
            return source, resolution.path
        return None, 'class not found'


def line_of(text: str, offset: int) -> int:
    """The 1-based line number `offset` falls on."""
    return text.count('\n', 0, offset) + 1


def _call_arguments(
    structure: str, values: str, open_paren: int
) -> list[str] | None:
    """Splits one call's arguments, or None if the parens do not close.

    `structure` has string contents blanked so brackets inside a literal
    cannot throw off the nesting; `values` keeps them, and is what the
    arguments are sliced from.
    """
    depth = 0
    start = open_paren + 1
    arguments: list[str] = []
    for index in range(open_paren, len(structure)):
        character = structure[index]
        if character in '([{':
            depth += 1
        elif character in ')]}':
            depth -= 1
            if depth == 0:
                arguments.append(values[start:index])
                return [argument.strip() for argument in arguments]
        elif character == ',' and depth == 1:
            arguments.append(values[start:index])
            start = index + 1
    return None


def _string_argument(argument: str) -> str | None:
    """The value of a string-literal argument, joining `+` concatenation."""
    chunks = re.findall(r'"([^"]*)"', argument)
    return ''.join(chunks) if chunks else None


def _class_argument(argument: str) -> str | None:
    """The simple type name of a `Foo.class` argument."""
    match = _CLASS_TOKEN_RE.match(argument.strip())
    if match is None:
        return None
    name = match.group(1)
    suffix = ''
    while name.endswith('[]'):
        suffix += '[]'
        name = name[:-2]
    return name.rsplit('.', 1)[-1] + suffix


def class_key_of(path_literal: str) -> str:
    """Normalises a class path as `BytecodeTest` writes it.

    `getClassForPath` turns every `/` into a `.` before calling
    `Class.forName`, so a literal may use either separator -- and a few mix
    them. Slashes are what `ClassIndex` matches on.
    """
    outer, separator, nested = path_literal.partition('$')
    return outer.replace('.', '/') + separator + nested


def _type_arguments(arguments: list[str]) -> tuple[str, ...]:
    """Reads a `Foo.class, Bar.class` argument run as simple type names."""
    return tuple(
        _class_argument(argument) or UNKNOWN_TYPE
        for argument in arguments
        if argument
    )


def collect_bytecode_test(path: str, text: str) -> list[Literal]:
    """Collects the assertions in `BytecodeTest`."""
    structure = blank_comments_and_strings(text)
    values = blank_comments_and_strings(text, keep_strings=True)

    literals: list[Literal] = []
    for match in _ASSERTION_CALL_RE.finditer(structure):
        arguments = _call_arguments(structure, values, match.end() - 1)
        if not arguments:
            continue
        class_key = _string_argument(arguments[0])
        if class_key is None:
            # The helper's own declaration, or a call passing a variable.
            continue
        literals.extend(
            _bytecode_literals(
                kind=_ASSERTIONS[match.group(1)],
                class_key=class_key_of(class_key),
                arguments=arguments,
                file=path,
                line=line_of(text, match.start()),
            )
        )
    return literals


def _bytecode_literals(
    *,
    kind: str,
    class_key: str,
    arguments: list[str],
    file: str,
    line: int,
) -> list[Literal]:
    """Builds the literals for one assertion call."""
    common = {
        'collector': 'bytecode-test',
        'file': file,
        'line': line,
    }

    if kind == 'class':
        return [Literal(kind='class', class_key=class_key, **common)]

    if kind == 'field':
        member = _string_argument(arguments[1]) if len(arguments) > 1 else None
        if member is None:
            return []
        return [
            Literal(kind='field', class_key=class_key, member=member, **common)
        ]

    if kind == 'constructor':
        # `constructorsMatch` asserts both classes declare the constructor, so
        # both ends are worth resolving.
        other = _string_argument(arguments[1]) if len(arguments) > 1 else None
        parameters = _type_arguments(arguments[2:])
        return [
            Literal(
                kind='constructor',
                class_key=class_key_of(key),
                parameters=parameters,
                **common,
            )
            for key in (class_key, other)
            if key
        ]

    member = _string_argument(arguments[1]) if len(arguments) > 1 else None
    if member is None:
        return []
    modifier = (
        _MODIFIER_TOKEN_RE.match(arguments[2]) if len(arguments) > 2 else None
    )
    return_type = _class_argument(arguments[3]) if len(arguments) > 3 else None
    return [
        Literal(
            kind='method',
            class_key=class_key,
            member=member,
            parameters=_type_arguments(arguments[4:]),
            return_type=return_type or '',
            is_static=(modifier.group(1) == 'STATIC' if modifier else None),
            **common,
        )
    ]


def collect_reflection(path: str, text: str) -> list[Literal]:
    """Collects the `BraveReflectionUtil` call sites in one Brave file."""
    structure = blank_comments_and_strings(text)
    values = blank_comments_and_strings(text, keep_strings=True)
    source = JavaSource.parse(text)

    literals: list[Literal] = []
    for match in _REFLECTION_CALL_RE.finditer(structure):
        arguments = _call_arguments(structure, values, match.end() - 1)
        if not arguments:
            continue
        owner = _class_argument(arguments[0])
        class_key = _qualify(owner, source) if owner else None
        if class_key is None:
            logging.debug(
                '%s:%d: owner class is not a literal, skipping',
                path,
                line_of(text, match.start()),
            )
            continue

        common = {
            'collector': 'reflection',
            'class_key': class_key,
            'file': path,
            'line': line_of(text, match.start()),
        }

        if match.group(1) == 'invokeMethod':
            member = (
                _string_argument(arguments[2]) if len(arguments) > 2 else None
            )
            if member is None:
                continue
            # Arguments past the name interleave `type, value`, so the types
            # are the ones at even distance from the first.
            literals.append(
                Literal(
                    kind='method',
                    member=member,
                    parameters=_type_arguments(arguments[3::2]),
                    **common,
                )
            )
            continue

        member = _string_argument(arguments[1]) if len(arguments) > 1 else None
        if member is not None:
            literals.append(Literal(kind='field', member=member, **common))
    return literals


def _qualify(owner: str, source: JavaSource) -> str | None:
    """Turns a `Foo.class` simple name into a slash-separated class key.

    Resolved through the file's own imports, falling back to its package, the
    way javac would.
    """
    head, _, nested = owner.partition('.')
    imported = source.imports.get(head)
    if imported is None:
        if not source.package:
            return None
        imported = f'{source.package}.{head}'
    key = imported.replace('.', '/')
    return f'{key}${nested.replace(".", "$")}' if nested else key


def collect(only: str | None = None) -> list[Literal]:
    """Collects every literal in brave-core, from both families."""
    literals: list[Literal] = []

    if only in (None, 'bytecode-test'):
        path = repository.brave.root / BYTECODE_TEST
        literals += collect_bytecode_test(
            BYTECODE_TEST.as_posix(), path.read_bytes().decode('utf-8')
        )

    if only in (None, 'reflection'):
        listing = repository.brave.run_git(
            'grep', '-l', 'BraveReflectionUtil', '--', '*.java', no_trim=True
        )
        for relative in sorted(listing.splitlines()):
            if not relative:
                continue
            text = (
                (repository.brave.root / relative).read_bytes().decode('utf-8')
            )
            literals += collect_reflection(relative, text)
    return literals


def _describe(declared: tuple[str, ...], expected: tuple[str, ...]) -> str:
    """Names the first difference between a declaration and a literal."""
    if len(declared) != len(expected):
        return f'takes {len(declared)} parameters, not {len(expected)}'
    for position, (have, want) in enumerate(zip(declared, expected)):
        if want not in (UNKNOWN_TYPE, have):
            return f'parameter {position + 1} is {have}, not {want}'
    return 'matches'


def parameters_match(
    declared: tuple[str, ...], expected: tuple[str, ...]
) -> bool:
    """Compares a declared parameter list against what a literal asked for.

    `UNKNOWN_TYPE` matches anything, so a position the call site did not spell
    out still counts towards arity.
    """
    if len(declared) != len(expected):
        return False
    return all(
        want in (UNKNOWN_TYPE, have) for have, want in zip(declared, expected)
    )


PRESENT = Verdict(found=True, detail='present')


def check(literal: Literal, revision: Revision) -> Verdict:
    """Resolves one literal against one revision."""
    source, where = revision.lookup(literal.class_key)
    if source is None:
        return Verdict(found=None, detail=where)

    owner = literal.class_key.rsplit('/', 1)[-1]
    if owner not in source.type_names:
        return Verdict(found=False, detail=f'{owner} not declared in {where}')

    if literal.kind == 'class':
        return PRESENT

    if literal.kind == 'field':
        for declaration in source.fields:
            if (
                declaration.owner == owner
                and declaration.name == literal.member
            ):
                return PRESENT
        return Verdict(found=False, detail=f'no field {literal.member}')

    if literal.kind == 'constructor':
        declared = [
            constructor
            for constructor in source.constructors
            if constructor.owner == owner
        ]
        if not declared and not literal.parameters:
            # A class with no constructor still has the implicit no-arg one,
            # which is what `getDeclaredConstructor()` finds.
            return Verdict(found=True, detail='present (implicit)')
        for constructor in declared:
            if parameters_match(
                constructor.parameters, literal.parameters or ()
            ):
                return PRESENT
        if not declared:
            return Verdict(found=False, detail='declares no constructor')
        return Verdict(
            found=False,
            detail='; '.join(
                _describe(constructor.parameters, literal.parameters or ())
                for constructor in declared
            ),
        )

    return _check_method(literal, source, owner)


def _check_method(literal: Literal, source: JavaSource, owner: str) -> Verdict:
    """Resolves a method literal, reporting the closest declaration."""
    candidates = [
        method
        for method in source.methods
        if method.owner == owner and method.name == literal.member
    ]
    if not candidates:
        return Verdict(found=False, detail=f'no method {literal.member}')

    mismatches: list[str] = []
    for method in candidates:
        if literal.parameters is not None and not parameters_match(
            method.parameters, literal.parameters
        ):
            mismatches.append(_describe(method.parameters, literal.parameters))
            continue
        if literal.return_type and method.return_type != literal.return_type:
            mismatches.append(
                f'returns {method.return_type}, not {literal.return_type}'
            )
            continue
        if (
            literal.is_static is not None
            and method.is_static != literal.is_static
        ):
            mismatches.append(
                'is static' if method.is_static else 'is not static'
            )
            continue
        return PRESENT

    return Verdict(found=False, detail='; '.join(mismatches))


def bucket_of(old: Verdict, new: Verdict) -> str:
    """Puts one literal in a bucket from its two verdicts."""
    if new.found:
        return FIXED if old.found is False else OK
    # Unresolvable at *both* revisions means there was never any source to
    # read, which is the prebuilt-AAR case and not a finding. Unresolvable at
    # only the new one means the class itself went away, which is as broken as
    # a renamed member and must not be filed with the quiet ones.
    if new.found is None and old.found is None:
        return UNRESOLVABLE
    return BROKEN if old.found else PRE_EXISTING


def failing_buckets(old_revision: str, new_revision: str) -> tuple[str, ...]:
    """The buckets whose findings make a run exit non-zero.

    Breakage that predates the lift is not the lift's problem, so comparing two
    revisions only fails on a fresh break. With one revision nothing can have
    broken *between* anything, and pre-existing is the only bucket a finding can
    reach, so that is what fails instead -- otherwise auditing the tree as it
    stands could never fail at all.
    """
    if old_revision == new_revision:
        return (BROKEN, PRE_EXISTING)
    return (BROKEN,)


def classify(
    literals: list[Literal], old: Revision, new: Revision
) -> list[Finding]:
    """Resolves every literal against both revisions."""
    findings: list[Finding] = []
    for literal in literals:
        old_verdict = check(literal, old)
        new_verdict = check(literal, new)
        findings.append(
            Finding(
                literal=literal,
                old=old_verdict,
                new=new_verdict,
                bucket=bucket_of(old_verdict, new_verdict),
            )
        )
    return findings


def report(findings: list[Finding], old: Revision, new: Revision) -> None:
    """Prints the summary, then a table per bucket worth reading."""
    counts = {bucket: 0 for bucket in _BUCKET_ORDER}
    for finding in findings:
        counts[finding.bucket] += 1

    summary = Table(title=f'Java name literals: {old.name} -> {new.name}')
    summary.add_column('bucket')
    summary.add_column('count', justify='right')
    for bucket in _BUCKET_ORDER:
        summary.add_row(bucket, str(counts[bucket]))
    console.print(summary)

    for bucket, style in (
        (BROKEN, 'red'),
        (PRE_EXISTING, 'yellow'),
        (FIXED, 'green'),
        (UNRESOLVABLE, 'dim'),
    ):
        rows = [finding for finding in findings if finding.bucket == bucket]
        if not rows:
            continue
        if bucket == UNRESOLVABLE and not is_verbose():
            console.print(
                f'[dim]{len(rows)} unresolvable (not in either tree: '
                'prebuilt or generated Java). Pass --verbose to list '
                'them.[/]'
            )
            continue

        table = Table(
            title=bucket, title_style=style, show_lines=True, expand=True
        )
        table.add_column('literal', overflow='fold', ratio=2)
        table.add_column(f'at {old.name}', overflow='fold', ratio=1)
        table.add_column(f'at {new.name}', overflow='fold', ratio=1)
        for finding in sorted(
            rows, key=lambda row: (row.literal.file, row.literal.line)
        ):
            where = f'{Path(finding.literal.file).name}:{finding.literal.line}'
            table.add_row(
                f'{finding.literal.description}\n[dim]{where}[/]',
                finding.old.detail,
                finding.new.detail,
            )
        console.print(table)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        '--old',
        help='The Chromium tag or commit to resolve against first, e.g. '
        'the pin the lift started from. Defaults to --new, which audits the '
        'tree as it stands: nothing can have broken between one revision and '
        'itself, so whatever is broken reports as pre-existing, and that is '
        'what fails the run.',
    )
    parser.add_argument(
        '--new',
        help='The Chromium tag or commit to resolve against second. '
        'Defaults to the pin in package.json.',
    )
    parser.add_argument(
        '--only',
        choices=('bytecode-test', 'reflection'),
        help='Check only one family of literals.',
    )
    parser.add_argument(
        '--json',
        action='store_true',
        help='Print the findings as JSON instead of tables.',
    )
    parser.add_argument(
        '--exit-zero',
        action='store_true',
        help='Always exit 0, even when something is broken.',
    )
    parser.add_argument(
        '--verbose', action='store_true', help='Enable verbose logging'
    )
    args = parser.parse_args()

    logging.basicConfig(
        level=logging.DEBUG if is_verbose() else logging.INFO,
        format='%(message)s',
        force=True,
    )

    new_revision = args.new or str(versioning.Version.from_git('HEAD'))
    old_revision = args.old or new_revision
    for revision in (old_revision, new_revision):
        if not repository.chromium.is_valid_git_reference(revision):
            print(
                f'error: {revision} is not a revision in the Chromium repo',
                file=sys.stderr,
            )
            return 2

    literals = collect(args.only)
    with terminal.with_status(
        f'Resolving {len(literals)} literals at '
        f'{old_revision} and {new_revision}'
    ):
        old = Revision(old_revision, old_revision)
        new = Revision(new_revision, new_revision)
        findings = classify(literals, old, new)

    if args.json:
        print(
            json.dumps(
                {
                    'old': old_revision,
                    'new': new_revision,
                    'findings': [finding.as_dict() for finding in findings],
                },
                indent=2,
            )
        )
    else:
        report(findings, old, new)

    failing = failing_buckets(old_revision, new_revision)
    hits = [finding for finding in findings if finding.bucket in failing]
    return 1 if hits and not args.exit_zero else 0


if __name__ == '__main__':
    sys.exit(main())
