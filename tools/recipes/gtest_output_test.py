#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the gtest output parsers in `gtest_output`."""

from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))

import gtest_output as m


def _feed(parser: m.GTestLogParser, lines: list[str]) -> None:
    for line in lines:
        parser.process_line(line)


class GTestLogParserTest(unittest.TestCase):

    def test_run_then_ok_passes(self):
        parser = m.GTestLogParser()
        _feed(parser, [
            '[ RUN      ] Foo.Bar',
            '[       OK ] Foo.Bar (1 ms)',
        ])
        self.assertEqual(parser.passed_tests(), ['Foo.Bar'])
        self.assertEqual(parser.failed_tests(), [])

    def test_run_then_failed_is_failure(self):
        parser = m.GTestLogParser()
        _feed(parser, [
            '[ RUN      ] Foo.Bar',
            '[  FAILED  ] Foo.Bar (1 ms)',
        ])
        self.assertEqual(parser.failed_tests(), ['Foo.Bar'])
        self.assertEqual(parser.passed_tests(), [])

    def test_unfinished_run_promoted_to_timeout(self):
        # A second RUN with no result for the first marks it as timed out, so it
        # counts as failed.
        parser = m.GTestLogParser()
        _feed(parser, [
            '[ RUN      ] Foo.One',
            '[ RUN      ] Foo.Two',
            '[       OK ] Foo.Two (1 ms)',
        ])
        self.assertIn('Foo.One', parser.failed_tests())
        self.assertEqual(parser.passed_tests(), ['Foo.Two'])

    def test_skipped(self):
        parser = m.GTestLogParser()
        _feed(parser, [
            '[ RUN      ] Foo.Bar',
            '[  SKIPPED ] Foo.Bar (0 ms)',
        ])
        self.assertEqual(parser.skipped_tests(), ['Foo.Bar'])
        self.assertEqual(parser.failed_tests(), [])

    def test_disabled_and_flaky_counts(self):
        parser = m.GTestLogParser()
        _feed(parser, [
            '  YOU HAVE 3 DISABLED TESTS',
            '  YOU HAVE 2 FLAKY TESTS',
        ])
        self.assertEqual(parser.disabled_tests(), 3)
        self.assertEqual(parser.flaky_tests(), 2)

    def test_memory_tool_report_hashes(self):
        parser = m.GTestLogParser()
        _feed(parser, [
            '### BEGIN MEMORY TOOL REPORT (error hash=#ABCDEF#)',
            'some leak detail',
            '### END MEMORY TOOL REPORT (error hash=#ABCDEF#)',
        ])
        self.assertEqual(parser.memory_tool_report_hashes(), ['ABCDEF'])

    def test_failing_tests_block_catches_crash_after_ok(self):
        # A test that reports OK but then appears in the trailing failure list
        # (e.g. it crashed afterwards) is reclassified as failed.
        parser = m.GTestLogParser()
        _feed(parser, [
            '[ RUN      ] Foo.Bar',
            '[       OK ] Foo.Bar (1 ms)',
            'Failing tests:',
            'Foo.Bar',
        ])
        self.assertEqual(parser.failed_tests(), ['Foo.Bar'])

    def test_mixed_line_is_split(self):
        # A gtest directive appearing mid-line (subprocess noise) is still
        # recognized.
        parser = m.GTestLogParser()
        parser.process_line('subprocess noise[ RUN      ] Foo.Bar')
        parser.process_line('[       OK ] Foo.Bar (1 ms)')
        self.assertEqual(parser.passed_tests(), ['Foo.Bar'])

    def test_unparseable_disabled_count_is_some(self):
        parser = m.GTestLogParser()
        # A zero count can't be stored as a positive int, so it falls back.
        parser.process_line('  YOU HAVE 0 DISABLED TESTS')
        self.assertEqual(parser.disabled_tests(), 'some')


class GTestJSONParserTest(unittest.TestCase):

    def _make_data(self):
        return {
            'disabled_tests': ['Foo.Disabled'],
            'per_iteration_data': [{
                'Foo.Pass': [{
                    'status': 'SUCCESS',
                    'output_snippet': ''
                }],
                'Foo.Fail': [{
                    'status': 'FAILURE',
                    'output_snippet': ''
                }],
                'Foo.Flaky': [
                    {
                        'status': 'FAILURE',
                        'output_snippet': ''
                    },
                    {
                        'status': 'SUCCESS',
                        'output_snippet': ''
                    },
                ],
            }],
        }

    def test_process_json_data(self):
        parser = m.GTestJSONParser()
        parser.process_json_data(self._make_data())
        self.assertEqual(parser.passed_tests(), ['Foo.Flaky', 'Foo.Pass'])
        self.assertEqual(parser.failed_tests(), ['Foo.Fail'])
        self.assertEqual(parser.disabled_tests(), 1)
        self.assertEqual(parser.flaky_tests(), 1)

    def test_process_json_file_reads_and_deletes_temp(self):
        parser = m.GTestJSONParser()
        json_path = parser.prepare_json_file(None)
        self.assertTrue(json_path.exists())
        json_path.write_text(json.dumps(self._make_data()), encoding='utf-8')
        parser.process_json_file()
        self.assertEqual(parser.failed_tests(), ['Foo.Fail'])
        # A file we created ourselves is deleted after parsing.
        self.assertFalse(json_path.exists())

    def test_process_json_file_keeps_caller_supplied_path(self):
        with tempfile.TemporaryDirectory() as tmp:
            summary = Path(tmp) / 'summary.json'
            summary.write_text(json.dumps(self._make_data()), encoding='utf-8')
            parser = m.GTestJSONParser()
            parser.prepare_json_file(summary)
            parser.process_json_file()
            self.assertEqual(parser.failed_tests(), ['Foo.Fail'])
            # A caller-supplied file is left in place.
            self.assertTrue(summary.exists())

    def test_empty_summary_is_not_an_error(self):
        parser = m.GTestJSONParser()
        parser.prepare_json_file(None)
        # Left empty: the binary doesn't support JSON output.
        parser.process_json_file()
        self.assertEqual(parser.parsing_errors(), [])

    def test_invalid_summary_records_parsing_errors(self):
        parser = m.GTestJSONParser()
        json_path = parser.prepare_json_file(None)
        json_path.write_text('not json', encoding='utf-8')
        parser.process_json_file()
        self.assertEqual(parser.parsing_errors(), ['not json'])


if __name__ == '__main__':
    unittest.main()
