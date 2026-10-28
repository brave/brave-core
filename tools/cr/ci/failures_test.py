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


if __name__ == '__main__':
    unittest.main()
