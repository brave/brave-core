#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Runs a Brave/Chromium test suite as upstream's test bots do.

On Linux, macOS and Windows desktop it runs a gtest binary with the command
mb.py writes into upstream's isolates: `testing/xvfb.py` (windowed suites on
Linux) or `testing/test_env.py` wrapping `./<suite>` from the build dir, with
the suite's arguments from upstream's generated target spec for a reference
builder, and a per-run temp dir as swarming gives each task. Android suites
run through `logdog_wrapper.py` (junit ones through their `bin/run_<suite>`),
with the arguments of the upstream builder running the same emulator. Either
way it optionally parses the gtest summary and reports the outcome.
"""

from __future__ import annotations

import argparse
import ast
from collections.abc import Iterator
import contextlib
from dataclasses import dataclass
import functools
import json
import logging
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))

import gtest_output

# Exit code used by the test launcher to signal a non-fatal warning.
WARNING_EXIT_CODE = 88

# Exit code `test_runner.py` uses for an infra failure (devices, emulator).
INFRA_FAILURE_EXIT_CODE = 87

# The AVD Brave's CI runs Android tests on, from tools/android/avd/proto.
DEFAULT_AVD_CONFIG = 'android_33_google_apis_x64'

# Chromium's `src/`.
SRC_DIR = Path(__file__).resolve().parents[3]

# Where upstream's generated build configs keep each builder's target spec.
_GENERATED_DIR = SRC_DIR / 'infra' / 'config' / 'generated'

# The upstream builder whose suites a desktop run mirrors, by host.
_DESKTOP_REFERENCE_BUILDERS = {
    'linux': 'Linux Tests',
    'win32': 'Win10 Tests x64',
    'darwin-arm64': 'mac15-arm64-rel-tests',
    'darwin-x86_64': 'mac15-x64-rel-tests',
}

# The soft open-file limit upstream's Mac bots run tests with.
MAC_OPEN_FILE_LIMIT = 8000

# Spec arguments only an upstream bot can use: result uploads, and values the
# recipe fills in (`${got_revision}`, `${ISOLATED_OUTDIR}`).
_BOT_ONLY_ARG = re.compile(r'\$\{|^--gs-results-bucket=')

_FILTER_FILE_FLAG = '--test-launcher-filter-file='


@dataclass(frozen=True)
class Options:
    """Parsed command-line options for a runtest invocation."""

    # Absolute path to the build output directory.
    build_dir: Path

    # Test name used in the outcome report, e.g. 'brave_unit_tests'.
    test_type: str

    # Name of the builder running this script (cosmetic; may be None).
    builder_name: str | None

    # Treat the first command token as a python script rather than a binary.
    run_python_script: bool

    # On Linux, run the suite under an X server (`testing/xvfb.py`); None
    # decides by the suite's type in upstream's `gn_isolate_map.pyl`.
    xvfb: bool | None

    # Parse the gtest output and print an outcome summary.
    parse_gtest_output: bool

    # The sanitizers the build has, which `testing/test_env.py` sets up.
    enable_asan: bool
    enable_lsan: bool
    enable_msan: bool
    enable_tsan: bool

    # Path the test launcher writes its JSON summary to (None = auto temp file).
    test_launcher_summary_output: Path | None

    # 'android' to run an Android test suite; None for the host platform.
    test_platform: str | None = None

    # Name of the AVD config Android suites that need a device run on.
    avd_config: str = DEFAULT_AVD_CONFIG


# -- Running the test -------------------------------------------------------


def _kill_if_running(proc: subprocess.Popen) -> None:
    """Kills `proc` so that `Popen.__exit__` does not wait on it forever."""
    if proc.poll() is None:
        proc.kill()


def run_command(
    command: list[str],
    *,
    env: dict[str, str],
    parser: gtest_output.GTestParser | None = None,
    cwd: Path | None = None,
) -> int:
    """Runs `command`, streaming its output and returning its exit code.

    Without `parser` the child inherits stdout and stderr. With one, stdout and
    stderr are merged and each line is echoed and handed to the parser.
    """
    print('\n' + subprocess.list2cmdline(command))
    sys.stdout.flush()
    sys.stderr.flush()

    if not parser:
        return subprocess.run(command, env=env, cwd=cwd, check=False).returncode

    with contextlib.ExitStack() as stack:
        proc = stack.enter_context(
            subprocess.Popen(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                env=env,
                cwd=cwd,
                text=True,
                encoding='utf-8',
                errors='replace',
            )
        )
        stack.callback(_kill_if_running, proc)
        assert proc.stdout is not None
        for line in proc.stdout:
            line = line.rstrip('\n').rstrip('\r')
            print(line)
            parser.process_line(line)
        return proc.wait()


def using_gtest_json(options: Options) -> bool:
    """Returns True when the gtest JSON summary should be parsed."""
    return options.parse_gtest_output and not options.run_python_script


def create_log_processor(options: Options) -> gtest_output.GTestParser | None:
    """Creates the log processor matching the requested parsing mode."""
    if using_gtest_json(options):
        return gtest_output.GTestJSONParser()
    if options.parse_gtest_output:
        return gtest_output.GTestLogParser()
    return None


def report_outcome(
    test_name: str, exit_code: int, parser: gtest_output.GTestParser
) -> None:
    """Prints a human-readable summary of the test outcome."""
    # Always print the raw exit code; it's helpful for debugging, especially on
    # Windows where a "crashed or hung" message may have no other output.
    if exit_code < -100:
        # Windows error codes (e.g. 0xC0000005) are easier to recognize in hex.
        # Add 4 GiB to print them as unsigned.
        print(
            f'exit code (as seen by runtest.py): 0x{exit_code + (1 << 32):08X}'
        )
    else:
        print(f'exit code (as seen by runtest.py): {exit_code}')

    if parser.parsing_errors():
        print('runtest.py encountered the following errors')
        for error in parser.parsing_errors():
            print('  ', error)

    print()
    print(test_name)
    print(f'{parser.disabled_tests()} disabled')
    print(f'{parser.flaky_tests()} flaky')

    success, warnings, failure = range(3)
    status = success
    if exit_code == 0:
        if (
            parser.parsing_errors()
            or parser.failed_tests()
            or parser.memory_tool_report_hashes()
        ):
            status = warnings
    elif exit_code == WARNING_EXIT_CODE:
        status = warnings
    else:
        status = failure

    failed_test_count = len(parser.failed_tests())
    if failed_test_count == 0:
        if status == success:
            return
        if status == warnings:
            print('warnings')
            return

    if parser.running_tests():
        print('did not complete')

    if failed_test_count:
        print(f'failed {failed_test_count}')
    elif exit_code == INFRA_FAILURE_EXIT_CODE:
        print('infra failure')
    else:
        print('crashed or hung')


# -- Upstream's test configuration -------------------------------------------


@functools.cache
def _isolate_map() -> dict:
    path = _GENERATED_DIR / 'testing' / 'gn_isolate_map.pyl'
    return ast.literal_eval(path.read_text(encoding='utf-8'))


def is_windowed_suite(suite: str) -> bool:
    """Whether upstream runs `suite` under an X server on Linux.

    mb.py wraps `windowed_test_launcher` suites in `testing/xvfb.py`, and
    console ones in `testing/test_env.py`. Suites upstream doesn't list, such
    as Brave's own, are taken as windowed.
    """
    entry = _isolate_map().get(suite, {})
    return entry.get('type', 'windowed_test_launcher') != (
        'console_test_launcher'
    )


def _builder_specs(builder: str) -> Iterator[dict]:
    """The generated target specs of the CI builder `builder`."""
    targets = _GENERATED_DIR / 'builders' / 'ci' / builder / 'targets'
    for path in sorted(targets.glob('*.json')):
        yield from json.loads(path.read_text(encoding='utf-8')).values()


def spec_args(test: dict) -> list[str]:
    """A spec entry's arguments, less those only an upstream bot can use."""
    return [
        arg for arg in test.get('args', []) if not _BOT_ONLY_ARG.search(arg)
    ]


