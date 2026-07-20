#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the post_process check functions."""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
import check as check_mod
import post_process as pp
from recipe_test_api import PostprocessHookContext

# A non-infra failure carries an inner `failure`; an infra failure does not.
_FAILURE = {'failure': {}, 'humanReason': 'boom'}
_INFRA = {'humanReason': 'boom'}


def _steps(failure=None):
    steps = {
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
            'name': '$result'
        },
    }
    if failure is not None:
        steps['$result']['failure'] = failure
    return steps


def _run(func, steps, *args):
    """Run a check hook, returning (failure_count, return_value)."""
    hook = PostprocessHookContext(func, args, {}, __file__, 0)
    # `check` must be a local: the Checker walks the stack for its own frame.
    check = check_mod.Checker(hook, steps)
    result = func(check, steps, *args)
    return len(check.failed_checks), result


class PresenceTest(unittest.TestCase):

    def test_must_run(self):
        self.assertEqual(_run(pp.MustRun, _steps(), 'compile')[0], 0)
        self.assertEqual(_run(pp.MustRun, _steps(), 'missing')[0], 1)

    def test_must_run_variadic(self):
        # Both present -> 0; one missing -> 1.
        self.assertEqual(_run(pp.MustRun, _steps(), 'compile', 'test')[0], 0)
        self.assertEqual(_run(pp.MustRun, _steps(), 'compile', 'nope')[0], 1)

    def test_does_not_run(self):
        self.assertEqual(_run(pp.DoesNotRun, _steps(), 'missing')[0], 0)
        self.assertEqual(_run(pp.DoesNotRun, _steps(), 'compile')[0], 1)

    def test_run_re(self):
        self.assertEqual(_run(pp.MustRunRE, _steps(), r'^comp')[0], 0)
        self.assertEqual(_run(pp.MustRunRE, _steps(), r'^zzz')[0], 1)
        self.assertEqual(_run(pp.DoesNotRunRE, _steps(), r'^zzz')[0], 0)
        self.assertEqual(_run(pp.DoesNotRunRE, _steps(), r'^comp')[0], 1)

    def test_run_re_bounds(self):
        # at_most caps the number of matches.
        self.assertEqual(_run(pp.MustRunRE, _steps(), r'.', 1, 1)[0], 1)


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
        # A pattern that doesn't fully match its argument fails (full-match
        # semantics: 'ninj' does not match all of 'ninja').
        self.assertEqual(
            _run(pp.StepCommandRE, _steps(), 'compile',
                 [r'ninj', r'-C', r'out'])[0], 1)


class StatusTest(unittest.TestCase):

    def test_step_success_and_failure(self):
        self.assertEqual(_run(pp.StepSuccess, _steps(), 'compile')[0], 0)
        self.assertEqual(_run(pp.StepFailure, _steps(), 'test')[0], 0)
        self.assertEqual(_run(pp.StepSuccess, _steps(), 'test')[0], 1)

    def test_overall_status(self):
        self.assertEqual(_run(pp.StatusSuccess, _steps())[0], 0)
        self.assertEqual(_run(pp.StatusFailure, _steps(_FAILURE))[0], 0)
        self.assertEqual(_run(pp.StatusException, _steps(_INFRA))[0], 0)
        self.assertEqual(_run(pp.StatusAnyFailure, _steps(_FAILURE))[0], 0)

    def test_status_discriminates_infra_from_regular(self):
        # A regular failure is not an exception, and vice versa.
        self.assertEqual(_run(pp.StatusException, _steps(_FAILURE))[0], 1)
        self.assertEqual(_run(pp.StatusFailure, _steps(_INFRA))[0], 1)
        # Success satisfies none of the failure checks.
        self.assertEqual(_run(pp.StatusFailure, _steps())[0], 1)
        self.assertEqual(_run(pp.StatusAnyFailure, _steps())[0], 1)


class DropExpectationTest(unittest.TestCase):

    def test_returns_empty_mapping(self):
        _, result = _run(pp.DropExpectation, _steps())
        self.assertEqual(result, {})


if __name__ == '__main__':
    unittest.main()
