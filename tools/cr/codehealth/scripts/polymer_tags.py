#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Lists the tags Brave overrides with `polymer_overriding`, at two tags.

For each, it says whether upstream's element is still Polymer.

`polymer_overriding` only hooks Polymer elements, so an override of an element
that went Lit silently does nothing. Each `Register*('tag', …)` call, and each
key of a `Register*({'tag': …})` map, in a Brave file importing it is mapped to
the upstream file whose `is` returns the tag, in the same app (a Brave file
under `browser/resources/<app>/` is `chrome/browser/resources/<app>/`'s) or in
`ui/webui/resources`, and sorted into a bucket:

    broke   Polymer at the old tag, Lit (or gone) at the new one
    dead    Lit at both: an earlier lift left it dead
    review  the tag is found at neither tag (renamed: dead too, most likely),
            only in another app's sources (marked `(other app)`: its own app
            dropped it), or the file mixes both base classes
    fine    Polymer at both

Exits with 1 when anything broke.

    vpython3 tools/cr/codehealth/scripts/polymer_tags.py OLD..NEW [--plain]
"""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
import json
import re
import subprocess
import sys

import _boot  # noqa: F401  -- adds tools/cr to sys.path
from plaster import AST_GREP_BIN  # noqa: E402  -- after _boot
import repository  # noqa: E402  -- after _boot

# The tag of a `Register*` call: its first argument, or each key of the map
# that argument is.
_RULES = """
id: tag
language: ts
rule:
  kind: string
  any:
    - nthChild: 1
      inside:
        kind: arguments
        inside: &call
          kind: call_expression
          has:
            field: function
            regex: ^Register\\w+$
    - inside:
        kind: pair
        field: key
        inside:
          kind: object
          nthChild: 1
          inside:
            kind: arguments
            inside: *call
---
id: call
language: ts
rule:
  kind: call_expression
  has:
    field: function
    regex: ^Register\\w+$
    pattern: $FUNCTION
"""

_RESOURCES = ('chrome/browser/resources', 'ui/webui/resources')
# A Brave app's sources, and its upstream app's.
_BRAVE_APP = re.compile(r'^browser/resources/([^/]+)/')
_UPSTREAM_APP = 'chrome/browser/resources/{}/'
_SHARED = 'ui/webui/resources/'
_IS = re.compile(r"return '([a-z][a-z0-9]*(?:-[a-z0-9]+)+)';")
# The same, for `git grep -E`, which has no `(?:`.
_IS_ERE = r"return '[a-z][a-z0-9]*(-[a-z0-9]+)+';"


@dataclass(frozen=True)
class Registration:
    """A tag Brave registers, with the `Register*` function and where."""

    tag: str
    function: str
    where: str

    @property
    def scopes(self) -> tuple[str, ...] | None:
        """The upstream paths its element can be in: its app's and the
        shared ones. None when it isn't in an app."""
        app = _BRAVE_APP.match(self.where)
        if app is None:
            return None
        return (_UPSTREAM_APP.format(app.group(1)), _SHARED)


class BraveRegistrations:
    """The tags Brave's sources register with `polymer_overriding`."""

    def __init__(self, repo: repository.Repository) -> None:
        self._repo = repo

    def collect(self) -> list[Registration]:
        """The registrations of every Brave file importing the module."""
        found: list[Registration] = []
        files = self._repo.run_git(
            'grep', '-l', 'polymer_overriding', '--', '*.ts', '*.js'
        ).splitlines()
        for path in files:
            if path.startswith('ui/webui/resources/polymer_overriding'):
                continue
            text = (self._repo.root / path).read_bytes().decode('utf-8')
            found += self.parse(text, path)
        return found

    @staticmethod
    def parse(text: str, path: str) -> list[Registration]:
        """The registrations in `text`, the source of `path`, by position."""
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
        matches = json.loads(output or '[]')
        calls = [
            (
                m['range']['byteOffset']['start'],
                m['range']['byteOffset']['end'],
                m['metaVariables']['single']['FUNCTION']['text'],
            )
            for m in matches
            if m['ruleId'] == 'call'
        ]
        found: list[Registration] = []
        for m in matches:
            if m['ruleId'] != 'tag':
                continue
            start = m['range']['byteOffset']['start']
            # The innermost call holding the tag.
            function = min(
                (c for c in calls if c[0] <= start < c[1]),
                key=lambda c: c[1] - c[0],
            )[2]
            line = m['range']['start']['line'] + 1
            found.append(
                Registration(m['text'][1:-1], function, f'{path}:{line}')
            )
        return sorted(found, key=lambda r: (r.where, r.tag))


