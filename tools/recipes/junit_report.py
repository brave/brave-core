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
"""A JUnit XML report of a gtest run, from the launcher's JSON summary.

It has the layout of the launcher's own `--gtest_output=xml` report, which
CI systems read as JUnit: a `testsuite` per test case and a `testcase` per
test, with its last run's result. The statuses and failure messages are
derived as `result_adapter gtest` derives them for ResultDB, from
infra/go/src/infra/tools/result_adapter/gtest.go. A test's earlier failed
runs are reported as Surefire's `flakyFailure` (the last run passed) or
`rerunFailure` (it failed too).

Compared with the launcher's report that `pnpm run test --output_xml` asks
for (with Brave's failure details from
chromium_src/base/test/launcher/test_results_tracker.cc):

- Skipped tests are `<skipped/>`, GTEST_SKIP() ones included, not failures.
- A failure's message is its first assertion with its location, fatal ones
  first, or for a crash the CHECK, segfault, ASan report (with the
  allocation site) or expectation found in its output, instead of
  `[  FAILED  ] Case.Test`.
- A failure's type is the gtest status (CRASH, TIMEOUT, ...), and a test the
  launcher lost (NOTRUN, UNKNOWN) is an `<error>`.
- A failing test has one `<failure>`, not an empty one besides Brave's.
- Earlier failed runs of a retried test are kept, so flaky tests show.
- Names, messages and output are escaped and stripped of the characters XML
  cannot carry (e.g. the escape of a terminal colour), where the launcher
  prints them as they are, which can make the whole report unreadable.
- The launcher's synthetic `GoogleTestVerification` tests are left out.
"""

from __future__ import annotations

import base64
import binascii
from collections import defaultdict
from dataclasses import dataclass
import enum
from pathlib import Path
import re
from xml.etree import ElementTree


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

_ASAN_STACK_FRAME_RE = re.compile(
    rf'^\s*#\d+ 0x{_HEX} in (.*?)\(.*\).*?({_FILE_DOT_EXTENSION})'
    rf'({_LINE_NUMBER})'
)
_ASAN_RE = re.compile(r'ERROR: AddressSanitizer: (\S+)(.*)\n')
_ASAN_LOCATION_RE = re.compile(rf'({_FILE_DOT_EXTENSION})({_LINE_NUMBER})')
_ASAN_ALLOCATED_RE = re.compile(r'previously allocated by thread .* here:\n')

_GTEST_EXPECTATION_RE = re.compile(
    r'([a-zA-Z0-9_.]+\.[a-zA-Z0-9_]+):([0-9]+): Failure\n(.*?)Stack trace:',
    re.DOTALL,
)
_GTEST_EXPECTATION_WINDOWS_RE = re.compile(
    r'([a-zA-Z0-9_.]+\.[a-zA-Z0-9_]+)\(([0-9]+)\): error: (.*?)Stack trace:',
    re.DOTALL,
)

# Characters XML 1.0 cannot carry, even escaped.
_INVALID_XML_CHARS_RE = re.compile('[\x00-\x08\x0b\x0c\x0e-\x1f￾￿]')


def _decode(value: str) -> str:
    """A base64 field of the summary as text, `''` if it doesn't decode."""
    try:
        return base64.b64decode(value).decode('utf-8', errors='replace')
    except (binascii.Error, ValueError):
        return ''


def _xml_text(value: str) -> str:
    return _INVALID_XML_CHARS_RE.sub('�', value)


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


def _asan_stack_frames(snippet: str) -> list[re.Match]:
    """The first run of lines in `snippet` that are ASan stack frames."""
    frames = []
    for line in snippet.split('\n'):
        if match := _ASAN_STACK_FRAME_RE.search(line):
            frames.append(match)
        elif frames:
            break
    return frames


def _asan_allocated_location(snippet: str) -> str:
    """The first frame of the stack in `snippet` that isn't `operator new`."""
    for frame in _asan_stack_frames(snippet):
        if frame[1] != 'operator new':
            return f'{frame[2]}({_number(frame[3])})'
    return ''


