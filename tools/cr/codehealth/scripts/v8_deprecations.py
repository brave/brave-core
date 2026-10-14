#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Lists the V8 API deprecations between the V8 revisions of two Chromium tags.

Reads `v8_revision` from `src/DEPS` at both tags, and prints each `src/v8`
commit that adds or removes a `V8_DEPRECATE*` marker under `include/`, with the
declaration that follows it. Added `V8_DEPRECATE_SOON` lines are new
deprecations; a removed one next to an added `V8_DEPRECATED` is a promotion; a
removed `V8_DEPRECATED` is usually the API's removal.

    vpython3 tools/cr/codehealth/scripts/v8_deprecations.py OLD..NEW
"""

from __future__ import annotations

import argparse
import re
import sys

import _boot  # noqa: F401  -- adds tools/cr to sys.path
import repository  # noqa: E402  -- after _boot


class V8Deprecations:
    """The `V8_DEPRECATE*` changes of a V8 range."""

    _REVISION = re.compile(r"'v8_revision':\s*'([0-9a-f]{40})'")
    _MARKER = re.compile(r'^[-+].*\bV8_DEPRECAT')
    # Lines shown after a marker: the declaration it applies to.
    _CONTEXT = 2

    def __init__(self, old: str, new: str) -> None:
        self.old, self.new = old, new

    @classmethod
    def revision(cls, deps: str) -> str:
        """The `v8_revision` of a `DEPS` file.

        Raises:
            ValueError: `deps` has no `v8_revision`.
        """
        match = cls._REVISION.search(deps)
        if match is None:
            raise ValueError('DEPS has no v8_revision')
        return match.group(1)

    def range(self) -> str:
        """The V8 `OLD..NEW` range of the two Chromium tags."""
        chromium = repository.chromium
        old, new = (
            self.revision(chromium.run_git('show', f'{tag}:DEPS', no_trim=True))
            for tag in (self.old, self.new)
        )
        return f'{old}..{new}'

    @classmethod
    def parse(cls, log: str) -> list[str]:
        """`git log -p --format='@@@ %h %s'` output → one block per commit
        with markers."""
        blocks, header, shown, pending, current = [], None, [], 0, ''

        def flush() -> None:
            if header and shown:
                blocks.append('\n'.join([header, *shown]))

        for line in log.splitlines():
            if line.startswith('@@@ '):
                flush()
                header, shown, pending = line[4:], [], 0
            elif line.startswith('diff --git '):
                pending = 0
                current = f'    {line.split(" b/", 1)[-1]}'
            elif cls._MARKER.match(line):
                if current not in shown:
                    shown.append(current)
                shown.append(f'      {line}')
                pending = cls._CONTEXT
            elif (
                pending
                and line[:1] in ' +-'
                and not line.startswith(('--- ', '+++ '))
            ):
                shown.append(f'      {line}')
                pending -= 1
        flush()
        return blocks

    @staticmethod
    def summary(v8_range: str, total: int, blocks: list[str]) -> str:
        """The header: the V8 range's size, and how many of its commits change
        a deprecation marker, as 0 of them says little on its own."""
        return (
            f'V8 {v8_range}: {total} commits, {len(blocks)} changing '
            'deprecation markers'
        )

    def total(self) -> int:
        """The number of commits in the V8 range."""
        v8 = repository.Repository(repository.chromium.root / 'v8')
        return int(v8.run_git('rev-list', '--count', self.range()))

    def run(self) -> list[str]:
        """One block per V8 commit that changes a deprecation marker."""
        v8 = repository.chromium.root / 'v8'
        log = repository.Repository(v8).run_git(
            'log',
            '-p',
            '--no-color',
            '--format=@@@ %h %s',
            self.range(),
            '--',
            'include/',
            no_trim=True,
        )
        return self.parse(log)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('range', help='OLD..NEW Chromium tags.')
    args = parser.parse_args()
    old, _, new = args.range.partition('..')
    if not old or not new:
        parser.error('expected OLD..NEW')
    deprecations = V8Deprecations(old, new)
    blocks = deprecations.run()
    print(
        deprecations.summary(deprecations.range(), deprecations.total(), blocks)
    )
    print('\n\n'.join(blocks))
    return 0


if __name__ == '__main__':
    sys.exit(main())
