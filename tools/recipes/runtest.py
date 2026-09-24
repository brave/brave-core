#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Runs a Brave/Chromium test executable and processes its output.

This is a harness to run gtests on Linux, macOS and Windows desktop only (no
iOS/Android/run-isolated yet). It sets up the per-platform test environment
(sanitizers, xvfb + DBus on Linux, proxy/sandbox/library paths), runs the test,
optionally parses the gtest output, and reports the outcome.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass, field
import logging
import os
from pathlib import Path
import platform
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))

import gtest_output

# Exit code used by the test launcher to signal a non-fatal warning.
WARNING_EXIT_CODE = 88

# Path to the SUID sandbox binary, which must be installed on all Linux bots.
CHROME_SANDBOX_PATH = '/opt/chromium/chrome_sandbox'

# X display index used for the virtual X server on Linux.
_XVFB_DISPLAY_INDEX = 9


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

    # Start a virtual X server on Linux.
    xvfb: bool

    # Parse the gtest output and print an outcome summary.
    parse_gtest_output: bool

    # Enable AddressSanitizer.
    enable_asan: bool

    # Enable LeakSanitizer.
    enable_lsan: bool

    # Enable MemorySanitizer.
    enable_msan: bool

    # Enable ThreadSanitizer.
    enable_tsan: bool

    # Stack-trace path prefix stripped by the sanitizer symbolizer.
    strip_path_prefix: str

    # Path the test launcher writes its JSON summary to (None = auto temp file).
    test_launcher_summary_output: Path | None


@dataclass(frozen=True)
class SanitizerSetup:
    """Environment and argument overrides derived from the sanitizer flags."""

    # Extra environment variables to merge into the test environment.
    env: dict[str, str] = field(default_factory=dict)

    # Extra arguments to append to the test command (e.g. --no-sandbox).
    extra_args: list[str] = field(default_factory=list)

    # Whether test output must be piped through the offline symbolizer script.
    use_symbolization_script: bool = False


def get_temp_count() -> int:
    """Returns the number of entries inside the system temporary directory."""
    return len(list(Path(tempfile.gettempdir()).iterdir()))


# -- DBus (Linux) -----------------------------------------------------------


def launch_dbus() -> bool:
    """Launches DBus to work around a GLib fork/exec bug on Linux bots.

    GLib performs operations that aren't async-signal-safe (notably memory
    allocation) between fork and exec when spawning subprocesses, which can
    wedge Chrome's browser/utility processes. Users don't hit this because they
    have an active desktop session with `DBUS_SESSION_BUS_ADDRESS` set, but bots
    may hit this. See crbug.com/309093.

    Returns True if it actually spawned DBus.
    """
    if (platform.uname()[0].lower() == 'linux'
            and 'DBUS_SESSION_BUS_ADDRESS' not in os.environ):
        try:
            print('DBUS_SESSION_BUS_ADDRESS env var not found, starting '
                  'dbus-launch')
            dbus_output = subprocess.check_output(
                ['dbus-launch'], universal_newlines=True).split('\n')
            for line in dbus_output:
                match = re.match(r'([^=]+)\=(.+)', line)
                if match:
                    os.environ[match.group(1)] = match.group(2)
                    print(f'  setting {match.group(1)} to {match.group(2)}')
            return True
        except (subprocess.CalledProcessError, OSError) as error:
            print(f'Exception while running dbus-launch: {error}')
    return False


def shutdown_dbus() -> None:
    """Kills the DBus daemon previously launched by `launch_dbus`.

    Passing `--exit-with-session` to dbus-launch doesn't reliably shut the
    daemon down, so kill it manually using the PID it reported at launch.
    """
    if 'DBUS_SESSION_BUS_PID' in os.environ:
        dbus_pid = os.environ['DBUS_SESSION_BUS_PID']
        try:
            os.kill(int(dbus_pid), signal.SIGTERM)
            print(f'  killed dbus-daemon with PID {dbus_pid}')
        except OSError as error:
            print(f'  error killing dbus-daemon with PID {dbus_pid}: {error}')
    # Some bots re-invoke runtest.py in a way that leaves this variable set from
    # run to run, so clear it too.
    if 'DBUS_SESSION_BUS_ADDRESS' in os.environ:
        del os.environ['DBUS_SESSION_BUS_ADDRESS']
        print('  cleared DBUS_SESSION_BUS_ADDRESS environment variable')


