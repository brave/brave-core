# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Parsers for gtest output, used by `runtest.py` to report test outcomes.

Two parsers are provided, both satisfying the `GTestParser` protocol:

- `GTestLogParser` consumes the test binary's stdout line by line, tracking the
  `[ RUN ]` / `[ OK ]` / `[ FAILED ]` / `[ SKIPPED ]` directives, timeouts,
  disabled/flaky counts and memory-tool reports.
- `GTestJSONParser` ignores stdout and instead reads the out-of-band JSON
  summary written via `--test-launcher-summary-output`.
"""

from __future__ import annotations

from dataclasses import dataclass, field
import enum
import json
from pathlib import Path
import re
import tempfile
from typing import Protocol

# These labels match the ones emitted by gtest's JSON summary.
TEST_UNKNOWN_LABEL = 'UNKNOWN'
TEST_SUCCESS_LABEL = 'SUCCESS'
TEST_FAILURE_LABEL = 'FAILURE'
TEST_FAILURE_ON_EXIT_LABEL = 'FAILURE_ON_EXIT'
TEST_EXCESSIVE_OUTPUT_LABEL = 'EXCESSIVE_OUTPUT'
TEST_CRASH_LABEL = 'CRASH'
TEST_TIMEOUT_LABEL = 'TIMEOUT'
TEST_SKIPPED_LABEL = 'SKIPPED'
TEST_WARNING_LABEL = 'WARNING'

# Statuses in the JSON summary that count as the test having passed.
_JSON_PASSING_STATUSES = frozenset((TEST_SUCCESS_LABEL, TEST_SKIPPED_LABEL))


class TestStatus(enum.Enum):
    """The state a test can be in while parsing the gtest log."""

    STARTED = 'started'
    OK = 'OK'
    FAILED = 'failed'
    TIMEOUT = 'timeout'
    # A test that failed when run in parallel but passed when retried alone.
    WARNING = 'warning'
    SKIPPED = 'skipped'


class GTestParser(Protocol):
    """The interface `runtest.report_outcome` consumes.

    Both `GTestLogParser` and `GTestJSONParser` structurally satisfy this.
    """

    def process_line(self, line: str) -> None:
        ...

    def parsing_errors(self) -> list[str]:
        ...

    def failed_tests(self) -> list[str]:
        ...

    def disabled_tests(self) -> int | str:
        ...

    def flaky_tests(self) -> int | str:
        ...

    def memory_tool_report_hashes(self) -> list[str]:
        ...

    def running_tests(self) -> list[str]:
        ...


class GTestLogParser:
    """Parses gtest stdout, tracking per-test state as it goes."""

    # Test names look like "x.y", with 0 or more "w/" prefixes and 0 or more
    # "/z" suffixes, e.g. "SomeName/SomeTestCase.SomeTest/1".
    _TEST_NAME_RE = r'((\w+/)*\w+\.\w+(/\w+)*)'

    def __init__(self) -> None:
        self.completed = False
        self.retrying_failed = False

        self._current_test = ''
        self._failure_description: list[str] = []
        self._current_report_hash = ''
        self._current_report: list[str] = []
        self._parsing_failures = False
        self._line_number = 0

        # Human-readable descriptions of lines that produced parsing errors.
        self._internal_error_lines: list[str] = []

        # 'test.name' -> (status, [description]).
        self._test_status: dict[str, tuple[TestStatus, list[str]]] = {}

        # 'hash' -> [report lines].
        self._memory_tool_reports: dict[str, list[str]] = {}

        # Either a count or the string 'some' when the count can't be parsed.
        self._disabled_tests: int | str = 0
        self._flaky_tests: int | str = 0

        self._test_start = re.compile(r'\[\s+RUN\s+\] ' + self._TEST_NAME_RE)
        self._test_ok = re.compile(r'\[\s+OK\s+\] ' + self._TEST_NAME_RE)
        self._test_fail = re.compile(r'\[\s+FAILED\s+\] ' + self._TEST_NAME_RE)
        self._test_passed = re.compile(r'\[\s+PASSED\s+\] \d+ tests?.')
        self._test_skipped = re.compile(r'\[\s+SKIPPED\s+\] ' +
                                        self._TEST_NAME_RE)
        self._test_name = re.compile(self._TEST_NAME_RE)
        self._run_test_cases_line = re.compile(
            r'\[\s*\d+\/\d+\]\s+[0-9\.]+s ' + self._TEST_NAME_RE + ' .+')
        self._test_timeout = re.compile(
            r'Test timeout \([0-9]+ ms\) exceeded for ' + self._TEST_NAME_RE)
        self._disabled = re.compile(r'\s*YOU HAVE (\d+) DISABLED TEST')
        self._flaky = re.compile(r'\s*YOU HAVE (\d+) FLAKY TEST')
        self._report_start = re.compile(
            r'### BEGIN MEMORY TOOL REPORT \(error hash=#([0-9A-F]+)#\)')
        self._report_end = re.compile(
            r'### END MEMORY TOOL REPORT \(error hash=#([0-9A-F]+)#\)')
        self._retry_message = re.compile('RETRYING FAILED TESTS:')

    # -- GTestParser protocol ------------------------------------------------

    def process_line(self, line: str) -> None:
        """Feed a single line of the test log to the parser."""
        self._line_number += 1

        # Some tests run subprocesses that write to the shared stdout buffer.
        # Their output can appear between a newline and a gtest directive
        # ('[ RUN ]', etc), which breaks parsing. Detect that case and split
        # the mixed line into two.
        gtest_regexps = (
            self._test_start,
            self._test_ok,
            self._test_fail,
            self._test_passed,
            self._test_skipped,
        )
        match = None
        for regexp in gtest_regexps:
            match = regexp.search(line)
            if match:
                break

        if not match or match.start() == 0:
            self._process_line(line)
        else:
            self._process_line(line[:match.start()])
            self._process_line(line[match.start():])

    def parsing_errors(self) -> list[str]:
        """Returns lines that caused parsing errors."""
        return self._internal_error_lines

    def failed_tests(self,
                     include_fails: bool = False,
                     include_flaky: bool = False) -> list[str]:
        """Returns tests that failed, timed out, or never finished (crashed).

        This is only accurate once the complete log has been processed, since
        currently-running tests are reported as failed.
        """
        return (self._tests_by_status(TestStatus.FAILED, include_fails,
                                      include_flaky) +
                self._tests_by_status(TestStatus.TIMEOUT, True, True) +
                self._tests_by_status(TestStatus.WARNING, include_fails,
                                      include_flaky) + self.running_tests())

    def disabled_tests(self) -> int | str:
        """Returns the number of disabled tests (or 'some' if unparseable)."""
        return self._disabled_tests

    def flaky_tests(self) -> int | str:
        """Returns the number of flaky tests (or 'some' if unparseable)."""
        return self._flaky_tests

    def memory_tool_report_hashes(self) -> list[str]:
        """Returns the report hashes found in the log."""
        return list(self._memory_tool_reports.keys())

    def running_tests(self) -> list[str]:
        """Returns tests that appear to still be running."""
        return self._tests_by_status(TestStatus.STARTED, True, True)

    # -- Additional accessors ------------------------------------------------

    def passed_tests(self,
                     include_fails: bool = False,
                     include_flaky: bool = False) -> list[str]:
        """Returns tests that passed."""
        return self._tests_by_status(TestStatus.OK, include_fails,
                                     include_flaky)

    def skipped_tests(self,
                      include_fails: bool = False,
                      include_flaky: bool = False) -> list[str]:
        """Returns tests that were skipped."""
        return self._tests_by_status(TestStatus.SKIPPED, include_fails,
                                     include_flaky)

    def completed_without_failure(self) -> bool:
        """Returns True if all tests completed and none failed unexpectedly."""
        return self.completed and not self.failed_tests()

    # -- Internals -----------------------------------------------------------

    def _status_of_test(self, test: str) -> TestStatus | None:
        entry = self._test_status.get(test)
        return entry[0] if entry else None

    def _tests_by_status(self, status: TestStatus, include_fails: bool,
                         include_flaky: bool) -> list[str]:
        tests = [
            name for name in self._test_status
            if self._status_of_test(name) == status
        ]
        if not include_fails:
            tests = [t for t in tests if 'FAILS_' not in t]
        if not include_flaky:
            tests = [t for t in tests if 'FLAKY_' not in t]
        return tests

    def _record_error(self, line: str, reason: str) -> None:
        self._internal_error_lines.append(
            f'{self._line_number}: {line.strip()} [{reason}]')

    def _process_line(self, line: str) -> None:
        """Parses a single (already de-mixed) line, updating test state."""
        # Note: when sharding, the disabled/flaky counts are read multiple
        # times; only the most recent values are kept (they should match).

        if self._run_test_cases_line.match(line):
            # A run_test_cases.py progress line. If the current test never
            # reported a result, treat it as timed out.
            if self._current_test and self._status_of_test(
                    self._current_test) == TestStatus.STARTED:
                self._test_status[self._current_test] = (
                    TestStatus.TIMEOUT, self._failure_description)
            self._current_test = ''
            self._failure_description = []
            return

        if self._test_passed.match(line):
            self.completed = True
            self._current_test = ''
            return

        results = self._disabled.match(line)
        if results:
            self._disabled_tests = self._parse_count(results.group(1),
                                                     self._disabled_tests)
            return

        results = self._flaky.match(line)
        if results:
            self._flaky_tests = self._parse_count(results.group(1),
                                                  self._flaky_tests)
            return

        results = self._test_start.match(line)
        if results:
            if self._current_test and self._status_of_test(
                    self._current_test) == TestStatus.STARTED:
                self._test_status[self._current_test] = (
                    TestStatus.TIMEOUT, self._failure_description)
            test_name = results.group(1)
            self._test_status[test_name] = (TestStatus.STARTED,
                                            ['Did not complete.'])
            self._current_test = test_name
            if self.retrying_failed:
                self._failure_description = self._test_status[test_name][1]
                self._failure_description.extend(['', 'RETRY OUTPUT:', ''])
            else:
                self._failure_description = []
            return

        results = self._test_ok.match(line)
        if results:
            test_name = results.group(1)
            status = self._status_of_test(test_name)
            if status != TestStatus.STARTED:
                self._record_error(line, f'success while in status {status}')
            if self.retrying_failed:
                self._test_status[test_name] = (TestStatus.WARNING,
                                                self._failure_description)
            else:
                self._test_status[test_name] = (TestStatus.OK, [])
            self._failure_description = []
            self._current_test = ''
            return

        results = self._test_skipped.match(line)
        if results:
            test_name = results.group(1)
            status = self._status_of_test(test_name)
            # Skipped tests are listed again in the summary.
            if status not in (TestStatus.STARTED, TestStatus.SKIPPED):
                self._record_error(line, f'skipped while in status {status}')
            self._test_status[test_name] = (TestStatus.SKIPPED, [])
            self._failure_description = []
            self._current_test = ''
            return

        results = self._test_fail.match(line)
        if results:
            test_name = results.group(1)
            status = self._status_of_test(test_name)
            if status not in (TestStatus.STARTED, TestStatus.FAILED,
                              TestStatus.TIMEOUT):
                self._record_error(line, f'failure while in status {status}')
            # Don't overwrite the description when a failing test is listed a
            # second time in the summary, or if it already timed out.
            if status not in (TestStatus.FAILED, TestStatus.TIMEOUT):
                self._test_status[test_name] = (TestStatus.FAILED,
                                                self._failure_description)
            self._failure_description = []
            self._current_test = ''
            return

        results = self._test_timeout.search(line)
        if results:
            test_name = results.group(1)
            status = self._status_of_test(test_name)
            if status not in (TestStatus.STARTED, TestStatus.FAILED):
                self._record_error(line, f'timeout while in status {status}')
            self._test_status[test_name] = (TestStatus.TIMEOUT,
                                            self._failure_description +
                                            ['Killed (timed out).'])
            self._failure_description = []
            self._current_test = ''
            return

        results = self._report_start.match(line)
        if results:
            report_hash = results.group(1)
            if report_hash in self._memory_tool_reports:
                self._record_error(line, 'multiple reports for this hash')
            self._memory_tool_reports[report_hash] = []
            self._current_report_hash = report_hash
            self._current_report = []
            return

        results = self._report_end.match(line)
        if results:
            report_hash = results.group(1)
            if not self._current_report_hash:
                self._record_error(line, 'no BEGIN matches this END')
            elif report_hash != self._current_report_hash:
                self._record_error(
                    line,
                    f'expected (error hash=#{self._current_report_hash}#)')
            else:
                self._memory_tool_reports[self._current_report_hash] = (
                    self._current_report)
            self._current_report_hash = ''
            self._current_report = []
            return

        if self._retry_message.match(line):
            self.retrying_failed = True
            return

        # Reports are generated after all tests finish, so a random line while
        # a report is open belongs to the current report.
        if self._current_report_hash:
            self._current_report.append(line)
            return

        # A random line while a test is running is collected as failure
        # context. Tests may run simultaneously, so this can be slightly off.
        if self._current_test:
            self._failure_description.append(line)

        # Parse the trailing "Failing tests:" list, capturing tests that crash
        # after their OK line.
        if self._parsing_failures:
            results = self._test_name.match(line)
            if results:
                test_name = results.group(1)
                status = self._status_of_test(test_name)
                if status in (None, TestStatus.OK):
                    self._test_status[test_name] = (TestStatus.FAILED, [
                        'Unknown error, see stdio log.'
                    ])
            else:
                self._parsing_failures = False
        elif line.startswith('Failing tests:'):
            self._parsing_failures = True

    @staticmethod
    def _parse_count(raw: str, current: int | str) -> int | str:
        """Parses a disabled/flaky count, falling back to 'some'."""
        try:
            value = int(raw)
        except ValueError:
            value = 0
        if value > 0 and isinstance(current, int):
            return value
        # A safety net for a case that shouldn't happen but isn't fatal.
        return 'some'


@dataclass
class GTestJSONParser:
    """Reads the out-of-band gtest JSON summary instead of stdout.

    The summary is written by the test binary at the path handed to it via
    `--test-launcher-summary-output`; see `prepare_json_file`.
    """

    _json_file_path: Path | None = None
    _delete_json_file: bool = False
    _disabled_tests: set[str] = field(default_factory=set)
    _passed_tests: set[str] = field(default_factory=set)
    _failed_tests: set[str] = field(default_factory=set)
    _flaky_tests: set[str] = field(default_factory=set)
    _parsing_errors: list[str] = field(default_factory=list)

    # -- GTestParser protocol ------------------------------------------------

    def process_line(self, line: str) -> None:
        # Deliberately a no-op: the JSON summary is parsed out-of-band.
        pass

    def parsing_errors(self) -> list[str]:
        return self._parsing_errors

    def failed_tests(self) -> list[str]:
        return sorted(self._failed_tests)

    def disabled_tests(self) -> int:
        return len(self._disabled_tests)

    def flaky_tests(self) -> int:
        return len(self._flaky_tests)

    def memory_tool_report_hashes(self) -> list[str]:
        return []

    def running_tests(self) -> list[str]:
        return []

    # -- Additional accessors ------------------------------------------------

    def passed_tests(self) -> list[str]:
        return sorted(self._passed_tests)

    # -- JSON file lifecycle -------------------------------------------------

    def prepare_json_file(self, cmdline_path: Path | None) -> Path:
        """Returns the path the test binary should write its summary to.

        If the caller supplied a path we use (and keep) it; otherwise we create
        a temporary file and delete it after parsing.
        """
        if cmdline_path:
            self._json_file_path = cmdline_path
            self._delete_json_file = False
        else:
            with tempfile.NamedTemporaryFile(delete=False,
                                             suffix='.json') as temp_file:
                self._json_file_path = Path(temp_file.name)
            self._delete_json_file = True
        return self._json_file_path

    def process_json_file(self) -> None:
        """Reads and parses the summary file prepared by `prepare_json_file`."""
        if not self._json_file_path:
            return

        raw = self._json_file_path.read_bytes().decode('utf-8')
        try:
            json_data = json.loads(raw)
        except ValueError:
            # Only signal an error for a non-empty file. An empty file most
            # likely means the binary doesn't support JSON output.
            if raw:
                self._parsing_errors = raw.split('\n')
        else:
            self.process_json_data(json_data)

        if self._delete_json_file:
            self._json_file_path.unlink(missing_ok=True)

    def process_json_data(self, json_data: dict) -> None:
        """Populates the pass/fail/disabled/flaky sets from parsed JSON."""
        self._disabled_tests.update(json_data.get('disabled_tests', []))

        for iteration_data in json_data.get('per_iteration_data', []):
            for test_name, test_runs in iteration_data.items():
                if not test_runs:
                    continue
                if test_runs[-1].get('status') in _JSON_PASSING_STATUSES:
                    self._passed_tests.add(test_name)
                else:
                    self._failed_tests.add(test_name)

                first_status = test_runs[0].get('status')
                for run_data in test_runs:
                    # A run whose result differs from the first marks the test
                    # as flaky.
                    if run_data.get('status') != first_status:
                        self._flaky_tests.add(test_name)
