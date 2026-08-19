#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the pure logic in `runtest` (sanitizer setup + outcome report)."""

from __future__ import annotations

import contextlib
import io
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))

import runtest as m


def _options(**overrides) -> m.Options:
    defaults = dict(
        build_dir=Path('out/Component'),
        test_type='brave_unit_tests',
        builder_name=None,
        run_python_script=False,
        xvfb=True,
        parse_gtest_output=False,
        enable_asan=False,
        enable_lsan=False,
        enable_msan=False,
        enable_tsan=False,
        strip_path_prefix='build/src/out/Release/../../',
        test_launcher_summary_output=None,
    )
    defaults.update(overrides)
    return m.Options(**defaults)


class _FakeParser:
    """A minimal `GTestParser` stub for exercising `report_outcome`."""

    def __init__(self, *, failed=(), running=(), errors=(), mem_hashes=()):
        self._failed = list(failed)
        self._running = list(running)
        self._errors = list(errors)
        self._mem = list(mem_hashes)

    def process_line(self, line):
        pass

    def parsing_errors(self):
        return self._errors

    def failed_tests(self):
        return self._failed

    def disabled_tests(self):
        return 0

    def flaky_tests(self):
        return 0

    def memory_tool_report_hashes(self):
        return self._mem

    def running_tests(self):
        return self._running


class ConfigureSanitizerToolsTest(unittest.TestCase):

    def test_no_sanitizer_is_empty(self):
        setup = m.configure_sanitizer_tools(_options())
        self.assertEqual(setup.env, {})
        self.assertEqual(setup.extra_args, [])
        self.assertFalse(setup.use_symbolization_script)

    def test_asan_uses_offline_symbolization(self):
        setup = m.configure_sanitizer_tools(_options(enable_asan=True))
        self.assertTrue(setup.use_symbolization_script)
        self.assertEqual(setup.env['G_SLICE'], 'always-malloc')
        self.assertIn('symbolize=0', setup.env['ASAN_OPTIONS'])
        self.assertIn('LLVM_SYMBOLIZER_PATH', setup.env)

    def test_tsan_uses_online_symbolization_and_no_sandbox(self):
        setup = m.configure_sanitizer_tools(_options(enable_tsan=True))
        self.assertFalse(setup.use_symbolization_script)
        self.assertIn('--no-sandbox', setup.extra_args)
        self.assertIn('symbolize=1', setup.env['TSAN_OPTIONS'])

    def test_asan_with_lsan_enables_leak_detection(self):
        setup = m.configure_sanitizer_tools(
            _options(enable_asan=True, enable_lsan=True))
        self.assertIn('detect_leaks=1', setup.env['ASAN_OPTIONS'])
        self.assertIn('--no-sandbox', setup.extra_args)


class ReportOutcomeTest(unittest.TestCase):

    def _report(self, exit_code, parser):
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer):
            m.report_outcome('brave_unit_tests', exit_code, parser)
        return buffer.getvalue()

    def test_success_is_silent(self):
        output = self._report(0, _FakeParser())
        self.assertNotIn('warnings', output)
        self.assertNotIn('failed', output)
        self.assertNotIn('crashed or hung', output)

    def test_warning_exit_code_reports_warnings(self):
        output = self._report(m.WARNING_EXIT_CODE, _FakeParser())
        self.assertIn('warnings', output)

    def test_zero_exit_with_failures_reports_warnings(self):
        output = self._report(0, _FakeParser(failed=['A.B']))
        self.assertIn('failed 1', output)

    def test_failure_with_failed_tests(self):
        output = self._report(1, _FakeParser(failed=['A.B', 'A.C']))
        self.assertIn('failed 2', output)

    def test_failure_without_failed_tests_is_crash(self):
        output = self._report(1, _FakeParser())
        self.assertIn('crashed or hung', output)

    def test_did_not_complete_when_tests_running(self):
        output = self._report(1, _FakeParser(running=['A.B']))
        self.assertIn('did not complete', output)

    def test_windows_hex_exit_code(self):
        # 0xC0000005 access violation, seen as a large negative int.
        output = self._report(-1073741819, _FakeParser())
        self.assertIn('0xC0000005', output)


if __name__ == '__main__':
    unittest.main()