# -- Temporary file cleanup -------------------------------------------------


def _log_and_remove_files(temp_dir: Path, regex_pattern: str) -> None:
    """Removes entries in `temp_dir` matching `regex_pattern`, printing each."""
    regex = re.compile(regex_pattern)
    if not temp_dir.is_dir():
        return
    for item in temp_dir.iterdir():
        if not regex.search(item.name):
            continue
        print(f'Removing leaked temp item: {item}')
        try:
            if item.is_symlink() or item.is_file():
                item.unlink()
            elif item.is_dir():
                _remove_directory(item)
            else:
                print("Temp item wasn't a file or directory?")
        except OSError as error:
            print(error, file=sys.stderr)  # Don't fail.


def _remove_directory(path: Path) -> None:
    """Recursively removes a directory tree."""
    shutil.rmtree(path, ignore_errors=True)


def remove_chrome_temporary_files() -> None:
    """Nukes files that may have leaked from crashed or killed unittests."""
    # Print what is cleaned up so bots don't time out on large cleanups and so
    # the leaks show up in build logs. A leading dot was added at some point;
    # support the name with and without it.
    log_regex = r'^\.?(com\.google\.Chrome|org\.chromium)\.'
    if sys.platform == 'win32':
        _remove_chrome_desktop_files()
    elif sys.platform.startswith('linux'):
        _log_and_remove_files(Path(tempfile.gettempdir()), log_regex)
        _log_and_remove_files(Path('/dev/shm'), log_regex)
    elif sys.platform.startswith('darwin'):
        home = os.environ['HOME']
        for name in ('Chromium', 'Google Chrome'):
            crash_path = Path(
                home
            ) / 'Library' / 'Application Support' / name / 'Crash Reports'
            _log_and_remove_files(crash_path, r'^.+\.dmp$')
    else:
        raise NotImplementedError(
            f'Platform "{sys.platform}" is not currently supported.')


def _remove_chrome_desktop_files() -> None:
    """Removes Chrome shortcuts from the Windows desktop."""
    desktop_path = Path(os.environ['USERPROFILE']) / 'Desktop'
    _log_and_remove_files(desktop_path, r'^(Chromium|chrome) \(.+\)?\.lnk$')


# -- Running the test -------------------------------------------------------


def run_command(command: list[str],
                *,
                env: dict[str, str],
                parser: gtest_output.GTestParser | None = None,
                symbolizer: list[str] | None = None) -> int:
    """Runs `command`, streaming its output and returning its exit code.

    When neither `parser` nor `symbolizer` is given the child inherits stdout
    and stderr directly. Otherwise stdout and stderr are merged and streamed
    line by line: each line is echoed and, when `parser` is set, handed to
    `parser.process_line`. When `symbolizer` is given the test's output is piped
    through it (used for offline sanitizer symbolization).

    Unlike the chrome-infra original, which returned the first non-zero code
    among all processes, this returns the test binary's exit code and only falls
    back to the symbolizer's code when the test itself succeeded.
    """
    print('\n' + subprocess.list2cmdline(command))
    if symbolizer:
        print('     | ' + subprocess.list2cmdline(symbolizer))
    sys.stdout.flush()
    sys.stderr.flush()

    if not parser and not symbolizer:
        return subprocess.run(command, env=env, check=False).returncode

    test_proc = subprocess.Popen(command,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT,
                                 env=env,
                                 text=True,
                                 encoding='utf-8',
                                 errors='replace')
    symbolizer_proc = None
    if symbolizer:
        symbolizer_proc = subprocess.Popen(symbolizer,
                                           stdin=test_proc.stdout,
                                           stdout=subprocess.PIPE,
                                           stderr=subprocess.STDOUT,
                                           env=env,
                                           text=True,
                                           encoding='utf-8',
                                           errors='replace')
        # Allow the test process to receive SIGPIPE if the symbolizer exits.
        assert test_proc.stdout is not None
        test_proc.stdout.close()

    reader = symbolizer_proc if symbolizer_proc else test_proc
    assert reader.stdout is not None
    for line in reader.stdout:
        line = line.rstrip('\n').rstrip('\r')
        print(line)
        if parser:
            parser.process_line(line)

    test_code = test_proc.wait()
    if symbolizer_proc:
        symbolizer_code = symbolizer_proc.wait()
        if test_code == 0 and symbolizer_code != 0:
            return symbolizer_code
    return test_code


