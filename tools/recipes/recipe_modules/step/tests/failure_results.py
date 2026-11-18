# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests that the engine turns a recipe's outcome into the right result."""

from __future__ import annotations

from dataclasses import dataclass

from PB.recipe_engine import result as result_pb2
import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    json,
    step,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    json: json.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    json: json.TEST_API


def RunSteps(api: DEPS) -> result_pb2.RawResult | dict[str, str]:
    raw_result = result_pb2.RawResult()
    try:
        result = api.step('step_result', ['cmd', api.json.output()])
    except api.step.StepFailure as ex:
        step_data = api.step.active_result
        if step_data.json.output:
            raw_result.summary_markdown = step_data.json.output['summary']

        if ex.retcode == 1:
            raw_result.status = result_pb2.FAILURE
        else:
            raw_result.status = result_pb2.INFRA_FAILURE
        if raw_result.summary_markdown == '':
            raise
        return raw_result

    output = result.json.output or {}
    if 'return' in output:
        return {'summary': 'not a RawResult'}
    if 'raise' in output:
        raise api.step.StepFailure(output['raise'])
    if 'crash' in output:
        raise ValueError(output['crash'])
    if output:
        raw_result.summary_markdown = output['summary']
        raw_result.status = result_pb2.SUCCESS
    else:
        raw_result.summary_markdown = 'No json output.'
        raw_result.status = result_pb2.FAILURE
    return raw_result


def GenTests(api: TEST_DEPS):
    yield api.test(
        'successful_result',
        api.step_data(
            'step_result',
            api.json.output({'summary': 'This test should be successful'}),
        ),
        api.post_process(
            post_process.SummaryMarkdown, 'This test should be successful'
        ),
        api.post_process(post_process.StatusSuccess),
    )

    yield api.test(
        'successful_result_no_json',
        api.step_data('step_result', api.json.output({})),
        api.post_process(post_process.SummaryMarkdown, 'No json output.'),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )

    yield api.test(
        'failure_result',
        api.step_data(
            'step_result',
            api.json.output(
                {'summary': 'Failure: step failed at line 90'}, retcode=1
            ),
        ),
        api.post_process(
            post_process.SummaryMarkdown, 'Failure: step failed at line 90'
        ),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )

    yield api.test(
        'infra_failure_result',
        api.step_data(
            'step_result',
            api.json.output({'summary': 'Infra Failure: no memory'}, retcode=2),
        ),
        api.post_process(
            post_process.SummaryMarkdown, 'Infra Failure: no memory'
        ),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )

    yield api.test(
        'failure_result_no_json',
        api.step_data('step_result', api.json.output(None, retcode=1)),
        api.post_process(
            post_process.SummaryMarkdown, "Step('step_result') (retcode: 1)"
        ),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )

    yield api.test(
        'manual_failure',
        api.step_data('step_result', api.json.output({'raise': 'no luck'})),
        api.post_process(post_process.SummaryMarkdown, 'no luck'),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )

    yield api.test(
        'uncaught_exception',
        api.step_data('step_result', api.json.output({'crash': 'boom'})),
        api.post_process(
            post_process.SummaryMarkdown,
            "Uncaught Exception: ValueError('boom')",
        ),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )

    yield api.test(
        'incorrect_object_returned',
        api.step_data('step_result', api.json.output({'return': True})),
        api.post_process(
            post_process.SummaryMarkdownRE,
            (
                r'"<class \'dict\'>" is not a valid return type for recipes\.'
                r' Did you mean to use "RawResult"\?'
            ),
        ),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )

    yield api.test(
        'summary_markdown_too_long',
        api.step_data('step_result', api.json.output({'summary': 'a' * 4001})),
        api.post_process(
            post_process.SummaryMarkdownRE,
            r'^summary_markdown is greater than 4000 bytes \(4001 > 4000\)',
        ),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )
