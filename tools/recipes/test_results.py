# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
#
# For some of the code lifted below.
#
# Copyright 2020 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""The results of a gtest run, from the launcher's JSON summary.

`read_summary()` gives every run of each test, and its outcome. The
statuses and failure messages are derived as `result_adapter gtest` derives
them for ResultDB, from infra/go/src/infra/tools/result_adapter/gtest.go,
with a crash named by its sanitiser report when it has one.

`SuiteOutcome` is what `runtest.py` hands the recipes of a suite's run:
how many tests had each outcome, and the tests that failed, flaky ones
included, with what they failed with and the output that says why.
"""

from __future__ import annotations

import base64
import binascii
from collections import defaultdict
from collections.abc import Iterable
from dataclasses import dataclass
import enum
import json
from pathlib import Path
import re
from typing import Any

import sanitizer_reports


class Outcome(enum.Enum):
    PASSED = 'passed'
    FAILED = 'failed'
    SKIPPED = 'skipped'
    # The launcher could not run the test, or lost its result.
    ERRORED = 'errored'


_OUTCOMES = {
    'SUCCESS': Outcome.PASSED,
    'FAILURE': Outcome.FAILED,
    'FAILURE_ON_EXIT': Outcome.FAILED,
    'TIMEOUT': Outcome.FAILED,
    'CRASH': Outcome.FAILED,
    'SKIPPED': Outcome.SKIPPED,
    'EXCESSIVE_OUTPUT': Outcome.FAILED,
    'NOTRUN': Outcome.ERRORED,
    'UNKNOWN': Outcome.ERRORED,
}

# A test the launcher makes up, not a real one.
_SYNTHETIC_TEST_RE = re.compile(
    r'^GoogleTestVerification.Uninstantiated(?:Type)?ParameterizedTestSuite'
    r'<\w+>$'
)

# Output from SCOPED_TRACE calls, which isn't part of the failure.
_GOOGLE_TEST_TRACE_RE = re.compile(r'Google Test trace:.*$', re.DOTALL)

_FILE_DOT_EXTENSION = r'[a-zA-Z0-9_.]+\.[a-zA-Z0-9_]+'
_LINE_NUMBER = r'(?:\([0-9]+\)|:[0-9]+)'
_WHOLE_NUMBER_RE = re.compile(r'(\d+)')
_HEX = r'[0-9a-f]+'
_FUNCTION_NAME = r'[a-zA-Z_][a-zA-Z0-9_:]*'

_CHECK_FAILED_RE = re.compile(
    rf'.*?({_FILE_DOT_EXTENSION})({_LINE_NUMBER})\]? (Check failed:.*)'
)
_FATAL_MESSAGE_RE = re.compile(
    rf'.*FATAL.*?({_FILE_DOT_EXTENSION})({_LINE_NUMBER})\]? (.*)'
)

_UNIX_SEGFAULT_MESSAGE_RE = re.compile(
    r'[Rr]eceived signal(?:: Segmentation fault| 11 SEGV_MAPERR)'
)
_WINDOWS_SEGFAULT_MESSAGE_RE = re.compile(
    r'Received fatal exception EXCEPTION_ACCESS_VIOLATION\n\s+(.*)\n'
)
_SIGACTION_LINE_RE = re.compile(r'(?:__GI___sigaction|libc.so| 0x0 \+).*\n(.*)')
_SIGTRAMP_LINE_RE = re.compile(r'_sigtramp \+.*\n(.*)')
_LINUX_FUNCTION_RE = re.compile(rf'#\d 0x{_HEX} ({_FUNCTION_NAME})')
_LINUX_ASAN_FUNCTION_RE = re.compile(rf'#\d 0x{_HEX} in ({_FUNCTION_NAME})')
_WINDOWS_FUNCTION_RE = re.compile(rf'({_FUNCTION_NAME})\s+\[0x0x{_HEX}\+\d+\]')
_MAC_FUNCTION_RE = re.compile(rf'\d.* 0x{_HEX} ({_FUNCTION_NAME})')

_GTEST_EXPECTATION_RE = re.compile(
    r'([a-zA-Z0-9_.]+\.[a-zA-Z0-9_]+):([0-9]+): Failure\n(.*?)Stack trace:',
    re.DOTALL,
)
_GTEST_EXPECTATION_WINDOWS_RE = re.compile(
    r'([a-zA-Z0-9_.]+\.[a-zA-Z0-9_]+)\(([0-9]+)\): error: (.*?)Stack trace:',
    re.DOTALL,
)


def _decode(value: str) -> str:
    """A base64 field of the summary as text, `''` if it doesn't decode."""
    try:
        return base64.b64decode(value).decode('utf-8', errors='replace')
    except (binascii.Error, ValueError):
        return ''


