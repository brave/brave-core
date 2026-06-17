#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Integration test for Brave Teamcity test launcher reporter output.

Runs brave_unit_tests with disabled integration gtests and verifies ##teamcity
service message output. Intended to run from the build output directory, e.g.:

  cd out/Default
  python3 teamcity_reporter_integration_test.py
"""

import difflib
import os
import re
import subprocess
import sys
import unittest


class TeamcityReporterIntegrationTest(unittest.TestCase):
    test_binary = None

    base_test_args = [
        '--test-launcher-bot-mode',
        '--test-launcher-enable-teamcity-reporter',
        '--gtest_filter=DISABLED_TeamcityReporterIntegration*',
        '--gtest_also_run_disabled_tests',
        # Enable retry limit explicitly, because it's set to 0 when
        # --gtest_filter is passed.
        '--test-launcher-retry-limit=1',
    ]

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.test_binary = os.path.join(os.getcwd(), 'brave_unit_tests')
        if sys.platform == 'win32':
            cls.test_binary += '.exe'

        print(f'test binary: {cls.test_binary}')

    def test_teamcity_reporter_output(self):
        args = self.base_test_args

        expected_lines = [
            "##teamcity[testSuiteStarted name='brave_unit_tests']",
            "##teamcity[testRetrySupport enabled='true']",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Success'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Success'",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
            "##teamcity[testFailed name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
            "##teamcity[testFailed name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
            "##teamcity[testIgnored name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
            "##teamcity[testFailed name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
            "##teamcity[testFailed name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
            "##teamcity[testSuiteFinished name='brave_unit_tests']",
        ]

        self._assert_teamcity_reporter_output(args, expected_lines)

    def test_ignore_preliminary_failures_teamcity_reporter_output(self):
        args = self.base_test_args + [
            '--test-launcher-teamcity-reporter-ignore-preliminary-failures',
        ]

        expected_lines = [
            "##teamcity[testSuiteStarted name='brave_unit_tests']",
            "##teamcity[testRetrySupport enabled='true']",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Success'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Success'",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
            "##teamcity[testIgnored name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
            "##teamcity[testIgnored name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
            "##teamcity[testIgnored name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
            "##teamcity[testFailed name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Failure'",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
            "##teamcity[testFailed name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.CheckFailure'",
            "##teamcity[testStarted name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
            "##teamcity[testFinished name='DISABLED_TeamcityReporterIntegrationTest.Skipped'",
            "##teamcity[testSuiteFinished name='brave_unit_tests']",
        ]

        self._assert_teamcity_reporter_output(args, expected_lines)

    def _assert_teamcity_reporter_output(self, test_args,
                                         expected_teamcity_lines):
        self.assertTrue(
            os.path.exists(self.test_binary),
            f'Missing test runner executable {self.test_binary}',
        )

        command = [self.test_binary, *test_args]
        print(f'command: {" ".join(command)}')

        result = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )

        output_lines = result.stdout.split('\n')
        output_teamcity_lines = [
            line for line in output_lines if line.startswith('##teamcity')
        ]
        diff = self._generate_diff(expected_teamcity_lines,
                                   output_teamcity_lines)

        if diff:
            message_lines = [
                'Output ## teamcity lines do not match expected lines '
                '(## teamcity was replaced with %%teamcity):',
                '',
                diff,
                '',
                'Full test output:',
                *output_lines,
            ]
            self.fail('\n'.join(message_lines).replace('##teamcity',
                                                       '%%teamcity'))

    @staticmethod
    def _generate_diff(expected_teamcity_lines, output_teamcity_lines):

        class PrefixString(str):

            def __eq__(self, other):
                # Make sure we are comparing against another string
                if isinstance(other, str):
                    # Check if one is a prefix of the other
                    return self.startswith(other) or other.startswith(self)
                return NotImplemented

            def __hash__(self):
                # Force a hash collision so difflib actually calls __eq__
                return 1

        expected_teamcity_lines = [
            PrefixString(line) for line in expected_teamcity_lines
        ]
        output_teamcity_lines = [
            PrefixString(line) for line in output_teamcity_lines
        ]

        return '\n'.join(
            difflib.unified_diff(
                expected_teamcity_lines,
                output_teamcity_lines,
                fromfile='expected',
                tofile='actual',
                lineterm='',
            ))


if __name__ == '__main__':
    unittest.main()
