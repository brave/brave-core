#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import unittest

import _boot  # noqa: F401  -- adds parent tools/cr to sys.path
from ci import failures  # noqa: E402  -- after _boot
from ci.failures import (  # noqa: E402  -- after _boot
    CheckFailures,
    FailedBuild,
    FailedTest,
    Kind,
)


class ClassifyTest(unittest.TestCase):
    _TEST = (FailedTest('Suite.Test', 'FAILURE'),)

    def test_workspace_switching_build_systems_is_infra(self):
        log = '[build] Run gn clean before switching from ninja to siso in out'
        self.assertEqual(
            failures.classify(log, (), step_failed=True), Kind.INFRA
        )

    def test_only_tests(self):
        self.assertEqual(
            failures.classify('', self._TEST, step_failed=False), Kind.TESTS
        )

    def test_a_failed_step_is_a_build_failure(self):
        self.assertEqual(
            failures.classify('error', self._TEST, step_failed=True),
            Kind.BUILD,
        )


class FailedStepTailTest(unittest.TestCase):
    def test_stops_at_the_first_failed_step(self):
        log = '\n'.join(
            [
                '[Step 1/3] Process exited with code 0',
                '[Step 2/3] error: it broke',
                '[Step 2/3] Process exited with code 1',
                '[Step 3/3] cleanup',
                '[Step 3/3] Process exited with code 0',
            ]
        )
        self.assertEqual(
            failures.failed_step_tail(log, lines=2),
            '[Step 2/3] error: it broke\n[Step 2/3] Process exited with code 1',
        )

    def test_without_a_failed_step_takes_the_end(self):
        self.assertEqual(failures.failed_step_tail('a\nb\nc', lines=2), 'b\nc')


_MOCHA_OUTPUT = '\n'.join(
    [
        '[1:1:1104/051404.248329:INFO:CONSOLE:44] "  started: Page renders", '
        'source: chrome://webui-test/mocha_adapter_simple.js (44)',
        '[1:1:1104/051404.758232:INFO:CONSOLE:58] "   failed: Page saves its '
        'pref',
        'AssertionError: expected false to be true',
        '    at assertTrue (chrome://webui-test/chai_assert.js:14:12)',
        '    at Context.<anonymous> (chrome://webui-test/page_test.js:107:9)", '
        'source: chrome://webui-test/mocha_adapter_simple.js (58)',
        '../../chrome/test/base/web_ui_mocha_browser_test.cc:222: Failure',
    ]
)


class MochaFailuresTest(unittest.TestCase):
    def test_quotes_the_failed_test_and_its_error(self):
        self.assertEqual(
            failures.mocha_failures(_MOCHA_OUTPUT),
            (
                'Page saves its pref\n'
                'AssertionError: expected false to be true\n'
                '    at assertTrue (chrome://webui-test/chai_assert.js:14:12)\n'
                '    at Context.<anonymous> '
                '(chrome://webui-test/page_test.js:107:9)',
            ),
        )

    def test_a_retry_is_quoted_once(self):
        output = f'{_MOCHA_OUTPUT}\n[ RUN      ] Retry\n{_MOCHA_OUTPUT}'
        self.assertEqual(len(failures.mocha_failures(output)), 1)

    def test_a_long_stack_is_cut(self):
        output = '\n'.join(
            [
                '[1:1:0:INFO:CONSOLE:58] "failed: Page t',
                *[f'    at f{i} (x.js:1:1)' for i in range(20)],
            ]
        )
        found = failures.mocha_failures(output)
        self.assertEqual(len(found), 1)
        failure = found[0]
        self.assertEqual(len(failure.splitlines()), 8)

    def test_other_output_has_none(self):
        self.assertEqual(
            failures.mocha_failures('Expected: true\nfailed: x'), ()
        )


class ToMarkdownTest(unittest.TestCase):
    def test_no_failures(self):
        self.assertEqual(failures.to_markdown([]), 'No failed checks.\n')

    def test_lists_builds_tests_and_logs(self):
        report = failures.to_markdown(
            [
                CheckFailures(
                    check='PR browser (android)',
                    url='https://tc/agg',
                    system='teamcity',
                    builds=(
                        FailedBuild(
                            name='Android_Build',
                            url='https://tc/build',
                            summary='Exit code 1',
                            kind=Kind.BUILD,
                            problems=('Process exited with code 1',),
                            log_tail='error: ```boom```',
                        ),
                        FailedBuild(
                            name='Android_Test',
                            url='https://tc/test',
                            summary='Tests failed: 1',
                            kind=Kind.TESTS,
                            tests=(
                                FailedTest(
                                    'Suite.Test',
                                    'FAILURE',
                                    details='Expected 1',
                                    known='failing since build 1',
                                ),
                            ),
                            log_tail='not shown for tests',
                        ),
                    ),
                ),
                CheckFailures(
                    check='other', url='u', system='x', error='no access'
                ),
            ]
        )
        self.assertIn('### Android_Build: build', report)
        self.assertIn('- Problem: Process exited with code 1', report)
        # A log quoting a fence gets a longer one.
        self.assertIn('````\nerror: ```boom```\n````', report)
        self.assertIn(
            '- Test `Suite.Test`: FAILURE (failing since build 1)', report
        )
        self.assertIn('```\nExpected 1\n```', report)
        self.assertNotIn('not shown for tests', report)
        self.assertIn('Could not read its builds: no access', report)

    def test_lists_mocha_failures_and_history(self):
        report = failures.to_markdown(
            [
                CheckFailures(
                    check='PR browser linux (x64)',
                    url='https://tc/agg',
                    system='teamcity',
                    builds=(
                        FailedBuild(
                            name='Linux_Test',
                            url='https://tc/test',
                            summary='Tests failed: 1',
                            kind=Kind.TESTS,
                            tests=(
                                FailedTest(
                                    'Suite.Mocha',
                                    'FAILURE',
                                    mocha=('Page saves\nAssertionError: no',),
                                    history=('`Linux_Test`: failed 1 of 2',),
                                ),
                            ),
                        ),
                    ),
                )
            ]
        )
        self.assertIn(
            '- Test `Suite.Mocha`: FAILURE\n'
            '  - Mocha test failed: `Page saves`\n'
            '  - History: `Linux_Test`: failed 1 of 2\n',
            report,
        )
        self.assertIn(
            '`Suite.Mocha`, Mocha:\n\n```\nPage saves\nAssertionError: no\n```',
            report,
        )


if __name__ == '__main__':
    unittest.main()
