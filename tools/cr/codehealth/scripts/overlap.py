#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Lists the upstream files Brave overrides that a range touches, by file.

Each overridden file is printed once, with how Brave overrides it and the
range commits touching it, so that each override is read once against all of
them. Files are grouped by top-level directory, to split the list across
reviewers. Build files, flag metadata, histograms and strings are left out.

    vpython3 tools/cr/codehealth/scripts/overlap.py OLD..NEW [--kind KIND]
"""

from __future__ import annotations

import argparse
from collections import defaultdict
import json
from pathlib import Path
import posixpath
import re
import sys

import _boot  # noqa: F401  -- adds tools/cr to sys.path
import repository  # noqa: E402  -- after _boot

_NOISE = (
    'BUILD.gn',
    '.gni',
    'DEPS',
    'OWNERS',
    'about_flags.cc',
    'flag-metadata.json',
    'histograms.xml',
    'enums.xml',
    '.grd',
    '.grdp',
)

KINDS = ('chromium_src', 'lit_mangler', 'patch', 'plaster')

_REVERTS = re.compile(r'^This reverts commit ([0-9a-f]{40})', re.M)


def net_zero(log: str, abbrev: int = 12) -> set[str]:
    """The abbreviated hashes of the commits a later commit in the range
    reverts, and of those reverts, from `git log --format=%H%x00%b%x01`."""
    commits = {}
    for record in log.split('\x01'):
        sha, _, body = record.strip().partition('\x00')
        if sha:
            commits[sha] = _REVERTS.findall(body)
    pairs = set()
    for sha, reverted in commits.items():
        for target in reverted:
            if target in commits:
                pairs.update((sha[:abbrev], target[:abbrev]))
    return pairs


class OverrideIndex:
    """The upstream files Brave overrides, with how it overrides each."""

    def __init__(self, root: Path):
        self._kinds: dict[str, set[str]] = defaultdict(set)
        chromium_src = root / 'chromium_src'
        for path in chromium_src.rglob('*'):
            # Brave's own presubmits, which override nothing upstream.
            if path.is_file() and not path.name.startswith('PRESUBMIT'):
                name = path.relative_to(chromium_src).as_posix()
                if name.endswith('.lit_mangler.ts'):
                    self._kinds[name.removesuffix('.lit_mangler.ts')].add(
                        'lit_mangler'
                    )
                else:
                    self._kinds[name].add('chromium_src')
        patches = root / 'patches'
        for info in patches.rglob('*.patchinfo'):
            sub = info.parent.relative_to(patches).as_posix()
            data = json.loads(info.read_bytes().decode('utf-8'))
            for applies in data.get('appliesTo', []):
                self._kinds[
                    posixpath.normpath(posixpath.join(sub, applies['path']))
                ].add('patch')
        rewrite = root / 'rewrite'
        for plaster in rewrite.rglob('*.yaml'):
            upstream = plaster.relative_to(rewrite).with_suffix('')
            self._kinds[upstream.as_posix()].add('plaster')

    @property
    def kinds(self) -> dict[str, set[str]]:
        """`upstream path -> {kind, …}`, kinds as in `KINDS`."""
        return self._kinds

    def touched(
        self,
        log: str,
        kind: str | None = None,
        skip: frozenset[str] | set[str] = frozenset(),
    ) -> dict[str, list[str]]:
        """`path -> [hash, …]` from `git log --format='@@@ %h' --name-only`,
        leaving out the commits in `skip`."""
        touched: dict[str, list[str]] = defaultdict(list)
        commit = None
        for line in log.splitlines():
            if line.startswith('@@@ '):
                commit = line[4:]
            elif (
                commit not in skip
                and line in self._kinds
                and not line.endswith(_NOISE)
                and (kind is None or kind in self._kinds[line])
            ):
                touched[line].append(commit)
        return touched

    def format(self, touched: dict[str, list[str]]) -> str:
        """The `touched` files grouped under `## <top-level dir>/` headings,
        each with its kinds and commits, then the totals."""
        lines: list[str] = []
        top = None
        for path in sorted(touched):
            head = path.split('/', 1)[0]
            if head != top:
                top = head
                lines.append(f'## {head}/')
            kinds = ','.join(sorted(self._kinds[path]))
            lines.append(f'{path} ({kinds}): {" ".join(touched[path])}')
        commits = {c for hashes in touched.values() for c in hashes}
        lines.append(f'{len(touched)} overridden files, {len(commits)} commits')
        return '\n'.join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('range', help='OLD..NEW Chromium tags.')
    parser.add_argument('--kind', choices=KINDS, help='Only this kind.')
    args = parser.parse_args()
    if '..' not in args.range:
        parser.error('expected OLD..NEW')
    log = repository.chromium.run_git(
        'log',
        '--abbrev=12',
        '--format=@@@ %h',
        '--name-only',
        args.range,
        no_trim=True,
    )
    # A commit reverted within the range, and its revert, change nothing.
    bodies = repository.chromium.run_git(
        'log', '--format=%H%x00%b%x01', args.range, no_trim=True
    )
    index = OverrideIndex(repository.brave.root)
    print(index.format(index.touched(log, args.kind, net_zero(bodies))))
    return 0


if __name__ == '__main__':
    sys.exit(main())