def suite_args(suite: str, builder: str) -> list[str]:
    """`suite`'s arguments on the upstream builder `builder`; none if the
    builder doesn't run it."""
    for spec in _builder_specs(builder):
        for test in spec.get('gtest_tests', []):
            if test.get('name') == suite:
                return spec_args(test)
    return []


def desktop_reference_builder() -> str:
    """The upstream builder whose suites a run on this host mirrors."""
    if sys.platform == 'darwin':
        machine = 'arm64' if platform.machine() == 'arm64' else 'x86_64'
        return _DESKTOP_REFERENCE_BUILDERS[f'darwin-{machine}']
    if sys.platform == 'win32':
        return _DESKTOP_REFERENCE_BUILDERS['win32']
    return _DESKTOP_REFERENCE_BUILDERS['linux']


def merge_filter_files(args: list[str]) -> list[str]:
    """`args` with every `--test-launcher-filter-file` merged into one, each
    file once.

    The launcher takes one flag, of `;`-separated files: a later flag would
    replace the spec's filter rather than add to it.
    """
    flags = [arg for arg in args if arg.startswith(_FILTER_FILE_FLAG)]
    files = list(
        dict.fromkeys(
            path
            for flag in flags
            for path in flag.removeprefix(_FILTER_FILE_FLAG).split(';')
            if path
        )
    )
    if not flags:
        return args
    merged = [arg for arg in args if not arg.startswith(_FILTER_FILE_FLAG)]
    return merged + [_FILTER_FILE_FLAG + ';'.join(files)]


