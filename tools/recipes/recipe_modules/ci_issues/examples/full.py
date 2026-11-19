# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `ci_issues` module."""

from __future__ import annotations

from dataclasses import dataclass
import string

import post_process
from PB.recipe_modules.brave.ci_issues.examples.full import InputProperties
from recipe_api import RecipeScriptApi
from recipe_modules import ci_issues, gh
from recipe_modules.ci_issues import policy, requests
from recipe_modules.sanitizer_findings import Findings
from recipe_test_api import RecipeTestApi
from sanitizer_reports import Frame, Report, RunReports, Sanitizer
from test_results import SuiteOutcome, TestFailure

_SANITIZER_REPO = 'brave/sanitizer-findings'
_TEST_REPO = 'brave/test-failures'


@dataclass
class DEPS(RecipeScriptApi):
    ci_issues: ci_issues.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    ci_issues: ci_issues.TEST_API
    gh: gh.TEST_API


PROPERTIES = InputProperties


def _failure(
    name: str, message: str, *, flaky: bool = False, output: str = ''
) -> TestFailure:
    return TestFailure(
        name=name,
        flaky=flaky,
        status='FAILURE',
        message=message,
        output=output or f'{name} broke',
    )


def _inputs(
    extra_failing_tests: int,
) -> tuple[Findings, dict[str, SuiteOutcome]]:
    """A build's findings and how its suites' tests did."""
    use_after_free = Report(
        sanitizer=Sanitizer.ADDRESS,
        kind='heap-use-after-free',
        summary='heap-use-after-free ../../brave/foo.cc:12:3',
        frames=(Frame('brave::Foo::Bar', 'brave/foo.cc:12'),),
        text='==1==ERROR: AddressSanitizer: heap-use-after-free',
        test='FooTest.Frees',
    )
    overflow = Report(
        sanitizer=Sanitizer.UNDEFINED,
        kind='signed integer overflow',
        summary='undefined-behavior',
        frames=(Frame('__ubsan_handle_add_overflow'),),
        text='runtime error: signed integer overflow',
    )
    leak = Report(
        sanitizer=Sanitizer.LEAK,
        kind='direct-leak',
        summary='8 byte(s) leaked in 1 allocation(s).',
        frames=(Frame('brave::Bar::Make', 'brave/bar.cc:3'),),
        text='Direct leak of 8 byte(s)',
    )
    findings = Findings.gather(
        {
            'brave_unit_tests': RunReports((use_after_free, overflow)),
            # Its sanitiser ran out of memory: its own findings are withheld.
            'brave_components_unittests': RunReports(
                (leak,), unreliable=('AddressSanitizer failed to allocate',)
            ),
        }
    )
    failures = [
        # Explained by the use-after-free finding.
        _failure('FooTest.Frees', 'CRASH'),
        _failure('BarTest.Fails', 'bar.cc(3): Expected true'),
        _failure('BazTest.PRE_Restarts', 'baz.cc(9): Expected 1'),
        _failure('FlakyTest.Passes', 'flaky.cc(1): Expected 2', flaky=True),
        # Its name is longer than a title, and its output than an issue, and
        # quotes a fenced block.
        _failure(
            'LongTest.' + 'Named' * 50,
            'long.cc(1): Expected x',
            output='```\n' + 'x' * 70_000,
        ),
        # One reason, once the timeouts' numbers are left out.
        *(
            _failure(f'WaitTest.Times{n}', f'Timed out after {n}000 ms')
            for n in range(3, 6)
        ),
        *(
            _failure(f'ExtraTest.T{n}', f'Check failed: {letter}')
            for n, letter in zip(
                range(extra_failing_tests), string.ascii_lowercase * 2
            )
        ),
    ]
    outcomes = {
        'brave_unit_tests': SuiteOutcome(
            counts={'passed': 1, 'failed': len(failures)},
            failures=tuple(failures),
        )
    }
    return findings, outcomes


def RunSteps(api: DEPS, properties: InputProperties):
    findings, outcomes = _inputs(properties.extra_failing_tests)
    api.ci_issues.file(
        builder='linux-x64-asan-brave', findings=findings, outcomes=outcomes
    )


def _keys() -> dict[str, str]:
    """The keys of the example's requests, by title."""
    findings, outcomes = _inputs(0)
    made = (
        *requests.from_sanitizer_findings(findings).requests,
        *requests.from_test_failures(
            outcomes,
            explained=(t for f in findings.findings for t in f.tests),
        ).requests,
    )
    return {request.title: request.key for request in made}


def _filed(check, value: dict, expected: dict[str, str]) -> bool:
    """Whether what was filed is `expected`, actions by key."""
    return check(
        {entry['key']: entry['action'] for entry in value['filed']} == expected
    )


