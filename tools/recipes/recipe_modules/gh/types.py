# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The GitHub resources the `gh` module works with, as GitHub's REST API
describes them."""

from __future__ import annotations

from dataclasses import dataclass
import enum
from typing import Any


class IssueState(enum.StrEnum):
    OPEN = 'open'
    CLOSED = 'closed'


class StateReason(enum.StrEnum):
    """Why an issue is in its state."""

    COMPLETED = 'completed'
    NOT_PLANNED = 'not_planned'
    DUPLICATE = 'duplicate'
    REOPENED = 'reopened'


@dataclass(frozen=True)
class Label:
    name: str
    # The label's colour, as six hexadecimal digits without a `#`.
    color: str = 'ededed'
    description: str = ''

    def to_api(self) -> dict[str, Any]:
        return {
            'name': self.name,
            'color': self.color,
            'description': self.description,
        }

    @classmethod
    def from_api(cls, data: dict[str, Any]) -> Label:
        return cls(
            name=data['name'],
            color=data.get('color') or 'ededed',
            description=data.get('description') or '',
        )


@dataclass(frozen=True)
class Issue:
    number: int
    title: str
    body: str
    state: IssueState
    # None while an issue was never closed or reopened.
    state_reason: StateReason | None
    labels: tuple[str, ...]
    assignees: tuple[str, ...]
    url: str

    @classmethod
    def from_api(cls, data: dict[str, Any]) -> Issue:
        return cls(
            number=data['number'],
            title=data['title'],
            body=data.get('body') or '',
            state=IssueState(data['state']),
            state_reason=(
                StateReason(data['state_reason'])
                if data.get('state_reason')
                else None
            ),
            labels=tuple(label['name'] for label in data.get('labels', [])),
            assignees=tuple(
                assignee['login'] for assignee in data.get('assignees', [])
            ),
            url=data['html_url'],
        )


@dataclass(frozen=True)
class Comment:
    url: str

    @classmethod
    def from_api(cls, data: dict[str, Any]) -> Comment:
        return cls(url=data['html_url'])