# -- Desktop -----------------------------------------------------------------


def _vpython3() -> str:
    """vpython3, which runs upstream's wrappers with Chromium's `.vpython3`
    (psutil and pywin32 for `test_env.py` on Windows)."""
    return shutil.which('vpython3') or 'vpython3'


def build_desktop_command(
    options: Options, suite: str, args: list[str]
) -> list[str]:
    """Builds the command mb.py writes into the suite's upstream isolate.

    Paths are relative to the build dir, which the command runs from.
    """
    windowed = (
        options.xvfb if options.xvfb is not None else is_windowed_suite(suite)
    )
    wrapper = (
        'xvfb.py'
        if sys.platform.startswith('linux') and windowed
        else 'test_env.py'
    )
    executable = suite
    if sys.platform == 'win32' and not executable.endswith('.exe'):
        executable += '.exe'
    command = [
        _vpython3(),
        os.path.relpath(SRC_DIR / 'testing' / wrapper, options.build_dir),
        f'./{executable}',
        '--test-launcher-bot-mode',
        f'--asan={int(options.enable_asan)}',
        f'--lsan={int(options.enable_lsan)}',
        f'--msan={int(options.enable_msan)}',
        f'--tsan={int(options.enable_tsan)}',
        '--cfi-diag=0',
    ]
    return command + merge_filter_files(
        suite_args(suite.removesuffix('.exe'), desktop_reference_builder())
        + args
    )


@contextlib.contextmanager
def task_temp_dir() -> Iterator[dict[str, str]]:
    """A fresh temp dir for the run, as swarming gives every task, removed
    afterwards. Yields the environment variables pointing at it."""
    temp_dir = tempfile.mkdtemp(prefix='runtest_')
    env = {name: temp_dir for name in ('TMPDIR', 'TMP', 'TEMP')}
    if sys.platform == 'darwin':
        env['MAC_CHROMIUM_TMPDIR'] = temp_dir
    try:
        yield env
    finally:
        shutil.rmtree(temp_dir, ignore_errors=True)


def prepare_mac_host() -> None:
    """Gives the run the open-file limit upstream's Mac bots have, and warns
    when it isn't in a GUI session, which windowed suites need."""
    import resource  # pylint: disable=import-outside-toplevel

    soft, hard = resource.getrlimit(resource.RLIMIT_NOFILE)
    wanted = MAC_OPEN_FILE_LIMIT
    if hard != resource.RLIM_INFINITY:
        wanted = min(wanted, hard)
    if soft < wanted:
        resource.setrlimit(resource.RLIMIT_NOFILE, (wanted, hard))
        print(f'Raised the open file limit from {soft} to {wanted}.')

    session = subprocess.run(
        ['launchctl', 'managername'],
        capture_output=True,
        text=True,
        check=False,
    ).stdout.strip()
    if session != 'Aqua':
        print(
            f'Warning: running in a {session or "unknown"} launchd session, '
            'not Aqua: windowed suites need the logged-in GUI session, as '
            "upstream's Mac bots run in.",
            file=sys.stderr,
        )