def build_test_binary_command(test_exe_path: Path,
                              options: Options) -> list[str]:
    """Builds the base command for running a test binary."""
    command = [str(test_exe_path)]
    if options.parse_gtest_output:
        command.append('--test-launcher-bot-mode')
    return command


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


def get_sanitizer_symbolize_command(strip_path_prefix: str = '') -> list[str]:
    """Returns the command that symbolizes sanitizer reports offline."""
    script_path = Path('src', 'tools', 'valgrind', 'asan',
                       'asan_symbolize.py').resolve()
    command = [sys.executable, str(script_path)]
    if strip_path_prefix:
        command.append(strip_path_prefix)
    return command


def configure_sanitizer_tools(options: Options) -> SanitizerSetup:
    """Builds the environment and args needed by the enabled sanitizers.

    Returns a `SanitizerSetup`; the caller is responsible for merging its `env`
    into the test environment and appending its `extra_args` to the command.
    """
    env: dict[str, str] = {}
    extra_args: list[str] = []
    use_symbolization_script = False

    any_sanitizer = (options.enable_asan or options.enable_tsan
                     or options.enable_msan or options.enable_lsan)
    if any_sanitizer:
        # Instruct GTK to use malloc while running under a sanitizer.
        env['G_SLICE'] = 'always-malloc'
        env['NSS_DISABLE_ARENA_FREE_LIST'] = '1'
        env['NSS_DISABLE_UNLOAD'] = '1'

    symbolizer_path = str(
        Path('src', 'third_party', 'llvm-build', 'Release+Asserts', 'bin',
             'llvm-symbolizer').resolve())

    # Symbolization of sanitizer reports.
    symbolization_options: list[str] = []
    if sys.platform in ('win32', 'cygwin'):
        # On Windows the in-process symbolizer works even when sandboxed.
        symbolization_options = []
    elif options.enable_tsan or options.enable_lsan:
        # TSan and LSan aren't sandbox-compatible, so online symbolization is
        # used; they need it to apply suppressions.
        symbolization_options = [
            'symbolize=1',
            f'external_symbolizer_path={symbolizer_path}',
            f'strip_path_prefix={options.strip_path_prefix}',
        ]
    elif options.enable_asan or options.enable_msan:
        # ASan and MSan symbolize offline via asan_symbolize.py. When leak
        # detection is also on, the online (LSan) options above apply instead.
        symbolization_options = ['symbolize=0']
        env['LLVM_SYMBOLIZER_PATH'] = symbolizer_path
        use_symbolization_script = True

    def append_env(key: str, values: list[str]) -> None:
        # Appends to an existing environment value if one is already set.
        existing = env.get(key) or os.environ.get(key)
        env[key] = ' '.join(filter(bool, [existing, *values]))

    if options.enable_tsan:
        append_env('TSAN_OPTIONS', symbolization_options)
        # Disable sandboxing under TSan. crbug.com/223602.
        extra_args.append('--no-sandbox')

    if options.enable_lsan:
        # These options take effect only for standalone LSan.
        append_env('LSAN_OPTIONS', symbolization_options)
        extra_args.append('--no-sandbox')

    if options.enable_asan:
        asan_options = list(symbolization_options)
        if options.enable_lsan:
            asan_options.append('detect_leaks=1')
        append_env('ASAN_OPTIONS', asan_options)

    if options.enable_msan:
        msan_options = list(symbolization_options)
        if options.enable_lsan:
            msan_options.append('detect_leaks=1')
        append_env('MSAN_OPTIONS', msan_options)

    return SanitizerSetup(env=env,
                          extra_args=extra_args,
                          use_symbolization_script=use_symbolization_script)


