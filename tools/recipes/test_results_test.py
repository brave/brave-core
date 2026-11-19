#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the gtest result model of `test_results`."""

from __future__ import annotations

import base64
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))

import test_results as m
from test_results import Outcome


def _b64(text: str) -> str:
    return base64.b64encode(text.encode()).decode()


def _run(status: str, *, snippet: str = '', ms: int = 1000) -> dict:
    return {
        'status': status,
        'elapsed_time_ms': ms,
        'output_snippet_base64': _b64(snippet),
        'result_parts': [],
    }


def _summary(*iterations: dict) -> dict:
    return {'per_iteration_data': list(iterations)}


class ReadSummaryTest(unittest.TestCase):
    def test_each_test_s_runs_across_iterations_by_name(self):
        results = m.read_summary(
            _summary(
                {'B.Flaky': [_run('FAILURE')], 'A.Passes': [_run('SUCCESS')]},
                {'B.Flaky': [_run('SUCCESS')]},
            )
        )
        self.assertEqual(
            [result.name for result in results], ['A.Passes', 'B.Flaky']
        )
        flaky = results[1]
        self.assertEqual(len(flaky.runs), 2)
        self.assertEqual(flaky.outcome, Outcome.PASSED)
        self.assertTrue(flaky.flaky)
        self.assertFalse(results[0].flaky)

    def test_a_test_that_failed_on_every_run_is_not_flaky(self):
        (result,) = m.read_summary(
            _summary({'A.B': [_run('CRASH'), _run('FAILURE')]})
        )
        self.assertEqual(result.outcome, Outcome.FAILED)
        self.assertFalse(result.flaky)
        self.assertEqual(len(result.failed_runs), 2)


class TrimOutputTest(unittest.TestCase):
    def test_keeps_short_output_whole(self):
        self.assertEqual(m.trim_output('a\nb'), 'a\nb')

    def test_keeps_the_head_and_tail_of_long_output(self):
        lines = [f'line {n}' for n in range(100)]
        trimmed = m.trim_output('\n'.join(lines)).splitlines()
        self.assertEqual(
            trimmed[: m.OUTPUT_HEAD_LINES], lines[: m.OUTPUT_HEAD_LINES]
        )
        self.assertEqual(trimmed[m.OUTPUT_HEAD_LINES], '[... 50 lines cut ...]')
        self.assertEqual(
            trimmed[-m.OUTPUT_TAIL_LINES :], lines[-m.OUTPUT_TAIL_LINES :]
        )

    def test_cuts_the_middle_of_a_long_line(self):
        line = 'a' * 1500 + 'b' * 1500
        trimmed = m.trim_output(line)
        self.assertTrue(trimmed.startswith('a' * 1000))
        self.assertTrue(trimmed.endswith('b' * 1000))
        self.assertIn('[... 1000 characters cut ...]', trimmed)


class SuiteOutcomeTest(unittest.TestCase):
    def _suite_outcome(self) -> m.SuiteOutcome:
        return m.SuiteOutcome.of(
            m.read_summary(
                _summary(
                    {
                        'A.Passes': [_run('SUCCESS')],
                        'A.Skips': [_run('SKIPPED')],
                        'A.Fails': [_run('FAILURE', snippet='it broke')],
                        'A.Flaky': [
                            _run('CRASH', snippet='crashed'),
                            _run('SUCCESS'),
                        ],
                        'A.Lost': [_run('NOTRUN')],
                    }
                )
            )
        )

    def test_counts_each_outcome(self):
        self.assertEqual(
            self._suite_outcome().counts,
            {'passed': 2, 'failed': 1, 'skipped': 1, 'errored': 1},
        )

    def test_lists_the_tests_that_failed_flaky_ones_included(self):
        failures = {
            failure.name: failure for failure in self._suite_outcome().failures
        }
        self.assertEqual(sorted(failures), ['A.Fails', 'A.Flaky', 'A.Lost'])
        self.assertFalse(failures['A.Fails'].flaky)
        self.assertEqual(failures['A.Fails'].output, 'it broke')
        self.assertTrue(failures['A.Flaky'].flaky)
        self.assertEqual(failures['A.Flaky'].status, 'CRASH')
        self.assertEqual(failures['A.Flaky'].output, 'crashed')

    def test_round_trips_through_a_file(self):
        outcome = self._suite_outcome()
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'out' / 'results.json'
            outcome.write(path)
            self.assertEqual(m.SuiteOutcome.read(path), outcome)


class SnippetFailureMessageTest(unittest.TestCase):
    def test_check_failures(self):
        self.assertEqual(
            m.snippet_failure_message(
                '[1:2:FATAL:foo.cc(42)] Check failed: x. \n'
            ),
            'foo.cc(42): Check failed: x.',
        )

    def test_segfaults(self):
        self.assertEqual(
            m.snippet_failure_message(
                'Received signal 11 SEGV_MAPERR 000000000000\n'
                '#0 0x7f0 base::debug::StackTrace::StackTrace()\n'
                '#1 0x7f1 libc.so.6+0x42\n'
                '#2 0x7f2 brave::Foo::Bar()\n'
            ),
            'brave::Foo::Bar: segfault',
        )

    def test_expectations(self):
        self.assertEqual(
            m.snippet_failure_message(
                '../../brave/foo.cc:7: Failure\nValue of: x\nStack trace:\n'
            ),
            'foo.cc(7): Value of: x',
        )

    def test_nothing_known(self):
        self.assertEqual(m.snippet_failure_message('it went away'), '')


if __name__ == '__main__':
    unittest.main()
