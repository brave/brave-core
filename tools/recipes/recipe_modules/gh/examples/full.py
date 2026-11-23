# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `gh` module."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import gh, step
from recipe_test_api import RecipeTestApi

_REPO = 'brave/test'


@dataclass
class DEPS(RecipeScriptApi):
    gh: gh.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    gh: gh.TEST_API


def RunSteps(api: DEPS):
    api.gh.ensure_labels(
        _REPO, [gh.Label('bot/type/asan', 'b60205'), gh.Label('ci-concern')]
    )
    for issue in api.gh.list_issues(_REPO, labels=['bot/type/asan']):
        api.step(
            f'found #{issue.number}',
            ['echo', issue.state, str(issue.state_reason), *issue.labels],
        )
        if issue.state is gh.IssueState.CLOSED:
            api.gh.reopen_issue(_REPO, issue.number)
        api.gh.comment(_REPO, issue.number, 'Seen again.')
        api.gh.add_labels(_REPO, issue.number, ['ci-concern'])
    created = api.gh.create_issue(
        _REPO,
        title='ASan finding',
        body='Found.',
        labels=['bot/type/asan'],
        assignees=['someone'],
    )
    api.step('created', ['echo', created.url])


def GenTests(api: TEST_DEPS):
    # Issues are listed from the issues endpoint, every page of them, without
    # pull requests; a closed one is reopened.
    yield api.test(
        'basic',
        api.step_data(
            'list labels (brave/test)', api.gh.labels('bot/type/asan')
        ),
        api.step_data(
            'list bot/type/asan issues (brave/test)',
            api.gh.issues(
                api.gh.issue(
                    7,
                    state='closed',
                    state_reason='completed',
                    labels=('bot/type/asan',),
                ),
                {**api.gh.issue(8), 'pull_request': {}},
            ),
        ),
        api.post_process(
            post_process.StepCommandContains,
            'list bot/type/asan issues (brave/test)',
            [
                'gh',
                'api',
                '--paginate',
                '--jq',
                '.[]',
                'repos/brave/test/issues?state=all&labels=bot%2Ftype%2Fasan'
                '&per_page=100',
            ],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'found #7',
            ['echo', 'closed', 'completed', 'bot/type/asan'],
        ),
        api.post_process(post_process.DoesNotRun, 'found #8'),
        api.post_process(post_process.MustRun, 'reopen issue #7 (brave/test)'),
        api.post_process(
            post_process.MustRun, 'create label ci-concern (brave/test)'
        ),
        api.post_process(
            post_process.DoesNotRun, 'create label bot/type/asan (brave/test)'
        ),
        api.post_process(
            post_process.StepCommandContains,
            'created',
            ['echo', 'https://github.com/brave/test/issues/1'],
        ),
    )
    # An open issue is commented on as it is.
    yield api.test(
        'open issue',
        api.step_data(
            'list bot/type/asan issues (brave/test)',
            api.gh.issues(api.gh.issue(3)),
        ),
        api.post_process(
            post_process.DoesNotRun, 'reopen issue #3 (brave/test)'
        ),
        api.post_process(
            post_process.MustRun, 'comment on issue #3 (brave/test)'
        ),
        api.post_process(post_process.DropExpectation),
    )