def _number(text: str) -> str:
    return _WHOLE_NUMBER_RE.search(text).group(0)


def _outcome(run: dict) -> Outcome:
    status = run.get('status', 'UNKNOWN')
    outcome = _OUTCOMES.get(status, Outcome.ERRORED)
    if outcome is Outcome.PASSED and any(
        part.get('type') == 'skip' for part in run.get('result_parts', [])
    ):
        return Outcome.SKIPPED
    return outcome


def _part_message(part: dict) -> str:
    """A result part's summary, located by its file name and line."""
    summary = _GOOGLE_TEST_TRACE_RE.sub(
        '', _decode(part.get('summary_base64', ''))
    ).strip()
    file_name = re.split(r'[\\/]', part.get('file', ''))[-1]
    return f'{file_name}({part.get("line", 0)}): {summary}'


def _failure_messages(run: dict) -> list[str]:
    """The run's failures, fatal ones first."""
    parts = run.get('result_parts', [])
    return [
        _part_message(part)
        for kind in ('fatal_failure', 'failure')
        for part in parts
        if part.get('type') == kind
    ]


def _fatal_message(snippet: str) -> str:
    match = _FATAL_MESSAGE_RE.search(snippet)
    check_failed = _CHECK_FAILED_RE.search(snippet)
    if match is None or (
        check_failed is not None and check_failed.start() < match.start()
    ):
        match = check_failed
    if match is None:
        return ''
    return f'{match[1]}({_number(match[2])}): {match[3].strip()}'


def _segfault_function(snippet: str) -> str:
    """The function at the top of a segfault's stack, past the signal
    handling."""
    if unix := _UNIX_SEGFAULT_MESSAGE_RE.search(snippet):
        rest = snippet[unix.end() :]
        line = _SIGACTION_LINE_RE.search(rest) or _SIGTRAMP_LINE_RE.search(rest)
        if line is None:
            return ''
        for function_re in (
            _LINUX_ASAN_FUNCTION_RE,
            _LINUX_FUNCTION_RE,
            _MAC_FUNCTION_RE,
        ):
            if function := function_re.search(line[1]):
                return function[1]
        return ''
    if windows := _WINDOWS_SEGFAULT_MESSAGE_RE.search(snippet):
        if function := _WINDOWS_FUNCTION_RE.search(windows[1]):
            return function[1]
    return ''


def _segfault_message(snippet: str) -> str:
    function = _segfault_function(snippet)
    return f'{function}: segfault' if function else ''


def _sanitizer_message(snippet: str) -> str:
    """The headline of the first sanitiser report in `snippet`."""
    parser = sanitizer_reports.ReportParser()
    for line in snippet.split('\n'):
        parser.process_line(line)
    parser.finish()
    reports = parser.result.reports
    return reports[0].headline if reports else ''


def _expectation_message(snippet: str) -> str:
    match = _GTEST_EXPECTATION_RE.search(snippet)
    windows = _GTEST_EXPECTATION_WINDOWS_RE.search(snippet)
    if match is None or (
        windows is not None and windows.start() < match.start()
    ):
        match = windows
    if match is None:
        return ''
    message = _GOOGLE_TEST_TRACE_RE.sub('', match[3]).strip()
    return f'{match[1]}({match[2]}): {message}'


def snippet_failure_message(snippet: str) -> str:
    """A failure message from the run's output, for a test that crashed
    before gtest could report its failures."""
    for extract in (
        _sanitizer_message,
        _fatal_message,
        _segfault_message,
        _expectation_message,
    ):
        if message := extract(snippet):
            return message
    return ''


@dataclass(frozen=True)
class Run:
    outcome: Outcome
    status: str
    seconds: float
    message: str
    snippet: str

    @classmethod
    def parse(cls, run: dict) -> Run:
        outcome = _outcome(run)
        snippet = _decode(run.get('output_snippet_base64', ''))
        message = ''
        if outcome is Outcome.FAILED:
            messages = _failure_messages(run)
            message = (
                messages[0] if messages else snippet_failure_message(snippet)
            )
        return cls(
            outcome=outcome,
            status=run.get('status', 'UNKNOWN'),
            seconds=run.get('elapsed_time_ms', 0) / 1000,
            message=message or run.get('status', 'UNKNOWN'),
            snippet=snippet,
        )