_USERNS_RESTRICTION = Path(
    '/proc/sys/kernel/apparmor_restrict_unprivileged_userns'
)


def linux_sandbox_problem(args: list[str]) -> str | None:
    """Says why browser processes will die with "No usable sandbox!" here,
    if they will.

    Upstream's bots allow unprivileged user namespaces, which Chromium's
    sandbox uses. Ubuntu 23.10+ restricts them with AppArmor, so a test needs
    `--no-sandbox` or the SUID sandbox (`CHROME_DEVEL_SANDBOX`) there.
    """
    if '--no-sandbox' in args or os.environ.get('CHROME_DEVEL_SANDBOX'):
        return None
    try:
        restricted = _USERNS_RESTRICTION.read_text(encoding='utf-8').strip()
    except OSError:
        return None
    if restricted != '1':
        return None
    return (
        'Warning: this host restricts unprivileged user namespaces '
        '(kernel.apparmor_restrict_unprivileged_userns=1), which '
        "upstream's bots don't: browser processes die with \"No usable "
        'sandbox!". Pass --no-sandbox, or set CHROME_DEVEL_SANDBOX to an '
        'installed SUID sandbox.'
    )


def run_test(options: Options, args: list[str]) -> int:
    """Runs the test on the current desktop platform and returns its code."""
    build_dir = options.build_dir
    suite = args[0]
    if options.run_python_script:
        command = [sys.executable, suite, *args[1:]]
        cwd = None
    else:
        executable = build_dir / suite
        if sys.platform == 'win32' and executable.suffix != '.exe':
            executable = executable.with_suffix('.exe')
        if not executable.exists():
            raise FileNotFoundError(f'Unable to find {executable}')
        command = build_desktop_command(options, suite, args[1:])
        cwd = build_dir

    if sys.platform == 'darwin':
        prepare_mac_host()
    elif sys.platform.startswith('linux') and (
        problem := linux_sandbox_problem(args)
    ):
        print(problem, file=sys.stderr)

    parser = create_log_processor(options)
    json_parser = (
        parser if isinstance(parser, gtest_output.GTestJSONParser) else None
    )
    try:
        if json_parser:
            json_file = json_parser.prepare_json_file(
                options.test_launcher_summary_output
            )
            command.append(f'--test-launcher-summary-output={json_file}')
        elif options.test_launcher_summary_output:
            command.append(
                '--test-launcher-summary-output='
                f'{options.test_launcher_summary_output}'
            )
        with task_temp_dir() as temp_env:
            result = run_command(
                command, env=_build_env(temp_env), parser=parser, cwd=cwd
            )
    finally:
        if json_parser:
            json_parser.process_json_file()

    if options.parse_gtest_output and parser is not None:
        report_outcome(options.test_type, result, parser)

    return result


def _build_env(extra_env: dict[str, str]) -> dict[str, str]:
    """Returns a copy of the current environment with `extra_env` applied."""
    env = os.environ.copy()
    if extra_env:
        print('Additional test environment:')
        for key, value in sorted(extra_env.items()):
            print(f'  {key}={value}')
    env.update(extra_env)
    return env


# -- Android ----------------------------------------------------------------

_BRAVE_SCRIPT_DIR = Path(__file__).resolve().parents[2] / 'script'

# The mode argument the generated `bin/run_<suite>` hands `test_runner.py`.
_TEST_RUNNER_MODE_RE = re.compile(r"test_runner\.py\)', '([a-z_]+)'")

# Upstream's emulator test builders, preferred over the other builders that
# run the same emulator (cronet, webview).
_EMULATOR_BUILDER_RE = re.compile(r'android-\d+-x64-rel')


def android_test_mode(wrapper: Path) -> str:
    """Returns the `test_runner.py` mode `wrapper` runs, e.g. 'junit'."""
    match = _TEST_RUNNER_MODE_RE.search(wrapper.read_text(encoding='utf-8'))
    if not match:
        raise ValueError(f'{wrapper} does not run build/android/test_runner.py')
    return match.group(1)


def _avd_arg(avd_config: str) -> str:
    return f'--avd-config=../../tools/android/avd/proto/{avd_config}.textpb'


