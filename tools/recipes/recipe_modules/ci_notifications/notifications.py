# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The messages a build has for the people following it, in Slack's markup.

A message is addressed to an `Audience`, which says who it is for, never
where it goes: CI sends each audience's messages to the channels it keeps
for it. The build's own result, which takes its earlier builds to tell
whether it is news, is CI's to announce; these are what only the build
knows: what it found, and what it filed.
"""

from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass
import enum
from typing import Any

from recipe_modules.ci_issues import Action, Filed, Filing, Where


class Audience(enum.StrEnum):
    """Who a message is for."""

    # Those following the builder's sanitiser findings: all of them.
    FINDINGS = 'findings'
    # Those who triage sanitiser findings: the ones seen for the first time.
    NEW_FINDINGS = 'new-findings'
    # Those reading the build's failure: the issues its failing tests are
    # filed on, as a reply to CI's announcement of the build's result.
    BUILD_FAILURE = 'build-failure'
    # Those who triage failing tests: the issues opened for them.
    NEW_TEST_ISSUES = 'new-test-issues'
    # Those who look after CI itself: what the build withheld from filing.
    CI = 'ci'


@dataclass(frozen=True)
class Notification:
    audience: Audience
    # The message, in Slack's markup.
    text: str

    def to_json(self) -> dict[str, Any]:
        return {'audience': str(self.audience), 'text': self.text}


def compose(filing: Filing) -> tuple[Notification, ...]:
    """The messages for what a build found and filed."""
    link = _link(filing.where)
    findings = [
        entry for entry in filing.filed if entry.request.kind.is_sanitizer
    ]
    tests = [
        entry
        for entry in filing.filed
        if not entry.request.kind.is_sanitizer and entry.url
    ]
    created = [entry for entry in filing.filed if entry.action is Action.CREATE]
    messages = [
        (Audience.FINDINGS, f'Sanitizer findings on {link}:', findings),
        (
            Audience.NEW_FINDINGS,
            f'New sanitizer findings on {link}:',
            [entry for entry in created if entry.request.kind.is_sanitizer],
        ),
        (
            Audience.BUILD_FAILURE,
            'Issues for the tests that failed in this build, for reasons other '
            'than a sanitizer finding:',
            tests,
        ),
        (
            Audience.NEW_TEST_ISSUES,
            f'Issues opened for the tests that failed on {link}:',
            [entry for entry in created if not entry.request.kind.is_sanitizer],
        ),
    ]
    return (
        *(
            Notification(audience, _listed(heading, entries))
            for audience, heading, entries in messages
            if entries
        ),
        *(
            Notification(
                Audience.CI, f'Not filed for {link}: {_escaped(reason)}.'
            )
            for reason in filing.withheld
        ),
    )


def _listed(heading: str, entries: Iterable[Filed]) -> str:
    return '\n'.join([heading, *(_bullet(entry) for entry in entries)])


def _bullet(entry: Filed) -> str:
    request = entry.request
    issue = f' (<{entry.url}|GitHub issue>)' if entry.url else ''
    return f'• {request.kind.short_name}: {_escaped(request.headline)}{issue}'


def _link(where: Where) -> str:
    if not where.build_url:
        return f'`{where.builder}`'
    return f'<{where.build_url}|{_escaped(where.builder)}>'


def _escaped(text: str) -> str:
    """`text` as Slack shows it as it is: its markup characters escaped."""
    return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
