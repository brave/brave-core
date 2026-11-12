#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `runtest`: upstream's commands, the outcome report, Android."""

from __future__ import annotations

import contextlib
import io
import json
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
        'build_dir': m.SRC_DIR / 'out' / 'Component',
        'test_type': 'brave_unit_tests',
        'builder_name': None,
        'run_python_script': False,
        'xvfb': None,
        'parse_gtest_output': False,
        'enable_asan': False,
        'enable_lsan': False,
        'enable_msan': False,
        'enable_tsan': False,
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


class _GeneratedConfigTest(unittest.TestCase):
    """Runs against a fake `infra/config/generated` tree."""

    def setUp(self):
        stack = contextlib.ExitStack()
        self.addCleanup(stack.close)
        self.generated = Path(
            stack.enter_context(tempfile.TemporaryDirectory())
        )
        stack.enter_context(
            mock.patch.object(m, '_GENERATED_DIR', self.generated)
        )
        for cached in (m._isolate_map, m._emulator_suite_args):
            cached.cache_clear()
            self.addCleanup(cached.cache_clear)
        testing = self.generated / 'testing'
        testing.mkdir()
        (testing / 'gn_isolate_map.pyl').write_text(
            repr(
                {
                    'base_unittests': {'type': 'console_test_launcher'},
                    'browser_tests': {'type': 'windowed_test_launcher'},
                }
            ),
            encoding='utf-8',
        )

    def write_spec(self, builder: str, tests: list[dict]) -> None:
        targets = self.generated / 'builders' / 'ci' / builder / 'targets'
        targets.mkdir(parents=True)
        (targets / 'chromium.json').write_text(
            json.dumps({builder: {'gtest_tests': tests}}), encoding='utf-8'
        )


class DesktopCommandTest(_GeneratedConfigTest):
    def _command(
        self, suite, platform='linux', args=('--gtest_filter=A.*',), **options
    ):
        with mock.patch.object(m.sys, 'platform', platform):
            return m.build_desktop_command(
                _options(**options), suite, list(args)
            )

    def test_windowed_suites_run_under_xvfb_on_linux(self):
        command = self._command('browser_tests')
        self.assertEqual(command[1], '../../testing/xvfb.py')
        self.assertEqual(
            command[2:],
            [
                './browser_tests',
                '--test-launcher-bot-mode',
                '--asan=0',
                '--lsan=0',
                '--msan=0',
                '--tsan=0',
                '--cfi-diag=0',
                '--gtest_filter=A.*',
            ],
        )

    def test_console_suites_run_under_test_env(self):
        self.assertEqual(
            self._command('base_unittests')[1], '../../testing/test_env.py'
        )

    def test_suites_upstream_does_not_list_are_windowed(self):
        self.assertEqual(
            self._command('brave_unit_tests')[1], '../../testing/xvfb.py'
        )

    def test_xvfb_flags_override_the_suite_type(self):
        self.assertEqual(
            self._command('browser_tests', xvfb=False)[1],
            '../../testing/test_env.py',
        )
        self.assertEqual(
            self._command('base_unittests', xvfb=True)[1],
            '../../testing/xvfb.py',
        )

    def test_mac_and_windows_run_under_test_env(self):
        self.assertEqual(
            self._command('browser_tests', 'darwin')[1],
            '../../testing/test_env.py',
        )
        command = self._command('browser_tests', 'win32')
        self.assertEqual(command[1], '../../testing/test_env.py')
        self.assertEqual(command[2], './browser_tests.exe')

    def test_sanitizers_are_flags_for_test_env(self):
        command = self._command('browser_tests', enable_asan=True)
        self.assertIn('--asan=1', command)
        self.assertIn('--lsan=0', command)

    def test_the_reference_builder_s_suite_args_come_first(self):
        self.write_spec(
            'Linux Tests',
            [
                {
                    'name': 'browser_tests',
                    'args': [
                        '--test-launcher-filter-file=../../up.filter',
                        '--git-revision=${got_revision}',
                    ],
                }
            ],
        )
        command = self._command(
            'browser_tests',
            args=['--test-launcher-filter-file=../../brave.filter'],
        )
        self.assertEqual(
            command[9:],
            ['--test-launcher-filter-file=../../up.filter;../../brave.filter'],
        )


class MergeFilterFilesTest(unittest.TestCase):
    def test_one_flag_is_left_alone(self):
        args = ['--a', '--test-launcher-filter-file=x']
        self.assertEqual(m.merge_filter_files(args), args)

    def test_each_file_is_kept_once(self):
        self.assertEqual(
            m.merge_filter_files(
                [
                    '--test-launcher-filter-file=x;y',
                    '--test-launcher-filter-file=y',
                    '--test-launcher-filter-file=x;z',
                ]
            ),
            ['--test-launcher-filter-file=x;y;z'],
        )

    def test_several_flags_become_one(self):
        self.assertEqual(
            m.merge_filter_files(
                [
                    '--test-launcher-filter-file=x',
                    '--a',
                    '--test-launcher-filter-file=y;z',
                ]
            ),
            ['--a', '--test-launcher-filter-file=x;y;z'],
        )