@functools.cache
def _emulator_suite_args(avd_config: str) -> dict[str, list[str]]:
    """Each suite's arguments on the upstream builder that runs it on
    `avd_config`, preferring the emulator test builders."""
    avd_arg = _avd_arg(avd_config)
    builders = sorted(
        (path for path in (_GENERATED_DIR / 'builders' / 'ci').iterdir()),
        key=lambda path: (
            not _EMULATOR_BUILDER_RE.fullmatch(path.name),
            path.name,
        ),
    )
    found: dict[str, list[str]] = {}
    for builder in builders:
        for path in sorted((builder / 'targets').glob('*.json')):
            text = path.read_text(encoding='utf-8')
            if avd_arg not in text:
                continue
            for spec in json.loads(text).values():
                for test in spec.get('gtest_tests', []):
                    name = test.get('name')
                    if name not in found and avd_arg in test.get('args', []):
                        found[name] = spec_args(test)
    return found


def build_android_test_command(
    suite: str, mode: str, avd_config: str
) -> list[str]:
    """Builds the command upstream's bots run an Android suite's shard with.

    Device suites run mb.py's isolate command, without the logdog butler,
    with the arguments of the upstream builder that runs the suite on the
    same emulator; suites upstream doesn't run there (Brave's) get the
    emulator mixin's. Junit suites are isolated scripts upstream, run through
    their `bin/run_<suite>`. Paths are relative to the build dir, which the
    command runs from.
    """
    if mode == 'junit':
        return [f'bin/run_{suite}']
    command = [
        'vpython3',
        '../../build/android/test_wrapper/logdog_wrapper.py',
        '--target',
        suite,
        '--store-tombstones',
    ]
    upstream = _emulator_suite_args(avd_config).get(suite)
    if upstream is not None:
        return command + upstream
    return command + [_avd_arg(avd_config), '--recover-devices']


def kvm_access_problem() -> str | None:
    """Says why the emulator can't use KVM here, if it can't.

    Without KVM the emulator doesn't fail, it times out booting.
    """
    kvm = Path('/dev/kvm')
    if not kvm.exists():
        return '/dev/kvm does not exist: the Android emulator needs KVM.'
    if not os.access(kvm, os.R_OK | os.W_OK):
        return (
            'No read/write access to /dev/kvm, which the Android emulator '
            'needs: the user must be in the kvm group (`id -nG`).'
        )
    return None


def _android_env(build_dir: Path) -> dict[str, str]:
    """Returns the environment Android suites run with."""
    # Brave's patches to the Android test scripts load their chromium_src
    # overrides with `brave_chromium_utils`.
    python_path = [str(_BRAVE_SCRIPT_DIR)]
    if os.environ.get('PYTHONPATH'):
        python_path.append(os.environ['PYTHONPATH'])
    return _build_env(
        {
            'CHROMIUM_OUTPUT_DIR': str(build_dir),
            'PYTHONPATH': os.pathsep.join(python_path),
        }
    )


def run_android_test(options: Options, args: list[str]) -> int:
    """Runs the Android suite `args[0]` from the host and returns its code."""
    build_dir = options.build_dir
    suite = args[0]
    wrapper = build_dir / 'bin' / f'run_{suite}'
    if not wrapper.exists():
        raise FileNotFoundError(f'Unable to find {wrapper}')

    mode = android_test_mode(wrapper)
    if mode != 'junit' and (problem := kvm_access_problem()):
        print(problem, file=sys.stderr)
        return INFRA_FAILURE_EXIT_CODE

    command = merge_filter_files(
        build_android_test_command(suite, mode, options.avd_config) + args[1:]
    )

    json_parser = (
        gtest_output.GTestJSONParser() if options.parse_gtest_output else None
    )
    json_file = options.test_launcher_summary_output
    if json_parser:
        json_file = json_parser.prepare_json_file(json_file)
    if json_file:
        command.append(f'--test-launcher-summary-output={json_file}')

    try:
        with task_temp_dir() as temp_env:
            env = _android_env(build_dir)
            env.update(temp_env)
            result = run_command(command, env=env, cwd=build_dir)
    finally:
        if json_parser:
            json_parser.process_json_file()

    if json_parser:
        report_outcome(options.test_type or suite, result, json_parser)
    return result