def GenTests(api: TEST_DEPS):
    keys = _keys()
    every_key = list(keys.values())
    use_after_free = keys[
        'ASan finding: heap-use-after-free in brave::Foo::Bar'
    ]
    overflow = keys['UBSan finding: signed integer overflow']
    fails = keys['Test failure: BarTest.Fails']
    restarts = keys['Test failure: BazTest.Restarts']
    waits = keys['Test failures: Timed out after N ms']

    def body(key: str) -> str:
        return f'{policy.marker(key)}\n\nFiled before.'

    # Without repositories from CI, nothing is filed; what would be withheld
    # still is.
    yield api.test(
        'no repositories',
        api.post_process(post_process.DoesNotRunRE, r'.*create issue.*'),
        api.post_process(
            post_process.PropertyMatchesCallable,
            'ci_issues',
            lambda check, value: check(
                value['filed'] == []
                and len(value['withheld']) == 1
                and 'ran out of memory' in value['withheld'][0]
            ),
        ),
        api.post_process(post_process.DropExpectation),
    )
    # Each bug is filed once: a finding by its signature, a failing test by
    # its name, and tests failing for one reason together; a flaky test, and
    # one a finding explains, are not.
    yield api.test(
        'new',
        api.ci_issues.repositories(labels=('bot/branch/master',)),
        api.step_data(
            'file issues.list labels (brave/test-failures)',
            api.gh.labels('ci-concern', 'bot/type/test'),
        ),
        api.post_process(
            post_process.PropertyMatchesCallable,
            'ci_issues',
            lambda check, value: _filed(
                check, value, {key: 'create' for key in every_key}
            ),
        ),
        api.post_process(
            post_process.MustRun,
            'file issues.create label bot/type/asan (brave/sanitizer-findings)',
            'file issues.create label bot/branch/master (brave/test-failures)',
        ),
        api.post_process(
            post_process.DoesNotRun,
            'file issues.create label bot/type/test (brave/test-failures)',
        ),
    )
    # A bug found again is filed on its issue: an open one hears it, one
    # closed as fixed is reopened, one closed as not planned is not, and of a
    # key's issues the open one is used.
    yield api.test(
        'found again',
        api.ci_issues.repositories(),
        api.step_data(
            'file issues.list bot/type/asan issues (brave/sanitizer-findings)',
            api.gh.issues(
                api.gh.issue(5, body=body(use_after_free)),
                api.gh.issue(6, body='Filed by hand, without a key.'),
            ),
        ),
        api.step_data(
            'file issues.list bot/type/ubsan issues (brave/sanitizer-findings)',
            api.gh.issues(
                api.gh.issue(
                    20,
                    body=body(overflow),
                    state='closed',
                    state_reason='completed',
                ),
                api.gh.issue(
                    21,
                    body=body(overflow),
                    state='closed',
                    state_reason='completed',
                ),
            ),
        ),
        api.step_data(
            'file issues.list bot/type/test issues (brave/test-failures)',
            api.gh.issues(
                api.gh.issue(
                    9,
                    body=body(fails),
                    state='closed',
                    state_reason='completed',
                    labels=('ci-concern', 'bot/type/test'),
                ),
                api.gh.issue(
                    10,
                    body=body(waits),
                    state='closed',
                    state_reason='not_planned',
                ),
                api.gh.issue(
                    3,
                    body=body(restarts),
                    state='closed',
                    state_reason='completed',
                ),
                api.gh.issue(12, body=body(restarts)),
                api.gh.issue(11, body=body(restarts)),
                api.gh.issue(
                    13,
                    body=body(restarts),
                    state='closed',
                    state_reason='completed',
                ),
            ),
        ),
        api.post_process(
            post_process.PropertyMatchesCallable,
            'ci_issues',
            lambda check, value: _filed(
                check,
                value,
                {
                    **{key: 'create' for key in every_key},
                    use_after_free: 'comment',
                    overflow: 'reopen',
                    fails: 'reopen',
                    restarts: 'comment',
                    waits: 'comment',
                },
            ),
        ),
        api.post_process(
            post_process.MustRun,
            'file issues.reopen issue #9 (brave/test-failures)',
            'file issues.reopen issue #21 (brave/sanitizer-findings)',
            'file issues.comment on issue #11 (brave/test-failures)',
            'file issues.label issue #5 (brave/sanitizer-findings)',
        ),
        api.post_process(
            post_process.DoesNotRun,
            'file issues.reopen issue #10 (brave/test-failures)',
            'file issues.label issue #9 (brave/test-failures)',
            'file issues.comment on issue #12 (brave/test-failures)',
        ),
    )
    # A build with more failing tests than issues are filed for files none
    # for its tests, and says why.
    yield api.test(
        'too many failing tests',
        api.ci_issues.repositories(),
        api.properties(extra_failing_tests=20),
        api.post_process(
            post_process.DoesNotRunRE, r'.*\(brave/test-failures\)'
        ),
        api.post_process(
            post_process.PropertyMatchesCallable,
            'ci_issues',
            lambda check, value: check(
                any(
                    'more than the 15' in reason for reason in value['withheld']
                )
            ),
        ),
        api.post_process(post_process.DropExpectation),
    )