def report_outcome(test_name: str, exit_code: int,
                   parser: gtest_output.GTestParser) -> None:
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
        if (parser.parsing_errors() or parser.failed_tests()
                or parser.memory_tool_report_hashes()):
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
    else:
        print('crashed or hung')


def _prepare_linux_env(build_dir: Path, options: Options,
                       extra_env: dict[str, str]) -> None:
    """Applies the Linux-specific environment tweaks before running a test."""
    # Unset proxy variables; when set they cause some tests to hang.
    # crbug.com/139638.
    for proxy_var in ('http_proxy', 'HTTPS_PROXY'):
        if proxy_var in os.environ:
            del os.environ[proxy_var]
            print(f'Deleted {proxy_var} environment variable.')

    extra_env['CHROME_DEVEL_SANDBOX'] = CHROME_SANDBOX_PATH

    ld_library_path = ''
    if options.enable_lsan:
        # Use the debug libstdc++ under LSan for complete stack traces.
        ld_library_path += '/usr/lib/x86_64-linux-gnu/debug:'
    ld_library_path += f'{build_dir}:{build_dir}/lib:{build_dir}/lib.target'
    extra_env['LD_LIBRARY_PATH'] = ld_library_path


def run_test(options: Options, args: list[str], extra_env: dict[str, str],
             sanitizer: SanitizerSetup) -> int:
    """Runs the test on the current desktop platform and returns its code."""
    build_dir = options.build_dir
    test_exe = args[0]
    if options.run_python_script:
        test_exe_path = Path(test_exe)
    else:
        test_exe_path = build_dir / test_exe

    if sys.platform.startswith('linux'):
        _prepare_linux_env(build_dir, options, extra_env)

    if not options.run_python_script and not test_exe_path.exists():
        raise FileNotFoundError(f'Unable to find {test_exe_path}')

    # Nuke stale chrome items left in the temp dir by previous runs.
    remove_chrome_temporary_files()

    if options.run_python_script:
        command = [sys.executable, test_exe]
    else:
        command = build_test_binary_command(test_exe_path, options)
    command.extend(args[1:])

    parser = create_log_processor(options)
    json_parser = (parser if isinstance(parser, gtest_output.GTestJSONParser)
                   else None)

    start_xvfb = False
    try:
        start_xvfb = (sys.platform.startswith('linux') and options.xvfb)
        if start_xvfb:
            start_virtual_x(build_dir)

        if json_parser:
            json_file = json_parser.prepare_json_file(
                options.test_launcher_summary_output)
            command.append(f'--test-launcher-summary-output={json_file}')
        elif options.test_launcher_summary_output:
            command.append('--test-launcher-summary-output='
                           f'{options.test_launcher_summary_output}')

        symbolizer = None
        if sanitizer.use_symbolization_script:
            symbolizer = get_sanitizer_symbolize_command(
                options.strip_path_prefix)

        env = _build_env(extra_env)
        result = run_command(command,
                             env=env,
                             parser=parser,
                             symbolizer=symbolizer)
    finally:
        if start_xvfb:
            stop_virtual_x()
        if json_parser:
            json_parser.process_json_file()

    if options.parse_gtest_output and parser is not None:
        report_outcome(options.test_type, result, parser)

    return result


