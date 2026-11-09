#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for the logic in `runtest` (sanitizers, outcome report, Android)."""

from __future__ import annotations

import contextlib
import io
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))

import runtest as m


def _options(**overrides) -> m.Options:
    defaults = {
        'build_dir': Path('out/Component'),
        'test_type': 'brave_unit_tests',
        'builder_name': None,
        'run_python_script': False,
        'xvfb': True,
        'parse_gtest_output': False,
        'enable_asan': False,
        'enable_lsan': False,
        'enable_msan': False,
        'enable_tsan': False,
        'strip_path_prefix': 'build/src/out/Release/../../',
        'test_launcher_summary_output': None,
    }
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
        # `configure_sanitizer_tools` deliberately leaves it to the
        # in-process symbolizer on Windows.
        with mock.patch.object(m.sys, 'platform', 'linux'):
            setup = m.configure_sanitizer_tools(_options(enable_asan=True))
        self.assertTrue(setup.use_symbolization_script)
        self.assertEqual(setup.env['G_SLICE'], 'always-malloc')
        self.assertIn('symbolize=0', setup.env['ASAN_OPTIONS'])
        self.assertIn('LLVM_SYMBOLIZER_PATH', setup.env)

    def test_tsan_uses_online_symbolization_and_no_sandbox(self):
        # See test_asan_uses_offline_symbolization: online symbolization is
        # likewise only wired up off Windows.
        with mock.patch.object(m.sys, 'platform', 'linux'):
            setup = m.configure_sanitizer_tools(_options(enable_tsan=True))
        self.assertFalse(setup.use_symbolization_script)
        self.assertIn('--no-sandbox', setup.extra_args)
        self.assertIn('symbolize=1', setup.env['TSAN_OPTIONS'])

    def test_asan_with_lsan_enables_leak_detection(self):
        setup = m.configure_sanitizer_tools(
            _options(enable_asan=True, enable_lsan=True)
        )
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

    def test_infra_exit_code_without_failed_tests_is_infra_failure(self):
        output = self._report(m.INFRA_FAILURE_EXIT_CODE, _FakeParser())
        self.assertIn('infra failure', output)
        self.assertNotIn('crashed or hung', output)


def _write_wrapper(directory: Path, suite: str, mode: str) -> None:
    # The line `create_test_runner_script.py` generates in `bin/run_<suite>`.
    bin_dir = directory / 'bin'
    bin_dir.mkdir(parents=True, exist_ok=True)
    (bin_dir / f'run_{suite}').write_text(
        "  args = ['@WrappedPath(../../build/android/test_runner.py)', "
        f"'{mode}', '--output-directory', '@WrappedPath(.)']\n",
        encoding='utf-8',
    )


class AndroidTestModeTest(unittest.TestCase):
    def test_reads_the_mode_from_the_wrapper(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            for mode in ('instrumentation', 'gtest', 'junit'):
                _write_wrapper(Path(temp_dir), 'suite', mode)
                self.assertEqual(
                    m.android_test_mode(Path(temp_dir, 'bin', 'run_suite')),
                    mode,
                )

    def test_rejects_a_wrapper_not_running_test_runner(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            wrapper = Path(temp_dir, 'run_suite')
            wrapper.write_text('print("hi")\n', encoding='utf-8')
            with self.assertRaises(ValueError):
                m.android_test_mode(wrapper)


class BuildAndroidTestCommandTest(unittest.TestCase):
    _WRAPPER_PREFIX = [
        'vpython3',
        '../../build/android/test_wrapper/logdog_wrapper.py',
        '--target',
        'brave_java_unit_tests',
    ]

    def test_device_suites_get_the_emulator_mixin(self):
        command = m.build_android_test_command(
            'brave_java_unit_tests', 'instrumentation', 'android_33_x64'
        )
        self.assertEqual(
            command,
            self._WRAPPER_PREFIX
            + [
                '--store-tombstones',
                '--avd-config=../../tools/android/avd/proto/'
                'android_33_x64.textpb',
                '--recover-devices',
            ],
        )

    def test_junit_suites_run_on_the_host(self):
        command = m.build_android_test_command(
            'brave_junit_tests', 'junit', 'android_33_x64'
        )
        self.assertEqual(command[-1], 'brave_junit_tests')
        self.assertFalse(any('avd' in arg for arg in command))
        self.assertNotIn('--store-tombstones', command)


class RunAndroidTestTest(unittest.TestCase):
    def _run(self, temp_dir, mode, kvm_problem=None, **option_overrides):
        _write_wrapper(Path(temp_dir), 'suite', mode)
        options = _options(
            build_dir=Path(temp_dir),
            test_platform='android',
            **option_overrides,
        )
        with (
            mock.patch.object(
                m, 'kvm_access_problem', return_value=kvm_problem
            ),
            mock.patch.object(m, 'run_command', return_value=0) as run_command,
            contextlib.redirect_stdout(io.StringIO()),
            contextlib.redirect_stderr(io.StringIO()),
        ):
            result = m.run_android_test(options, ['suite', '-f', 'A.*'])
        return result, run_command

    def test_runs_the_suite_from_the_build_dir(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            result, run_command = self._run(temp_dir, 'instrumentation')
        self.assertEqual(result, 0)
        command = run_command.call_args.args[0]
        kwargs = run_command.call_args.kwargs
        self.assertEqual(command[2:4], ['--target', 'suite'])
        self.assertEqual(command[-2:], ['-f', 'A.*'])
        self.assertEqual(kwargs['cwd'], Path(temp_dir))
        self.assertEqual(kwargs['env']['CHROMIUM_OUTPUT_DIR'], temp_dir)
        self.assertEqual(
            kwargs['env']['PYTHONPATH'].split(os.pathsep)[0],
            str(m._BRAVE_SCRIPT_DIR),
        )

    def test_passes_the_summary_output(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            summary = Path(temp_dir, 'summary.json')
            _, run_command = self._run(
                temp_dir, 'junit', test_launcher_summary_output=summary
            )
        self.assertEqual(
            run_command.call_args.args[0][-1],
            f'--test-launcher-summary-output={summary}',
        )

    def test_device_suite_without_kvm_is_an_infra_failure(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            result, run_command = self._run(
                temp_dir, 'instrumentation', kvm_problem='no kvm'
            )
        self.assertEqual(result, m.INFRA_FAILURE_EXIT_CODE)
        run_command.assert_not_called()

    def test_junit_suite_does_not_need_kvm(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            result, run_command = self._run(
                temp_dir, 'junit', kvm_problem='no kvm'
            )
        self.assertEqual(result, 0)
        run_command.assert_called_once()


if __name__ == '__main__':
    unittest.main()