class TaskTempDirTest(unittest.TestCase):
    def test_is_fresh_set_everywhere_and_removed(self):
        with mock.patch.object(m.sys, 'platform', 'darwin'):
            with m.task_temp_dir() as env:
                temp_dir = Path(env['TMPDIR'])
                self.assertTrue(temp_dir.is_dir())
                self.assertEqual(list(temp_dir.iterdir()), [])
                self.assertEqual({env[name] for name in env}, {str(temp_dir)})
                self.assertIn('MAC_CHROMIUM_TMPDIR', env)
        self.assertFalse(temp_dir.exists())


class LinuxSandboxProblemTest(unittest.TestCase):
    def _problem(self, restricted, args=(), env=None):
        with tempfile.TemporaryDirectory() as temp_dir:
            setting = Path(temp_dir, 'restrict')
            if restricted is not None:
                setting.write_text(f'{restricted}\n', encoding='utf-8')
            with (
                mock.patch.object(m, '_USERNS_RESTRICTION', setting),
                mock.patch.dict(m.os.environ, env or {}, clear=False),
            ):
                if not env:
                    m.os.environ.pop('CHROME_DEVEL_SANDBOX', None)
                return m.linux_sandbox_problem(list(args))

    def test_restricted_user_namespaces_are_reported(self):
        self.assertIn('--no-sandbox', self._problem(1))

    def test_unrestricted_or_unknown_hosts_are_fine(self):
        self.assertIsNone(self._problem(0))
        self.assertIsNone(self._problem(None))

    def test_no_sandbox_or_a_suid_sandbox_avoids_it(self):
        self.assertIsNone(self._problem(1, args=['--no-sandbox']))
        self.assertIsNone(
            self._problem(1, env={'CHROME_DEVEL_SANDBOX': '/opt/sandbox'})
        )


class RunTestTest(_GeneratedConfigTest):
    def test_runs_from_the_build_dir_with_a_task_temp_dir(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            build_dir = Path(temp_dir)
            (build_dir / 'browser_tests').touch()
            summary = build_dir / 'summary.json'
            with (
                mock.patch.object(m.sys, 'platform', 'linux'),
                mock.patch.object(m, 'run_command', return_value=0) as run,
                contextlib.redirect_stdout(io.StringIO()),
            ):
                result = m.run_test(
                    _options(
                        build_dir=build_dir,
                        test_launcher_summary_output=summary,
                    ),
                    ['browser_tests', '--gtest_filter=A.*'],
                )
        self.assertEqual(result, 0)
        command = run.call_args.args[0]
        kwargs = run.call_args.kwargs
        self.assertEqual(kwargs['cwd'], build_dir)
        self.assertEqual(
            command[-1], f'--test-launcher-summary-output={summary}'
        )
        self.assertIn('--gtest_filter=A.*', command)
        self.assertTrue(
            kwargs['env']['TMPDIR'].startswith(tempfile.gettempdir())
        )

    def test_missing_binary_is_an_error(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            with self.assertRaises(FileNotFoundError):
                m.run_test(_options(build_dir=Path(temp_dir)), ['nope'])


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


_AVD = '--avd-config=../../tools/android/avd/proto/android_33_x64.textpb'


class BuildAndroidTestCommandTest(_GeneratedConfigTest):
    _WRAPPER_PREFIX = [
        'vpython3',
        '../../build/android/test_wrapper/logdog_wrapper.py',
        '--target',
    ]

    def test_upstream_suites_take_the_emulator_builder_s_args(self):
        emulator_args = [_AVD, '--test-launcher-filter-file=emu.filter']
        self.write_spec(
            'android-cronet-x64-dbg',
            [{'name': 'content_browsertests', 'args': [_AVD]}],
        )
        self.write_spec(
            'android-13-x64-rel',
            [
                {
                    'name': 'content_browsertests',
                    'args': emulator_args
                    + [
                        '--gs-results-bucket=chromium-result-details',
                        '--git-revision=${got_revision}',
                    ],
                }
            ],
        )
        command = m.build_android_test_command(
            'content_browsertests', 'gtest', 'android_33_x64'
        )
        self.assertEqual(
            command,
            self._WRAPPER_PREFIX
            + ['content_browsertests', '--store-tombstones']
            + emulator_args,
        )

    def test_suites_upstream_does_not_run_get_the_emulator_mixin(self):
        (self.generated / 'builders' / 'ci').mkdir(parents=True)
        command = m.build_android_test_command(
            'brave_java_unit_tests', 'instrumentation', 'android_33_x64'
        )
        self.assertEqual(
            command,
            self._WRAPPER_PREFIX
            + [
                'brave_java_unit_tests',
                '--store-tombstones',
                _AVD,
                '--recover-devices',
            ],
        )

    def test_junit_suites_run_their_wrapper_on_the_host(self):
        self.assertEqual(
            m.build_android_test_command(
                'brave_junit_tests', 'junit', 'android_33_x64'
            ),
            ['bin/run_brave_junit_tests'],
        )


class RunAndroidTestTest(_GeneratedConfigTest):
    def _run(self, temp_dir, mode, kvm_problem=None, **option_overrides):
        (self.generated / 'builders' / 'ci').mkdir(parents=True, exist_ok=True)
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
        self.assertIn('TMPDIR', kwargs['env'])

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
