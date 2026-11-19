# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `ci_notifications` module."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from PB.recipe_modules.brave.ci_notifications.examples.full import (
    InputProperties,
)
from recipe_api import RecipeScriptApi
from recipe_modules import ci_notifications
from recipe_modules.ci_issues import (
    Action,
    Filed,
    Filing,
    IssueRequest,
    Kind,
    Where,
)
from recipe_test_api import RecipeTestApi

_BUILD_URL = 'https://ci.example/build/7'


@dataclass
class DEPS(RecipeScriptApi):
    ci_notifications: ci_notifications.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    pass


PROPERTIES = InputProperties


def _request(kind: Kind, headline: str) -> IssueRequest:
    return IssueRequest(
        key=f'{kind}:{headline}',
        kind=kind,
        headline=headline,
        title=headline,
        summary='',
        seen='',
        details_title='',
        details='',
    )


def _filing(properties: InputProperties) -> Filing:
    where = Where(
        'linux-x64-asan-brave',
        '' if properties.without_build_url else _BUILD_URL,
    )
    if properties.nothing_found:
        return Filing(where)
    return Filing(
        where=where,
        filed=(
            Filed(
                _request(
                    Kind.ASAN, 'heap-use-after-free in <skus::SDK>::summary'
                ),
                Action.CREATE,
                'https://github.com/o/r/issues/1',
            ),
            Filed(
                _request(Kind.LSAN, 'direct-leak in brave::Bar::Make'),
                Action.COMMENT,
                'https://github.com/o/r/issues/2#issuecomment-1',
            ),
            # CI gave no repository to file it in.
            Filed(_request(Kind.UBSAN, 'signed integer overflow')),
            Filed(
                _request(Kind.TEST, 'BarTest.Fails'),
                Action.CREATE,
                'https://github.com/o/t/issues/3',
            ),
            Filed(
                _request(Kind.TEST, 'BazTest.Restarts'),
                Action.REOPEN,
                'https://github.com/o/t/issues/4#issuecomment-1',
            ),
        ),
        withheld=(
            '20 tests failed for 20 reasons, more than the 15 this files',
        ),
    )


def RunSteps(api: DEPS, properties: InputProperties):
    api.ci_notifications.report(_filing(properties))


def _messages(check, value: list, expected: dict[str, list[str]]) -> bool:
    """Whether the messages are `expected`, lines by audience."""
    return check(
        {message['audience']: message['text'].split('\n') for message in value}
        == expected
    )


def GenTests(api: TEST_DEPS):
    link = f'<{_BUILD_URL}|linux-x64-asan-brave>'
    # Each audience hears its part: every finding, the new ones, the test
    # issues, the new test issues, and what was withheld; markup in what was
    # found is escaped.
    yield api.test(
        'basic',
        api.post_process(
            post_process.PropertyMatchesCallable,
            'notifications',
            lambda check, value: _messages(
                check,
                value,
                {
                    'findings': [
                        f'Sanitizer findings on {link}:',
                        '• ASan: heap-use-after-free in &lt;skus::SDK&gt;::summary'
                        ' (<https://github.com/o/r/issues/1|GitHub issue>)',
                        '• LSan: direct-leak in brave::Bar::Make'
                        ' (<https://github.com/o/r/issues/2#issuecomment-1|GitHub'
                        ' issue>)',
                        '• UBSan: signed integer overflow',
                    ],
                    'new-findings': [
                        f'New sanitizer findings on {link}:',
                        '• ASan: heap-use-after-free in &lt;skus::SDK&gt;::summary'
                        ' (<https://github.com/o/r/issues/1|GitHub issue>)',
                    ],
                    'build-failure': [
                        'Issues for the tests that failed in this build, for '
                        'reasons other than a sanitizer finding:',
                        '• Test: BarTest.Fails'
                        ' (<https://github.com/o/t/issues/3|GitHub issue>)',
                        '• Test: BazTest.Restarts'
                        ' (<https://github.com/o/t/issues/4#issuecomment-1|GitHub'
                        ' issue>)',
                    ],
                    'new-test-issues': [
                        f'Issues opened for the tests that failed on {link}:',
                        '• Test: BarTest.Fails'
                        ' (<https://github.com/o/t/issues/3|GitHub issue>)',
                    ],
                    'ci': [
                        f'Not filed for {link}: 20 tests failed for 20 reasons,'
                        ' more than the 15 this files.'
                    ],
                },
            ),
        ),
    )
    # A build without a page is named as its builder.
    yield api.test(
        'without build url',
        api.properties(without_build_url=True),
        api.post_process(
            post_process.PropertyMatchesCallable,
            'notifications',
            lambda check, value: check(
                value[0]['text'].startswith(
                    'Sanitizer findings on `linux-x64-asan-brave`:'
                )
            ),
        ),
        api.post_process(post_process.DropExpectation),
    )
    # A build that found nothing has nothing to say.
    yield api.test(
        'nothing found',
        api.properties(nothing_found=True),
        api.post_process(post_process.PropertyEquals, 'notifications', []),
        api.post_process(post_process.DropExpectation),
    )
