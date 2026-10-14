#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Lists the `EnableFeatureFromString` names a range removed from Blink.

Blink ignores a name it doesn't know, so a runtime feature upstream removes or
renames leaves Brave's string doing nothing. When the range changes
`runtime_enabled_features.json5`, its `name:`s at both tags, read with
ast-grep as JavaScript, are checked against the strings Brave's sources pass.
A name missing at both tags predates the range, and is only counted.

Exits with 1 when the range removed a name Brave passes.

    vpython3 tools/cr/codehealth/scripts/blink_runtime_names.py OLD..NEW
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import subprocess
import sys

import _boot  # noqa: F401  -- adds tools/cr to sys.path
from plaster import AST_GREP_BIN  # noqa: E402  -- after _boot
import repository  # noqa: E402  -- after _boot

JSON5 = 'third_party/blink/renderer/platform/runtime_enabled_features.json5'

# The `name:` of each entry, the json5 read as JavaScript: only `data`'s entries
# have a string `name`, which `parameters` declares as an object. No ancestor is
# matched, as ast-grep labels each one in every match: the `data` array would
# make the output a gigabyte.
_JSON5_NAMES = """
id: name
language: js
rule:
  kind: string_fragment
  inside:
    kind: string
    inside:
      kind: pair
      has: {field: key, regex: '^name$'}
"""

# The name each `EnableFeatureFromString` call passes.
_BRAVE_NAMES = """
id: name
language: cpp
rule:
  kind: string_content
  inside:
    kind: string_literal
    nthChild: 1
    inside:
      kind: argument_list
      inside:
        kind: call_expression
        has: {field: function, regex: 'EnableFeatureFromString$'}
"""


@dataclass(frozen=True)
class Use:
    """A name Brave passes to `EnableFeatureFromString`, and where."""

    name: str
    where: str


def _scan(rules: str, *, text: str | None = None, paths=()) -> list[dict]:
    output = subprocess.run(
        [
            str(AST_GREP_BIN),
            'scan',
            '--inline-rules',
            rules,
            '--json=compact',
            *(['--stdin'] if text is not None else paths),
        ],
        input=text,
        capture_output=True,
        text=True,
        check=True,
    ).stdout
    return json.loads(output or '[]')


def json5_names(text: str) -> set[str]:
    """The runtime feature names a `runtime_enabled_features.json5` defines.

    Wrapped in parentheses, its top-level object is an expression rather than
    a block, so the JavaScript grammar reads it."""
    return {m['text'] for m in _scan(_JSON5_NAMES, text=f'({text})')}


def brave_uses(root: Path) -> list[Use]:
    """The names Brave's sources under `root` pass to
    `EnableFeatureFromString`."""
    paths = subprocess.run(
        ['git', '-C', str(root), 'grep', '-l', 'EnableFeatureFromString', '--']
        + ['*.cc', '*.h', '*.mm'],
        capture_output=True,
        text=True,
        check=False,
    ).stdout.splitlines()
    if not paths:
        return []
    matches = _scan(_BRAVE_NAMES, paths=[str(root / p) for p in paths])
    resolved = root.resolve()
    return sorted(
        (
            Use(
                m['text'],
                f'{Path(m["file"]).resolve().relative_to(resolved).as_posix()}'
                f':{m["range"]["start"]["line"] + 1}',
            )
            for m in matches
        ),
        key=lambda u: u.where,
    )


def check(
    uses: list[Use], old: set[str], new: set[str]
) -> tuple[list[str], int]:
    """The lines for uses whose name the range removed, and how many name
    nothing at either tag."""
    removed = [
        f'removed\t{u.name}\t{u.where}'
        for u in uses
        if u.name in old and u.name not in new
    ]
    stale = sum(u.name not in old and u.name not in new for u in uses)
    return removed, stale


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('range', help='OLD..NEW Chromium tags.')
    args = parser.parse_args()
    old, _, new = args.range.partition('..')
    if not old or not new:
        parser.error('expected OLD..NEW')

    chromium = repository.chromium
    unchanged = (
        subprocess.run(
            [
                'git',
                '-C',
                str(chromium.root),
                'diff',
                '--quiet',
                old,
                new,
                '--',
                JSON5,
            ],
            check=False,
        ).returncode
        == 0
    )
    if unchanged:
        print(f'{JSON5} is unchanged in the range.')
        return 0

    uses = brave_uses(Path(repository.brave.root))
    old_names = json5_names(
        chromium.run_git('show', f'{old}:{JSON5}', no_trim=True)
    )
    new_names = json5_names(
        chromium.run_git('show', f'{new}:{JSON5}', no_trim=True)
    )
    removed, stale = check(uses, old_names, new_names)
    for line in removed:
        print(line)
    print(
        f'{len(removed)} removed in range, {stale} missing at both tags, '
        f'of {len(uses)} EnableFeatureFromString names'
    )
    return 1 if removed else 0


if __name__ == '__main__':
    sys.exit(main())
