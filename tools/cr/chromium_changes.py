# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The "Chromium changes:" section `git cr commit --culprit` writes.

The commit hook writes the culprits' links, then each culprit as
`git log -1 <hash>` prints it: a `commit <hash>` line, its headers, a blank
line, and its message indented by four spaces. The indentation keeps git from
reading the culprit's trailers, its Change-Id among them, as the commit's own.

Standard library only, as the commit hook uses it.
"""

from __future__ import annotations

import re

HEADING = 'Chromium changes:'

_COMMIT = re.compile(r'commit [0-9a-f]{40}\b.*')
_HEADER = re.compile(r'[A-Z][A-Za-z]*: .*')
_LINK = re.compile(r'https?://\S+')
_INDENT = '    '

HINT = (
    'The hook writes the "Chromium changes:" block: leave it out of the '
    'message, and pass the upstream commits to blame instead, as in\n'
    '    git cr commit --culprit=<hash>[,<hash>...] -m "<subject>" '
    '-m "<body>"\n'
    'It quotes each one as `git log -1 <hash>` prints it, its message '
    'indented by four spaces, so that git does not read the culprit\'s '
    'trailers (its Change-Id among them) as this commit\'s own. To keep the '
    'block of an existing commit (in an `amend!` message), copy it from '
    '`git log -1 --format=%B <commit>`, indentation included.'
)


def _blocks(lines: list[str]) -> list[tuple[int, int]]:
    """For each culprit quoted after a heading, the index of its `commit`
    line and of the first line of its message, or of where it would be."""
    blocks = []
    for heading, line in enumerate(lines):
        if line.rstrip() != HEADING:
            continue
        i = heading + 1
        while i < len(lines) and (
            not lines[i].strip() or _LINK.fullmatch(lines[i].strip())
        ):
            i += 1
        while i < len(lines) and _COMMIT.fullmatch(lines[i]):
            commit = i
            i += 1
            while i < len(lines) and _HEADER.fullmatch(lines[i]):
                i += 1
            while i < len(lines) and not lines[i].strip():
                i += 1
            blocks.append((commit, i))
            while i < len(lines) and (
                not lines[i].strip() or lines[i].startswith(_INDENT)
            ):
                i += 1
            # Messages written by hand can put each link before its quote.
            while i < len(lines) and (
                not lines[i].strip() or _LINK.fullmatch(lines[i].strip())
            ):
                i += 1
    return blocks


def problem(message: str) -> str | None:
    """Says how the message's "Chromium changes:" sections differ from what
    the hook writes, if they do."""
    lines = message.split('\n')
    for heading, line in enumerate(lines):
        if line.rstrip() != HEADING:
            continue
        i = heading + 1
        if i == len(lines) or not _LINK.fullmatch(lines[i].strip()):
            return (
                f'"{HEADING}" is not followed by the culprits\' links.\n{HINT}'
            )
        while i < len(lines) and _LINK.fullmatch(lines[i].strip()):
            i += 1
        while i < len(lines) and not lines[i].strip():
            i += 1
        if i == len(lines) or not _COMMIT.fullmatch(lines[i]):
            return (
                f'"{HEADING}" does not quote its culprits as '
                f'`git log -1 <hash>` prints them.\n{HINT}'
            )
    for commit, start in _blocks(lines):
        if start == len(lines) or not lines[start].startswith(_INDENT):
            culprit = lines[commit].split()[1]
            return (
                f'The message of culprit {culprit} is quoted without its '
                f'indentation.\n{HINT}'
            )
    return None


def fixed(message: str) -> str:
    """The message with each culprit message quoted without its indentation
    indented, up to the next culprit, the next "Chromium changes:" or the end
    of the message. Nothing of the commit's own follows such a quote: the
    hook took the culprit's trailers for the commit's, and added none."""
    lines = message.split('\n')
    # Each pass indents the first quote that lacks it, after which the
    # culprits after it are found again.
    while (
        start := next(
            (
                start
                for _, start in _blocks(lines)
                if start < len(lines) and not lines[start].startswith(_INDENT)
            ),
            None,
        )
    ) is not None:
        i = start
        while (
            i < len(lines)
            and not _COMMIT.fullmatch(lines[i])
            and lines[i].rstrip() != HEADING
        ):
            if lines[i].strip():
                lines[i] = _INDENT + lines[i]
            i += 1
    return '\n'.join(lines)
