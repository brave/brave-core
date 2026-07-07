#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the post_process check library."""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
import post_process as pp


def _steps(status='SUCCESS'):
    return {
        'compile': {
            'name': 'compile',
            'cmd': ['ninja', '-C', 'out'],
            'retcode': 0
        },
        'test': {
            'name': 'test',
            'cmd': ['run_tests'],
            'retcode': 1
        },
        '$result': {
            'name': '$result',
            'status': status
        },
    }


def _run(func, steps, *args):
    """Run a check hook, returning (failure_count, return_value)."""
    check = pp.Checker('ctx')
    result = func(check, steps, *args)
    return len(check.failures), result


class CheckerTest(unittest.TestCase):

    def test_collects_without_raising(self):
        check = pp.Checker('ctx')
        check(True)
        check('hint', False)
        self.assertEqual(len(check.failures), 1)
        self.assertIn('hint', check.failures[0])


class PresenceTest(unittest.TestCase):

    def test_must_run(self):
        self.assertEqual(_run(pp.MustRun, _steps(), 'compile')[0], 0)
        self.assertEqual(_run(pp.MustRun, _steps(), 'missing')[0], 1)

    def test_does_not_run(self):
        self.assertEqual(_run(pp.DoesNotRun, _steps(), 'missing')[0], 0)
        self.assertEqual(_run(pp.DoesNotRun, _steps(), 'compile')[0], 1)

    def test_run_re(self):
        self.assertEqual(_run(pp.MustRunRE, _steps(), r'^comp')[0], 0)
        self.assertEqual(_run(pp.DoesNotRunRE, _steps(), r'^zzz')[0], 0)

    def test_result_is_not_a_run_step(self):
        # The $result pseudo-step must not satisfy MustRun.
        self.assertEqual(_run(pp.MustRun, _steps(), '$result')[0], 1)


class CommandTest(unittest.TestCase):

    def test_command_contains_subsequence(self):
        self.assertEqual(
            _run(pp.StepCommandContains, _steps(), 'compile',
                 ['ninja', 'out'])[0], 0)
        self.assertEqual(
            _run(pp.StepCommandContains, _steps(), 'compile',
                 ['out', 'ninja'])[0], 1)  # wrong order

    def test_command_re(self):
        self.assertEqual(
            _run(pp.StepCommandRE, _steps(), 'compile',
                 [r'ninja', r'-C', r'out'])[0], 0)


class StatusTest(unittest.TestCase):

    def test_step_success_and_failure(self):
        self.assertEqual(_run(pp.StepSuccess, _steps(), 'compile')[0], 0)
        self.assertEqual(_run(pp.StepFailure, _steps(), 'test')[0], 0)
        self.assertEqual(_run(pp.StepSuccess, _steps(), 'test')[0], 1)

    def test_overall_status(self):
        self.assertEqual(_run(pp.StatusSuccess, _steps('SUCCESS'))[0], 0)
        self.assertEqual(_run(pp.StatusFailure, _steps('FAILURE'))[0], 0)
        self.assertEqual(_run(pp.StatusException, _steps('SUCCESS'))[0], 1)


class DropExpectationTest(unittest.TestCase):

    def test_returns_empty_mapping(self):
        _, result = _run(pp.DropExpectation, _steps())
        self.assertEqual(result, {})


if __name__ == '__main__':
    unittest.main()
