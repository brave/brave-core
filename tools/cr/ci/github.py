# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The checks GitHub shows on a pull request, and which CI system ran each."""

from __future__ import annotations

from dataclasses import dataclass
import enum

from ci import systems
from gh_cli import GhCli

_BRAVE_CORE = 'brave/brave-core'


class State(enum.StrEnum):
    SUCCESS = 'success'
    FAILURE = 'failure'
    PENDING = 'pending'
    # Neither passed nor failed: skipped, neutral or cancelled.
    NEUTRAL = 'neutral'


# Commit status states, and check run conclusions.
_STATES = {
    'SUCCESS': State.SUCCESS,
    'FAILURE': State.FAILURE,
    'ERROR': State.FAILURE,
    'TIMED_OUT': State.FAILURE,
    'ACTION_REQUIRED': State.FAILURE,
    'STARTUP_FAILURE': State.FAILURE,
    'PENDING': State.PENDING,
    'EXPECTED': State.PENDING,
}


@dataclass(frozen=True)
class Check:
    name: str
    state: State
    url: str

    @property
    def system(self) -> str:
        """The CI system that ran it, as `systems.name()` gives it."""
        return systems.name(self.url)


@dataclass(frozen=True)
class PullRequestChecks:
    number: int
    branch: str
    # The commit the checks ran on.
    head: str
    checks: tuple[Check, ...]

    def in_state(self, state: State) -> list[Check]:
        return [check for check in self.checks if check.state == state]


def _parse_rollup(rollup: list[dict]) -> tuple[Check, ...]:
    """Checks from a `statusCheckRollup`, the latest entry for each name."""
    checks: dict[str, Check] = {}
    for entry in rollup:
        if entry.get('__typename') == 'CheckRun':
            name = entry.get('name', '')
            url = entry.get('detailsUrl') or ''
            if entry.get('status') != 'COMPLETED':
                state = State.PENDING
            else:
                state = _STATES.get(entry.get('conclusion', ''), State.NEUTRAL)
        else:
            name = entry.get('context', '')
            url = entry.get('targetUrl') or ''
            state = _STATES.get(entry.get('state', ''), State.NEUTRAL)
        checks[name] = Check(name, state, url)
    return tuple(checks.values())


def has_pull_request(branch: str) -> bool:
    """Whether brave-core's `branch` has an open pull request."""
    return bool(
        GhCli().list_prs(head=branch, fields='number', repo=_BRAVE_CORE)
    )


def pr_checks(pr: int | str) -> PullRequestChecks:
    """The checks of brave-core's pull request `pr`, a number or a branch."""
    data = GhCli().view_pr(
        pr,
        repo=_BRAVE_CORE,
        fields='number,headRefName,headRefOid,statusCheckRollup',
    )
    return PullRequestChecks(
        number=data['number'],
        branch=data['headRefName'],
        head=data['headRefOid'],
        checks=_parse_rollup(data.get('statusCheckRollup') or []),
    )