def _asan_message(snippet: str) -> str:
    match = _ASAN_RE.search(snippet)
    if match is None:
        return ''
    location = 'Unknown'
    if location_match := _ASAN_LOCATION_RE.search(snippet, match.end(1)):
        location = f'{location_match[1]}({_number(location_match[2])})'
    message = f'{location}: ASan {match[1]}'
    if allocated := _ASAN_ALLOCATED_RE.search(snippet, match.start(2)):
        if allocated_location := _asan_allocated_location(
            snippet[allocated.end() :]
        ):
            message += f'\nAllocated: {allocated_location}'
    return message


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
        _fatal_message,
        _segfault_message,
        _asan_message,
        _expectation_message,
    ):
        if message := extract(snippet):
            return message
    return ''


@dataclass(frozen=True)
class _Run:
    outcome: Outcome
    status: str
    seconds: float
    message: str
    snippet: str

    @classmethod
    def parse(cls, run: dict) -> _Run:
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


def _runs(summary: dict) -> dict[str, list[_Run]]:
    """Every run of each real test, across the summary's iterations."""
    runs: dict[str, list[_Run]] = defaultdict(list)
    for iteration in summary.get('per_iteration_data', []):
        for test, test_runs in iteration.items():
            if _SYNTHETIC_TEST_RE.match(test):
                continue
            runs[test].extend(_Run.parse(run) for run in test_runs)
    return runs


def _add_testcase(element: ElementTree.Element, runs: list[_Run]) -> None:
    last = runs[-1]
    if last.outcome is Outcome.SKIPPED:
        ElementTree.SubElement(element, 'skipped')
    elif last.outcome is not Outcome.PASSED:
        tag = 'failure' if last.outcome is Outcome.FAILED else 'error'
        failure = ElementTree.SubElement(
            element, tag, message=_xml_text(last.message), type=last.status
        )
        failure.text = _xml_text(last.snippet)
    rerun_tag = (
        'flakyFailure' if last.outcome is Outcome.PASSED else 'rerunFailure'
    )
    for run in runs[:-1]:
        if run.outcome in (Outcome.FAILED, Outcome.ERRORED):
            rerun = ElementTree.SubElement(
                element,
                rerun_tag,
                message=_xml_text(run.message),
                type=run.status,
            )
            ElementTree.SubElement(rerun, 'stackTrace').text = _xml_text(
                run.snippet
            )


def _set_counts(element: ElementTree.Element, counts: dict) -> None:
    for key, value in counts.items():
        element.set(key, f'{value:.3f}' if key == 'time' else str(value))


def render(summary: dict, suite: str) -> str:
    """The JUnit XML report of the gtest JSON `summary` of `suite`."""
    cases: dict[str, list[tuple[str, list[_Run]]]] = defaultdict(list)
    for test, runs in sorted(_runs(summary).items()):
        if runs:
            case, _, name = test.partition('.')
            cases[case].append((name or case, runs))

    root = ElementTree.Element('testsuites', name=_xml_text(suite))
    totals = defaultdict(float)
    for case, tests in cases.items():
        element = ElementTree.SubElement(
            root, 'testsuite', name=_xml_text(case)
        )
        counts = {
            'tests': 0,
            'failures': 0,
            'errors': 0,
            'skipped': 0,
            'time': 0.0,
        }
        for name, runs in tests:
            testcase = ElementTree.SubElement(
                element,
                'testcase',
                name=_xml_text(name),
                classname=_xml_text(case),
                time=f'{runs[-1].seconds:.3f}',
            )
            _add_testcase(testcase, runs)
            counts['tests'] += 1
            counts['time'] += runs[-1].seconds
            if runs[-1].outcome is Outcome.FAILED:
                counts['failures'] += 1
            elif runs[-1].outcome is Outcome.ERRORED:
                counts['errors'] += 1
            elif runs[-1].outcome is Outcome.SKIPPED:
                counts['skipped'] += 1
        _set_counts(element, counts)
        for key, value in counts.items():
            totals[key] += value
    _set_counts(
        root,
        {
            key: totals[key] if key == 'time' else int(totals[key])
            for key in ('tests', 'failures', 'errors', 'skipped', 'time')
        },
    )
    ElementTree.indent(root)
    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        + ElementTree.tostring(root, encoding='unicode')
        + '\n'
    )


def write(path: Path, summary: dict, suite: str) -> None:
    """Writes the JUnit XML report of `summary` to `path`."""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(render(summary, suite), encoding='utf-8')
