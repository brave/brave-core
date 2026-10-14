#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Lists the commits of a range that change the upstream originals of Brave's
forks, so that each Brave copy can be checked for the same fix.

A fork is Brave code that copies upstream code rather than wrapping it:

    comment      a "copied from", "taken from", "adapted from" or "kept in
                 sync with" comment naming an upstream file, or a class whose
                 snake_case name is a single upstream file
    replacement  a `chromium_src` source that never includes its original
    known        a pair listed in `ForkIndex.KNOWN`, which nothing in the code
                 points at

`_ChromiumImpl` renames whose Brave body never calls the original aren't
indexed; read those by hand.

It also diffs each function a verbatim comment fork copies against the
original's at the new tag, when the range changed the original's function.
Copies that differ from an unchanged original are counted; `--debt` lists them.

    vpython3 tools/cr/codehealth/scripts/forks.py OLD..NEW [--debt]
"""

from __future__ import annotations

import argparse
from collections import defaultdict
from dataclasses import dataclass, field
import difflib
import functools
from pathlib import Path
import re
import subprocess
import sys

import _boot  # noqa: F401  -- adds tools/cr to sys.path
import repository  # noqa: E402  -- after _boot

_SOURCE_SUFFIXES = (
    '.cc',
    '.h',
    '.mm',
    '.ts',
    '.js',
    '.java',
    '.py',
    '.gn',
    '.gni',
    '.html',
    '.css',
)
_SUFFIX_ALTERNATION = '|'.join(s[1:] for s in _SOURCE_SUFFIXES)


@dataclass(frozen=True)
class Fork:
    """A Brave copy of an upstream file, and how it was found."""

    brave: str  # `path:line` in Brave.
    original: str  # The upstream path.
    kind: str
    # A "copied from", "taken from" or "kept in sync with" comment: the copy
    # should still match its original. "Adapted from" ones differ by design.
    verbatim: bool = field(default=False, compare=False)


class ForkIndex:
    """Brave's forks of upstream files, resolved against an upstream tag."""

    # Forks nothing in the code points at. Brave path -> upstream original.
    KNOWN = {
        'browser/net/brave_proxying_url_loader_factory.cc': (
            'extensions/browser/api/web_request/'
            'web_request_proxying_url_loader_factory.cc'
        ),
        'browser/ui/webui/settings/brave_search_engines_handler.cc': (
            'chrome/browser/ui/webui/settings/search_engines_handler.cc'
        ),
        'ios/browser/api/history/history_driver_ios.mm': (
            'ios/chrome/browser/history/ui_bundled/'
            'ios_browsing_history_driver.mm'
        ),
    }

    _COMMENT = re.compile(
        r'(?i)\b(?:copied|taken|adapted) from\b|\bkept in sync with\b'
    )
    _PATH = re.compile(
        rf'(?://)?\b([\w.-]+(?:/[\w.-]+)*\.(?:{_SUFFIX_ALTERNATION}))\b'
    )
    _CLASS = re.compile(r'\b([A-Z][a-z0-9]+(?:[A-Z][a-z0-9]*)+)(?:::\w+)?')

    def __init__(self, tag: str) -> None:
        self._tag = tag

    @functools.cached_property
    def _upstream(self) -> list[str]:
        """Every upstream path at the tag."""
        return repository.chromium.run_git(
            'ls-tree', '-r', '--name-only', self._tag
        ).splitlines()

    @functools.cached_property
    def _paths(self) -> set[str]:
        return set(self._upstream)

    @functools.cached_property
    def _by_name(self) -> dict[str, list[str]]:
        by_name = defaultdict(list)
        for path in self._upstream:
            by_name[Path(path).name].append(path)
        return by_name

    def forks(self) -> list[Fork]:
        """Every fork: the known ones, then the commented, then the
        replacements."""
        forks = [
            Fork(brave, original, 'known')
            for brave, original in self.KNOWN.items()
        ]
        forks += self._comment_forks()
        forks += self._replacements()
        return forks

    def _resolve(self, name: str) -> str | None:
        """An upstream path from a path, a `//` path or a basename."""
        name = name.lstrip('/')
        if name in self._paths:
            return name
        if '/' not in name and len(self._by_name.get(name, ())) == 1:
            return self._by_name[name][0]
        return None

    def _class_file(self, name: str) -> str | None:
        snake = re.sub(
            r'(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])', '_', name
        ).lower()
        for suffix in ('.cc', '.mm', '.ts', '.java'):
            base = (
                f'{name}{suffix}' if suffix == '.java' else f'{snake}{suffix}'
            )
            if path := self._resolve(base):
                return path
        return None

    def _comment_forks(self) -> list[Fork]:
        output = repository.brave.run_git(
            'grep',
            '-n',
            '-A1',
            '-i',
            '-E',
            '(copied|taken|adapted) from|kept in sync with',
            '--',
            *(f'*{s}' for s in _SOURCE_SUFFIXES),
            ':!third_party',
            ':!vendor',
            ':!chromium_src',
            ':!test/data',
            # Brave's own tooling, whose test fixtures read like forks.
            ':!tools/cr',
            no_trim=True,
        )
        forks, pending, verbatim = [], None, False
        for line in output.splitlines():
            match = re.match(r'(.+?)([:-])(\d+)[:-](.*)', line)
            if not match:
                pending = None
                continue
            path, sep, number, text = match.groups()
            if sep == ':' and (comment := self._COMMENT.search(text)):
                verbatim = not comment.group(0).lower().startswith('adapted')
                text = self._COMMENT.split(text, 1)[-1]
                pending = f'{path}:{number}'
            elif pending is None or not path:
                continue
            original = self._original(text)
            if original:
                forks.append(Fork(pending, original, 'comment', verbatim))
                pending = None
            elif sep == '-':
                pending = None
        return forks

    def _original(self, text: str) -> str | None:
        for match in self._PATH.finditer(text):
            if path := self._resolve(match.group(1)):
                return path
        for match in self._CLASS.finditer(text):
            if path := self._class_file(match.group(1)):
                return path
        return None

    def _replacements(self) -> list[Fork]:
        root = repository.brave.root
        forks = []
        for path in repository.brave.run_git(
            'ls-files',
            '--',
            'chromium_src/*.cc',
            'chromium_src/*.mm',
            'chromium_src/*.h',
        ).splitlines():
            original = path[len('chromium_src/') :]
            if original not in self._paths:
                continue
            text = (root / path).read_bytes().decode('utf-8', 'replace')
            if original not in text:
                forks.append(Fork(f'{path}:1', original, 'replacement'))
        return forks