# -- CLI --------------------------------------------------------------------


def main() -> int:
    """Entry point: parses args, sets up the environment and runs the test."""
    parser = argparse.ArgumentParser(
        description='Runs a test suite as upstream\'s test bots do.'
    )
    parser.add_argument(
        '--build-dir', required=True, type=Path, help='Path to the build dir.'
    )
    parser.add_argument(
        '--run-python-script',
        action='store_true',
        help='Treat the first argument as a python script.',
    )
    parser.add_argument(
        '--xvfb',
        dest='xvfb',
        action='store_true',
        default=None,
        help='On Linux, run under an X server (testing/xvfb.py); by default '
        "when upstream's gn_isolate_map.pyl says the suite is windowed.",
    )
    parser.add_argument(
        '--no-xvfb',
        dest='xvfb',
        action='store_false',
        help='On Linux, run without an X server (testing/test_env.py).',
    )
    parser.add_argument(
        '--builder-group',
        default=None,
        help='The group of the builder (accepted for '
        'compatibility; currently unused).',
    )
    parser.add_argument(
        '--builder-name',
        default=None,
        help='The name of the builder running this script.',
    )
    parser.add_argument(
        '--test-type',
        default='',
        help="The test name, e.g. 'brave_unit_tests'.",
    )
    parser.add_argument(
        '--parse-gtest-output',
        action='store_true',
        help='Parse the gtest output and report the outcome.',
    )
    parser.add_argument(
        '--enable-asan', action='store_true', help='Enable AddressSanitizer.'
    )
    parser.add_argument(
        '--enable-lsan', action='store_true', help='Enable LeakSanitizer.'
    )
    parser.add_argument(
        '--enable-msan', action='store_true', help='Enable MemorySanitizer.'
    )
    parser.add_argument(
        '--enable-tsan', action='store_true', help='Enable ThreadSanitizer.'
    )
    parser.add_argument(
        '--test-launcher-summary-output',
        type=Path,
        default=None,
        help='Path to the test launcher summary JSON file.',
    )
    parser.add_argument(
        '--test-platform',
        choices=['android'],
        default=None,
        help='Run an Android test suite (named as its bin/run_<suite>) '
        'instead of a host binary.',
    )
    parser.add_argument(
        '--avd-config',
        default=DEFAULT_AVD_CONFIG,
        help='The tools/android/avd/proto config (without .textpb) the '
        'emulator for Android suites runs (default: %(default)s).',
    )
    parser.add_argument(
        'test_command',
        nargs=argparse.REMAINDER,
        help='The test executable (or python script) and its arguments.',
    )

    parsed = parser.parse_args()
    if not parsed.test_command:
        parser.error('a test command is required')

    options = Options(
        build_dir=Path(parsed.build_dir).resolve(),
        test_type=parsed.test_type,
        builder_name=parsed.builder_name,
        run_python_script=parsed.run_python_script,
        xvfb=parsed.xvfb,
        parse_gtest_output=parsed.parse_gtest_output,
        enable_asan=parsed.enable_asan,
        enable_lsan=parsed.enable_lsan,
        enable_msan=parsed.enable_msan,
        enable_tsan=parsed.enable_tsan,
        test_launcher_summary_output=(
            parsed.test_launcher_summary_output.resolve()
            if parsed.test_launcher_summary_output
            else None
        ),
        test_platform=parsed.test_platform,
        avd_config=parsed.avd_config,
    )
    args = list(parsed.test_command)

    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s %(filename)s:%(lineno)-3d %(levelname)s %(message)s',
        datefmt='%y%m%d %H:%M:%S',
    )

    print(f'[Running on builder: "{options.builder_name}"]')

    if options.test_platform == 'android':
        if not sys.platform.startswith('linux'):
            print('Android suites run from a Linux host only.', file=sys.stderr)
            return 1
        return run_android_test(options, args)

    if sys.platform not in ('win32', 'cygwin') and not sys.platform.startswith(
        ('linux', 'darwin')
    ):
        print(f'Unknown sys.platform value {sys.platform!r}', file=sys.stderr)
        return 1

    return run_test(options, args)


if __name__ == '__main__':
    sys.exit(main())