def _build_env(extra_env: dict[str, str]) -> dict[str, str]:
    """Returns a copy of the current environment with `extra_env` applied.

    Also sets `CHROMIUM_TEST_LAUNCHER_BOT_MODE` (bot mode: test retries, stdio
    redirection, etc.) via the environment rather than a flag, since some
    internal callers run non-gtest code through here.
    """
    env = os.environ.copy()
    if extra_env:
        print('Additional test environment:')
        for key, value in sorted(extra_env.items()):
            print(f'  {key}={value}')
    env.update(extra_env)
    env['CHROMIUM_TEST_LAUNCHER_BOT_MODE'] = '1'
    return env


# -- Virtual X server (Linux) -----------------------------------------------


def _xvfb_pid_file() -> Path:
    """Returns the path of the Xvfb pid file (unique per display index)."""
    return Path(tempfile.gettempdir()) / f'xvfb-{_XVFB_DISPLAY_INDEX}.pid'


def start_virtual_x(build_dir: Path) -> None:
    """Starts a virtual X server (and openbox) and points DISPLAY at it.

    Linux only; assumes `Xvfb` and `openbox` are installed. When
    `build_dir/xdisplaycheck` exists it is used to verify the X connection.
    """
    # Use a pid file to ensure no xvfb from a previous run is still around.
    stop_virtual_x()

    xdisplaycheck_path = build_dir / 'xdisplaycheck' if build_dir else None

    display = f':{_XVFB_DISPLAY_INDEX}'
    os.environ['DISPLAY'] = display

    # Parts of Xvfb hard-code "/tmp" for temporary files and expect to hardlink
    # against files created in TMPDIR. crbug.com/715848.
    env = os.environ.copy()
    if env.get('TMPDIR') and env['TMPDIR'] != '/tmp':
        print(f'Overriding TMPDIR to "/tmp" for Xvfb, was: {env["TMPDIR"]}')
        env['TMPDIR'] = '/tmp'

    if xdisplaycheck_path and xdisplaycheck_path.exists():
        print('Verifying Xvfb is not running ...')
        check = subprocess.run([str(xdisplaycheck_path), '--noserver'],
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT,
                               env=env,
                               check=False)
        if check.returncode == 0:
            raise RuntimeError('Display already present.')

        lock_file = Path(f'/tmp/.X{_XVFB_DISPLAY_INDEX}-lock')
        if lock_file.exists():
            print(f'Removing stale xvfb lock file {lock_file!r}')
            try:
                lock_file.unlink()
            except OSError as error:
                print(f'Removing xvfb lock file failed: {error}')

    proc = subprocess.Popen([
        'Xvfb', display, '-screen', '0', '1280x800x24', '-ac', '-dpi', '96',
        '-maxclients', '512'
    ],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT,
                            env=env)
    _xvfb_pid_file().write_text(str(proc.pid), encoding='utf-8', newline='')

    if xdisplaycheck_path and xdisplaycheck_path.exists():
        print('Verifying Xvfb has started...')
        start = time.time()
        check = subprocess.run([str(xdisplaycheck_path)],
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT,
                               text=True,
                               check=False)
        elapsed = time.time() - start
        if check.returncode != 0:
            print(f'xdisplaycheck failed after {elapsed:.0f} seconds.')
            print('xdisplaycheck output:')
            for out_line in (check.stdout or '').splitlines():
                print(f'> {out_line}')
            if proc.poll() is None:
                print('Xvfb still running, stopping.')
                proc.terminate()
            raise RuntimeError(check.stdout or 'xdisplaycheck failed')
        print(f'xdisplaycheck succeeded after {elapsed:.0f} seconds.')

    # Some ChromeOS tests need a window manager.
    subprocess.Popen(['openbox'],
                     stdout=subprocess.PIPE,
                     stderr=subprocess.STDOUT)
    print('Window manager (openbox) started.')


def stop_virtual_x() -> None:
    """Stops the virtual X server started by `start_virtual_x`, if any.

    Killing the X server also takes down the window manager. Does nothing when
    no virtual X server is running.
    """
    pid_file = _xvfb_pid_file()
    if not pid_file.exists():
        return
    xvfb_pid = int(pid_file.read_bytes().decode('utf-8'))
    print(f'Stopping Xvfb with pid {xvfb_pid} ...')
    try:
        os.kill(xvfb_pid, signal.SIGKILL)
    except OSError:
        print('... killing failed, presuming unnecessary.')
    pid_file.unlink()
    print('Xvfb pid file removed')