_CALL = re.compile(r'((?:\w+::)*~?\w+)\s*\(')
_CONTROL = {'if', 'for', 'while', 'switch', 'return', 'sizeof', 'defined'}
_COMMENTS = re.compile(r'//[^\n]*|/\*.*?\*/', re.S)
_DIFF_LINES = 12


def copied_function(text: str, line: int) -> tuple[str, str] | None:
    """The name and body of the function whose definition starts within a
    few lines after `line` (1-based), the one a "copied from" comment there
    introduces. A copied snippet inside a function has none."""
    lines = text.splitlines(True)
    offset = sum(len(l) for l in lines[: line - 1])
    for candidate in lines[line - 1 : line + 3]:
        if not candidate.lstrip().startswith(('//', '*', '/*', '#')):
            for match in _CALL.finditer(candidate):
                name = match.group(1).rsplit('::', 1)[-1]
                if name in _CONTROL:
                    continue
                body = function_body(text, name, offset)
                if body is not None and body.startswith(candidate):
                    return name, body
        offset += len(candidate)
    return None


def function_body(text: str, name: str, start: int = 0) -> str | None:
    """The definition of `name` from the start of its line to its closing
    brace, the first one at or after offset `start`."""
    for match in re.finditer(rf'\b{re.escape(name)}\s*\(', text[start:]):
        pos, depth = start + match.end(), 1
        while pos < len(text) and depth:
            depth += {'(': 1, ')': -1}.get(text[pos], 0)
            pos += 1
        # Only qualifiers (`const`, `override`, `-> T`) may separate a
        # definition's parameters from its body.
        pos += re.match(r'(?:[\s\w<>,&*:]|->)*', text[pos:]).end()
        if pos >= len(text) or text[pos] != '{':
            continue  # A declaration or a call.
        depth = 0
        for end in range(pos, len(text)):
            depth += {'{': 1, '}': -1}.get(text[end], 0)
            if depth == 0:
                begin = text.rfind('\n', 0, start + match.start()) + 1
                return text[begin : end + 1]
        return None
    return None


def _normalized(code: str) -> str:
    return ' '.join(_COMMENTS.sub(' ', code).split())


def body_diff(ours: str, theirs: str) -> list[str]:
    """A short diff of two function bodies, or none if only comments or
    whitespace differ."""
    if _normalized(ours) == _normalized(theirs):
        return []
    diff = list(
        difflib.unified_diff(
            ours.splitlines(),
            theirs.splitlines(),
            'brave',
            'upstream',
            n=0,
            lineterm='',
        )
    )[2:]
    if len(diff) > _DIFF_LINES:
        diff = diff[:_DIFF_LINES] + [f'... {len(diff) - _DIFF_LINES} more']
    return diff


class ForkReport:
    """The range's commits to each fork's original."""

    def __init__(self, old: str, new: str, debt: bool = False) -> None:
        self._old, self._new, self._debt = old, new, debt
        self._originals: dict[tuple[str, str], str] = {}

    def lines(self) -> list[str]:
        """The report: each changed original, its forks and its commits."""
        forks = ForkIndex(self._new).forks()
        by_original = defaultdict(list)
        for fork in forks:
            by_original[fork.original].append(fork)
        log = repository.chromium.run_git(
            'log',
            '--abbrev=12',
            '--format=@@@ %h %s',
            '--name-only',
            f'{self._old}..{self._new}',
            '--',
            *sorted(by_original),
            no_trim=True,
        )
        commits = defaultdict(list)
        commit = None
        for line in log.splitlines():
            if line.startswith('@@@ '):
                commit = line[4:]
            elif line in by_original:
                commits[line].append(commit)
        lines = [
            f'{len(forks)} forks indexed, {len(commits)} originals changed '
            'in the range.'
        ]
        for original in sorted(commits):
            lines.append('')
            lines.append(original)
            for fork in by_original[original]:
                lines.append(f'  <- {fork.brave} ({fork.kind})')
            lines += [f'  {c[:120]}' for c in commits[original]]
        diverged = self._diverged(forks)
        if not diverged:
            return lines
        in_range = [d for d in diverged if d[3]]
        debt = [d for d in diverged if not d[3]]
        lines.append('')
        lines.append(
            f'{len(in_range)} copied functions differ from their original at '
            f'{self._new}, which the range changed:'
        )
        lines += self._listed(in_range)
        lines.append('')
        lines.append(
            f'{len(debt)} differ from an original the range didn\'t change'
            + ('' if self._debt or not debt else ' (list them with --debt)')
        )
        if self._debt:
            lines += self._listed(debt)
        return lines

    @staticmethod
    def _listed(diverged: list[tuple[Fork, str, list[str], bool]]) -> list[str]:
        lines = []
        for fork, name, diff, _ in diverged:
            lines.append('')
            lines.append(f'{fork.brave} {name}() <- {fork.original}')
            lines += [f'  {d[:120]}' for d in diff]
        return lines

    def _diverged(
        self, forks: list[Fork]
    ) -> list[tuple[Fork, str, list[str], bool]]:
        """Verbatim copies of a function that no longer match the original at
        the new tag, each with whether the range changed the original's
        function."""
        root = repository.brave.root
        diverged = []
        for fork in forks:
            if fork.kind != 'comment' or not fork.verbatim:
                continue
            path, line = fork.brave.rsplit(':', 1)
            text = (root / path).read_bytes().decode('utf-8', 'replace')
            copied = copied_function(text, int(line))
            if copied is None:
                continue
            name, ours = copied
            theirs = function_body(
                self._original_text(fork.original, self._new), name
            )
            if theirs is None:
                continue
            if diff := body_diff(ours, theirs):
                before = function_body(
                    self._original_text(fork.original, self._old), name
                )
                changed = before is None or body_diff(before, theirs) != []
                diverged.append((fork, name, diff, changed))
        return diverged

    def _original_text(self, original: str, tag: str) -> str:
        """The original's text at `tag`, empty when it isn't there yet."""
        if (original, tag) not in self._originals:
            try:
                text = repository.chromium.run_git(
                    'show', f'{tag}:{original}', no_trim=True
                )
            except subprocess.CalledProcessError:
                text = ''
            self._originals[(original, tag)] = text
        return self._originals[(original, tag)]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('range', help='OLD..NEW Chromium tags.')
    parser.add_argument(
        '--debt',
        action='store_true',
        help='List copies of originals the range didn\'t change.',
    )
    args = parser.parse_args()
    old, _, new = args.range.partition('..')
    if not old or not new:
        parser.error('expected OLD..NEW')
    print('\n'.join(ForkReport(old, new, args.debt).lines()))
    return 0


if __name__ == '__main__':
    sys.exit(main())