@dataclass(frozen=True)
class TestResult:
    """Every run of a test, across the summary's iterations."""

    name: str
    runs: tuple[Run, ...]

    @property
    def last(self) -> Run:
        return self.runs[-1]

    @property
    def outcome(self) -> Outcome:
        """The test's outcome: its last run's."""
        return self.last.outcome

    @property
    def failed_runs(self) -> tuple[Run, ...]:
        return tuple(
            run
            for run in self.runs
            if run.outcome in (Outcome.FAILED, Outcome.ERRORED)
        )

    @property
    def flaky(self) -> bool:
        """Whether the test passed once retried."""
        return self.outcome is Outcome.PASSED and bool(self.failed_runs)


def read_summary(summary: dict) -> tuple[TestResult, ...]:
    """Each real test in the launcher's JSON `summary`, by name."""
    runs: dict[str, list[Run]] = defaultdict(list)
    for iteration in summary.get('per_iteration_data', []):
        for test, test_runs in iteration.items():
            if _SYNTHETIC_TEST_RE.match(test):
                continue
            runs[test].extend(Run.parse(run) for run in test_runs)
    return tuple(
        TestResult(name, tuple(test_runs))
        for name, test_runs in sorted(runs.items())
        if test_runs
    )


# How much of a failed run's output a `TestFailure` keeps: its first lines,
# where gtest names the failure, and its last, where a crash ends.
OUTPUT_HEAD_LINES = 10
OUTPUT_TAIL_LINES = 40
# The longest line kept whole; a longer one keeps its start and its end.
OUTPUT_LINE_LENGTH = 2000


def trim_output(output: str) -> str:
    """`output` cut down to what says why a run failed."""
    lines = [_trim_line(line) for line in output.splitlines()]
    if len(lines) <= OUTPUT_HEAD_LINES + OUTPUT_TAIL_LINES:
        return '\n'.join(lines)
    cut = len(lines) - OUTPUT_HEAD_LINES - OUTPUT_TAIL_LINES
    return '\n'.join(
        [
            *lines[:OUTPUT_HEAD_LINES],
            f'[... {cut} lines cut ...]',
            *lines[-OUTPUT_TAIL_LINES:],
        ]
    )


def _trim_line(line: str) -> str:
    if len(line) <= OUTPUT_LINE_LENGTH:
        return line
    half = OUTPUT_LINE_LENGTH // 2
    return f'{line[:half]} [... {len(line) - 2 * half} characters cut ...] {line[-half:]}'


@dataclass(frozen=True)
class TestFailure:
    """A test that failed at least once."""

    name: str
    # Whether it passed once retried.
    flaky: bool
    # Its last failed run's gtest status, e.g. `FAILURE` or `CRASH`.
    status: str
    # What that run failed with.
    message: str
    # That run's output, trimmed to what says why.
    output: str

    @classmethod
    def of(cls, result: TestResult) -> TestFailure:
        failed = result.failed_runs[-1]
        return cls(
            name=result.name,
            flaky=result.flaky,
            status=failed.status,
            message=failed.message,
            output=trim_output(failed.snippet),
        )

    def to_json(self) -> dict[str, Any]:
        return {
            'name': self.name,
            'flaky': self.flaky,
            'status': self.status,
            'message': self.message,
            'output': self.output,
        }

    @classmethod
    def from_json(cls, data: dict[str, Any]) -> TestFailure:
        return cls(**data)


@dataclass(frozen=True)
class SuiteOutcome:
    """How a suite's tests did."""

    # How many tests had each outcome, by `Outcome` value.
    counts: dict[str, int]
    # The tests that failed at least once, flaky ones included.
    failures: tuple[TestFailure, ...] = ()

    @classmethod
    def of(cls, results: Iterable[TestResult]) -> SuiteOutcome:
        results = tuple(results)
        counts = {outcome.value: 0 for outcome in Outcome}
        for result in results:
            counts[result.outcome.value] += 1
        return cls(
            counts=counts,
            failures=tuple(
                TestFailure.of(result)
                for result in results
                if result.failed_runs
            ),
        )

    def to_json(self) -> dict[str, Any]:
        return {
            'counts': dict(self.counts),
            'failures': [failure.to_json() for failure in self.failures],
        }

    @classmethod
    def from_json(cls, data: dict[str, Any]) -> SuiteOutcome:
        return cls(
            counts=dict(data['counts']),
            failures=tuple(
                TestFailure.from_json(failure) for failure in data['failures']
            ),
        )

    def write(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(
            json.dumps(self.to_json(), indent=2) + '\n', encoding='utf-8'
        )

    @classmethod
    def read(cls, path: Path) -> SuiteOutcome:
        return cls.from_json(json.loads(path.read_text(encoding='utf-8')))
