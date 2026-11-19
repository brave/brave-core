# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `flake_analysis` module."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from PB.recipe_modules.brave.flake_analysis.examples.full import (
    InputProperties,
)
from recipe_api import RecipeScriptApi
from recipe_modules import cas, flake_analysis, isolate, platform
from recipe_modules.brave_tests import SuiteResult, TestRun
from recipe_test_api import RecipeTestApi
from test_results import SuiteOutcome, TestFailure


@dataclass
class DEPS(RecipeScriptApi):
    flake_analysis: flake_analysis.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    cas: cas.TEST_API
    flake_analysis: flake_analysis.TEST_API
    platform: platform.TEST_API


PROPERTIES = InputProperties


def _failure(name: str, flaky: bool = False) -> TestFailure:
    return TestFailure(
        name=name, flaky=flaky, status='FAILURE', message='', output=''
    )


def _suite(name: str, *failures: TestFailure) -> SuiteResult:
    return SuiteResult(
        name=name,
        passed=all(f.flaky for f in failures),
        outcome=SuiteOutcome(counts={}, failures=failures),
        sanitizer_reports=None,
    )


def _test_run(properties: InputProperties) -> TestRun:
    if properties.all_pass:
        return TestRun((_suite('unit_tests'),))
    browser_tests = (
        [_failure(f'Many.Test{i}') for i in range(101)]
        if properties.many_failures
        else [
            _failure('Upstream.Flaky', flaky=True),
            _failure('Upstream.Fails'),
        ]
    )
    return TestRun(
        (
            _suite('browser_tests', *browser_tests),
            _suite(
                'brave_unit_tests',
                _failure('Brave.Flaky'),
                _failure('Brave.Once'),
            ),
            _suite('base_unittests', _failure('Base.NoIsolate')),
            SuiteResult('crashed_tests', False, None, None),
            _suite(
                'interactive_ui_tests', _failure('Retried.Once', flaky=True)
            ),
        )
    )


def _rerunnable(suite: str) -> flake_analysis.Rerunnable:
    return flake_analysis.Rerunnable(
        isolate=isolate.Isolate(
            digest=f'{"a" * 64}/100',
            command=('vpython3', '../../testing/xvfb.py', f'./{suite}'),
            cwd='out/Release',
        ),
        args=(
            '--test-launcher-filter-file=../../brave/test/filters/x.filter',
            '--a',
        ),
    )


def RunSteps(api: DEPS, properties: InputProperties):
    test_run = _test_run(properties)
    analysis = api.flake_analysis.analyse(
        test_run,
        {
            suite: _rerunnable(suite)
            for suite in (
                'browser_tests',
                'brave_unit_tests',
                'interactive_ui_tests',
            )
        },
    )
    exonerated = analysis.exonerate(test_run)
    if properties.all_pass or properties.many_failures:
        assert exonerated == test_run
        return
    by_name = {suite.name: suite for suite in exonerated.suites}
    # Both failures are upstream's too, so `browser_tests` passes.
    assert by_name['browser_tests'].passed
    assert not by_name['browser_tests'].outcome.failures
    # Brave's own failures stay.
    assert not by_name['brave_unit_tests'].passed
    assert len(by_name['brave_unit_tests'].outcome.failures) == 2
    assert by_name['base_unittests'] == test_run.suites[2]


def GenTests(api: TEST_DEPS):
    yield api.test(
        'verdicts',
        api.platform.name('linux'),
        api.cas.configured(),
        api.flake_analysis.rerun('browser_tests', {'Upstream.Fails': 10}),
        api.flake_analysis.upstream_status(
            'browser_tests', 'Upstream.Flaky', 'stable', 'flaky', retcode=0
        ),
        api.flake_analysis.upstream_status(
            'browser_tests', 'Upstream.Fails', 'failing'
        ),
        api.flake_analysis.rerun(
            'brave_unit_tests', {'Brave.Flaky': 3, 'Brave.Once': 0}
        ),
        api.flake_analysis.upstream_status(
            'brave_unit_tests', 'Brave.Flaky', 'stable', retcode=3
        ),
        api.flake_analysis.upstream_status(
            'brave_unit_tests', 'Brave.Once', retcode=1
        ),
        api.post_process(
            post_process.StepCommandRE,
            'flake analysis.browser_tests.rerun.run',
            [
                'vpython3',
                r'.*xvfb\.py',
                r'\./browser_tests',
                '--a',
                '--gtest_filter=Upstream.Fails',
                '--gtest_repeat=10',
                '--test-launcher-retry-limit=0',
                '--test-launcher-batch-limit=1',
                r'--test-launcher-summary-output=.*summary\.json',
            ],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'flake analysis.brave_unit_tests.rerun.run',
            ['--gtest_filter=Brave.Flaky:Brave.Once'],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'flake analysis.browser_tests.upstream Upstream.Fails',
            [
                '--suite',
                '^browser_tests$',
                '--builder',
                '^(linux-rel|Linux Tests)$',
            ],
        ),
        api.post_process(
            post_process.DoesNotRun, 'flake analysis.base_unittests.rerun'
        ),
        # A test that passed on its retry is flaky already.
        api.post_process(
            post_process.DoesNotRun, 'flake analysis.interactive_ui_tests.rerun'
        ),
        api.post_process(post_process.MustRun, 'flake analysis.verdicts'),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'all pass',
        api.properties(all_pass=True),
        api.post_process(post_process.DoesNotRun, 'flake analysis'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    yield api.test(
        'too many to rerun',
        api.properties(many_failures=True),
        api.platform.name('mac'),
        api.cas.configured(),
        api.post_process(
            post_process.DoesNotRun, 'flake analysis.browser_tests.rerun'
        ),
        api.post_process(
            post_process.DoesNotRun,
            'flake analysis.browser_tests.upstream Many.Test0',
        ),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    yield api.test(
        'rerun results missing',
        api.properties(
            **{'$flake_analysis': {'upstream_builders': '^linux-rel$'}}
        ),
        api.cas.configured(),
        api.flake_analysis.rerun_missing('browser_tests'),
        api.flake_analysis.upstream_status(
            'browser_tests', 'Upstream.Flaky', 'flaky'
        ),
        api.flake_analysis.upstream_status(
            'browser_tests', 'Upstream.Fails', 'flaky'
        ),
        api.flake_analysis.rerun('brave_unit_tests', {'Brave.Flaky': 1}),
        api.post_process(
            post_process.StepCommandContains,
            'flake analysis.browser_tests.upstream Upstream.Fails',
            ['--builder', '^linux-rel$'],
        ),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