class UpstreamElements:
    """Upstream's WebUI elements at a tag: which file defines each tag, and
    on which base class."""

    # By whether the file mentions `PolymerElement`, then `CrLitElement`.
    _BASES = {
        (True, True): 'mixed',
        (True, False): 'polymer',
        (False, True): 'lit',
        (False, False): 'missing',
    }

    def __init__(self, repo: repository.Repository, tag: str) -> None:
        self._repo = repo
        self._tag = tag
        self._files: dict[str, list[str]] | None = None
        self._bases: dict[str, str] = {}

    def file(
        self, element: str, scopes: tuple[str, ...] | None = None
    ) -> str | None:
        """The first file defining `element` under one of `scopes` (path
        prefixes; anywhere when None), or None."""
        if self._files is None:
            self._files = {}
            output = self._repo.run_git(
                'grep', '-E', _IS_ERE, self._tag, '--', *_RESOURCES
            )
            for line in output.splitlines():
                _, path, text = line.split(':', 2)
                if path.endswith(('_test.ts', '.js')) or '/test/' in path:
                    continue
                for name in _IS.findall(text):
                    self._files.setdefault(name, []).append(path)
        return next(
            (
                path
                for path in self._files.get(element, ())
                if scopes is None or path.startswith(scopes)
            ),
            None,
        )

    def base(self, element: str, scopes: tuple[str, ...] | None = None) -> str:
        """`polymer`, `lit`, `mixed` or `missing`."""
        path = self.file(element, scopes)
        if path is None:
            return 'missing'
        if path not in self._bases:
            text = self._repo.run_git('show', f'{self._tag}:{path}')
            polymer = bool(re.search(r'\bPolymerElement\b', text))
            lit = bool(re.search(r'\bCrLitElement\b', text))
            self._bases[path] = self._BASES[polymer, lit]
        return self._bases[path]


@dataclass(frozen=True)
class Result:
    """A registration's upstream file, and its base class at both tags."""

    registration: Registration
    upstream: str | None
    old: str
    new: str
    # Whether `upstream` is only another app's element of that tag.
    other_app: bool = False

    @property
    def bucket(self) -> str:
        """One of `TagCheck.LISTED`."""
        if (
            self.upstream is None
            or self.other_app
            or 'mixed' in (self.old, self.new)
        ):
            return 'review'
        if self.new == 'polymer':
            return 'fine' if self.old == 'polymer' else 'review'
        return 'broke' if self.old == 'polymer' else 'dead'


class TagCheck:
    """Looks up every registration at both ends of a range."""

    LISTED = ('broke', 'dead', 'review', 'fine')

    def __init__(self, old: str, new: str) -> None:
        chromium = repository.chromium
        self._old = UpstreamElements(chromium, old)
        self._new = UpstreamElements(chromium, new)

    def run(self, registrations: list[Registration]) -> list[Result]:
        """Each registration's result."""
        return [self._result(r) for r in registrations]

    def _result(self, r: Registration) -> Result:
        scopes = r.scopes
        upstream = self._new.file(r.tag, scopes) or self._old.file(
            r.tag, scopes
        )
        if upstream or scopes is None:
            return Result(
                r,
                upstream,
                self._old.base(r.tag, scopes),
                self._new.base(r.tag, scopes),
            )
        elsewhere = self._new.file(r.tag) or self._old.file(r.tag)
        return Result(
            r,
            elsewhere,
            self._old.base(r.tag),
            self._new.base(r.tag),
            other_app=elsewhere is not None,
        )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('range', help='OLD..NEW Chromium tags.')
    parser.add_argument(
        '--plain', action='store_true', help='Leave out the fine rows.'
    )
    args = parser.parse_args()
    old, _, new = args.range.partition('..')
    if not old or not new:
        parser.error('expected OLD..NEW')

    results = TagCheck(old, new).run(
        BraveRegistrations(repository.brave).collect()
    )
    counts = Counter(r.bucket for r in results)
    for bucket in TagCheck.LISTED:
        if args.plain and bucket == 'fine':
            continue
        for r in results:
            if r.bucket == bucket:
                print(
                    '\t'.join(
                        (
                            bucket,
                            r.registration.tag,
                            r.registration.function,
                            r.registration.where,
                            (r.upstream or '-')
                            + (' (other app)' if r.other_app else ''),
                            f'{r.old}->{r.new}',
                        )
                    )
                )
    print(', '.join(f'{counts[b]} {b}' for b in TagCheck.LISTED))
    return 1 if counts['broke'] else 0


if __name__ == '__main__':
    sys.exit(main())
