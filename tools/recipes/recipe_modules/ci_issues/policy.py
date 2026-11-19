# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""How what a build found is filed on GitHub: the identity an issue carries,
what is done with an issue when its bug is found again, the labels, and the
wording of issues and comments.

An issue's identity is the key of the request it was filed for, written in
a marker in its body, never its title: a title is cut at GitHub's limit and
says where a bug was seen, which moves. A bug found again is filed on the
issue that carries its key, open or closed, rather than on a new one.
"""

from __future__ import annotations

from dataclasses import dataclass
import enum
import re

from recipe_modules.gh import IssueState, Label, StateReason


class Action(enum.StrEnum):
    """What is done to file a request."""

    CREATE = 'create'
    COMMENT = 'comment'
    REOPEN = 'reopen'


# What is done with the issue a bug was filed on, when it is found again,
# by the issue's state and why it is in it. An issue closed as fixed is
# reopened, as the bug is back; one closed as not planned, or as a
# duplicate, stays closed and hears that it was seen again.
ON_RECURRENCE: dict[tuple[IssueState, StateReason | None], Action] = {
    (IssueState.OPEN, None): Action.COMMENT,
    (IssueState.OPEN, StateReason.REOPENED): Action.COMMENT,
    (IssueState.CLOSED, None): Action.REOPEN,
    (IssueState.CLOSED, StateReason.COMPLETED): Action.REOPEN,
    (IssueState.CLOSED, StateReason.NOT_PLANNED): Action.COMMENT,
    (IssueState.CLOSED, StateReason.DUPLICATE): Action.COMMENT,
}

# The marker that carries an issue's key in its body.
_MARKER = '<!-- ci-issue: {key} -->'
_MARKER_RE = re.compile(r'<!-- ci-issue: (?P<key>\S+) -->')


def marker(key: str) -> str:
    return _MARKER.format(key=key)


def key_of(text: str) -> str | None:
    """The key an issue's body, `text`, carries, if it was filed for a
    request."""
    match = _MARKER_RE.search(text)
    return match['key'] if match else None


class Kind(enum.StrEnum):
    """What an issue is about, which its `bot/type/` label says."""

    ASAN = 'asan'
    LSAN = 'lsan'
    MSAN = 'msan'
    TSAN = 'tsan'
    UBSAN = 'ubsan'
    TEST = 'test'

    @property
    def label(self) -> Label:
        return Label(f'bot/type/{self}', _KIND_COLOURS[self])

    @property
    def short_name(self) -> str:
        """How issues and notifications name the kind, e.g. `ASan`."""
        return _KIND_NAMES[self]

    @property
    def is_sanitizer(self) -> bool:
        return self is not Kind.TEST


_KIND_NAMES: dict[Kind, str] = {
    Kind.ASAN: 'ASan',
    Kind.LSAN: 'LSan',
    Kind.MSAN: 'MSan',
    Kind.TSAN: 'TSan',
    Kind.UBSAN: 'UBSan',
    Kind.TEST: 'Test',
}


_KIND_COLOURS: dict[Kind, str] = {
    Kind.ASAN: 'b60205',
    Kind.LSAN: 'd93f0b',
    Kind.MSAN: 'b60205',
    Kind.TSAN: 'b60205',
    Kind.UBSAN: 'fbca04',
    Kind.TEST: 'c5def5',
}

# The label every issue filed for a build carries.
CI_CONCERN = Label('ci-concern', 'e4e669', 'Found by CI')

# The longest title GitHub keeps, and the most of a body's or comment's
# 65536 characters its folded details take, with room to spare.
MAX_TITLE_LENGTH = 200
MAX_DETAILS_LENGTH = 60_000


@dataclass(frozen=True)
class Where:
    """Where a build saw what it files: for an issue's body and comments."""

    builder: str
    build_url: str

    def link(self) -> str:
        return (
            f'[{self.builder}]({self.build_url})'
            if self.build_url
            else (f'`{self.builder}`')
        )


def title(text: str) -> str:
    """`text` as an issue title, within GitHub's limit."""
    if len(text) <= MAX_TITLE_LENGTH:
        return text
    return text[: MAX_TITLE_LENGTH - 1].rstrip() + '…'


def body(key: str, summary: str, details_title: str, details: str) -> str:
    """An issue's body: its marker, what was found, and the details, folded
    away."""
    return '\n\n'.join([marker(key), summary, _folded(details_title, details)])


def comment(where: Where, seen: str, details_title: str, details: str) -> str:
    """A comment saying the issue's bug was seen again, and where."""
    return '\n\n'.join(
        [
            f'Seen again on {where.link()}: {seen}.',
            _folded(details_title, details),
        ]
    )


def _folded(summary: str, text: str) -> str:
    """`text`, within the details' limit, in a fenced block folded away
    under `summary`."""
    if len(text) > MAX_DETAILS_LENGTH:
        text = text[:MAX_DETAILS_LENGTH] + '\n[... cut ...]'
    fence = '```'
    while fence in text:
        fence += '`'
    return (
        f'<details>\n<summary>{summary}</summary>\n\n'
        f'{fence}\n{text}\n{fence}\n</details>'
    )