# -- CLI --------------------------------------------------------------------


def main() -> int:
    """Entry point: parses args, sets up the environment and runs the test."""
    parser = argparse.ArgumentParser(
        description='Runs a test executable and processes its output.')
    parser.add_argument('--build-dir',
                        required=True,
                        type=Path,
                        help='Path to the build dir.')
    parser.add_argument('--run-python-script',
                        action='store_true',
                        help='Treat the first argument as a python script.')
    parser.add_argument('--xvfb',
                        dest='xvfb',
                        action='store_true',
                        default=True,
                        help='Start a virtual X server on Linux.')
    parser.add_argument('--no-xvfb',
                        dest='xvfb',
                        action='store_false',
                        help='Do not start a virtual X server on Linux.')
    parser.add_argument('--builder-group',
                        default=None,
                        help='The group of the builder (accepted for '
                        'compatibility; currently unused).')
    parser.add_argument('--builder-name',
                        default=None,
                        help='The name of the builder running this script.')
    parser.add_argument('--test-type',
                        default='',
                        help="The test name, e.g. 'brave_unit_tests'.")
    parser.add_argument('--parse-gtest-output',
                        action='store_true',
                        help='Parse the gtest output and report the outcome.')
    parser.add_argument('--enable-asan',
                        action='store_true',
                        help='Enable AddressSanitizer.')
    parser.add_argument('--enable-lsan',
                        action='store_true',
                        help='Enable LeakSanitizer.')
    parser.add_argument('--enable-msan',
                        action='store_true',
                        help='Enable MemorySanitizer.')
    parser.add_argument('--enable-tsan',
                        action='store_true',
                        help='Enable ThreadSanitizer.')
    parser.add_argument('--strip-path-prefix',
                        default='build/src/out/Release/../../',
                        help='Source paths in stack traces are stripped of '
                        'prefixes ending with this substring (sanitizers).')
    parser.add_argument('--test-launcher-summary-output',
                        type=Path,
                        default=None,
                        help='Path to the test launcher summary JSON file.')
    parser.add_argument('test_command',
                        nargs=argparse.REMAINDER,
                        help='The test executable (or python script) and its '
                        'arguments.')

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
        strip_path_prefix=parsed.strip_path_prefix,
        test_launcher_summary_output=parsed.test_launcher_summary_output,
    )
    args = list(parsed.test_command)

    logging.basicConfig(
        level=logging.INFO,
        format=
        '%(asctime)s %(filename)s:%(lineno)-3d %(levelname)s %(message)s',
        datefmt='%y%m%d %H:%M:%S')

    print(f'[Running on builder: "{options.builder_name}"]')

    if sys.platform not in ('win32', 'cygwin') and not sys.platform.startswith(
        ('linux', 'darwin')):
        print(f'Unknown sys.platform value {sys.platform!r}', file=sys.stderr)
        return 1

    extra_env: dict[str, str] = {}
    sanitizer = configure_sanitizer_tools(options)
    extra_env.update(sanitizer.env)
    args.extend(sanitizer.extra_args)

    did_launch_dbus = launch_dbus()
    try:
        temp_files = get_temp_count()
        result = run_test(options, args, extra_env, sanitizer)
        new_temp_files = get_temp_count()
        if temp_files > new_temp_files:
            print(
                f'Confused: {temp_files - new_temp_files} files were deleted '
                f'from {tempfile.gettempdir()} during the test run',
                file=sys.stderr)
        elif temp_files < new_temp_files:
            print(
                f'{new_temp_files - temp_files} new files were left in '
                f'{tempfile.gettempdir()}: fix the tests to clean up after '
                'themselves.',
                file=sys.stderr)
        return result
    finally:
        if did_launch_dbus:
            shutdown_dbus()


if __name__ == '__main__':
    sys.exit(main())
