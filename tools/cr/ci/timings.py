# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""How long each command in a timestamped build log took.

A command is a line a recipe echoes before running something (`>>> ` or
`>>>> `), or a shell step's `+ ` line. It lasts until the next command
starts, or, for the last one, until the log's last timestamped line.
"""

from __future__ import annotations

from dataclasses import dataclass
import datetime
import re

# A line's leading timestamp: Jenkins' Timestamper `[2026-11-23T15:13:09.922Z]`
# or GitHub Actions' `2026-11-23T15:13:09.9220000Z`.
_STAMP = re.compile(r'^\[?(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d)(?:\.(\d+))?Z\]? ?')
_COMMAND = re.compile(r'(?:^|\s)>>>>? |^\+ ')


@dataclass(frozen=True)
class Command:
    """A command a log echoed, when it started and how long it ran."""

    started: datetime.datetime
    seconds: float
    line: str


def _time(match: re.Match) -> datetime.datetime:
    fraction = (match.group(2) or '0')[:6].ljust(6, '0')
    return datetime.datetime.fromisoformat(f'{match.group(1)}.{fraction}+00:00')


def commands(log: str) -> list[Command]:
    """The commands in `log`, in order, with how long each ran. Lines without
    a leading timestamp are left out, so a log read without timestamps has
    none."""
    found: list[tuple[datetime.datetime, str]] = []
    last: datetime.datetime | None = None
    for line in log.splitlines():
        match = _STAMP.match(line)
        if not match:
            continue
        last = _time(match)
        text = line[match.end() :]
        if _COMMAND.search(text):
            found.append((last, text.strip()))
    result = []
    for index, (started, text) in enumerate(found):
        ended = found[index + 1][0] if index + 1 < len(found) else last
        result.append(Command(started, (ended - started).total_seconds(), text))
    return result


def matching(log: str, pattern: str) -> list[Command]:
    """The commands in `log` whose line matches the regular expression
    `pattern`."""
    expression = re.compile(pattern)
    return [
        command for command in commands(log) if expression.search(command.line)
    ]


def format_seconds(seconds: float) -> str:
    """`seconds` as e.g. `7m02s`, or `42s` under a minute."""
    minutes, rest = divmod(round(seconds), 60)
    return f'{minutes}m{rest:02d}s' if minutes else f'{rest}s'
