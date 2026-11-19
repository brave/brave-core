# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""A JUnit XML report of a gtest run, from the launcher's JSON summary.

It has the layout of the launcher's own `--gtest_output=xml` report, which
CI systems read as JUnit: a `testsuite` per test case and a `testcase` per
test, with its last run's result, as `test_results` reads them. A test's
earlier failed runs are reported as Surefire's `flakyFailure` (the last run
passed) or `rerunFailure` (it failed too).

Compared with the launcher's report that `pnpm run test --output_xml` asks
for (with Brave's failure details from
chromium_src/base/test/launcher/test_results_tracker.cc):

- Skipped tests are `<skipped/>`, GTEST_SKIP() ones included, not failures.
- A failure's message is its first assertion with its location, fatal ones
  first, or for a crash the sanitiser report, CHECK, segfault or
  expectation found in its output, instead of `[  FAILED  ] Case.Test`.
- A failure's type is the gtest status (CRASH, TIMEOUT, ...), and a test the
  launcher lost (NOTRUN, UNKNOWN) is an `<error>`.
- A failing test has one `<failure>`, not an empty one besides Brave's.
- Earlier failed runs of a retried test are kept, so flaky tests show.
- Names, messages and output are escaped and stripped of the characters XML
  cannot carry (e.g. the escape of a terminal colour), where the launcher
  prints them as they are, which can make the whole report unreadable.
- The launcher's synthetic `GoogleTestVerification` tests are left out.
- A run that wrote no summary, for example one that crashed at startup, is an
  `<error>` testcase named after the suite, not a clean run of no tests.
"""

from __future__ import annotations

from collections import defaultdict
from pathlib import Path
import re
from xml.etree import ElementTree

from test_results import Outcome, TestResult, read_summary

# The message of the error a run without a summary is reported as.
MISSING_SUMMARY_MESSAGE = 'no gtest JSON summary'

# Characters XML 1.0 cannot carry, even escaped.
_INVALID_XML_CHARS_RE = re.compile('[\x00-\x08\x0b\x0c\x0e-\x1f￾￿]')


def _xml_text(value: str) -> str:
    return _INVALID_XML_CHARS_RE.sub('�', value)


def _add_testcase(element: ElementTree.Element, result: TestResult) -> None:
    runs = result.runs
    last = result.last
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


def _serialise(root: ElementTree.Element) -> str:
    ElementTree.indent(root)
    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        + ElementTree.tostring(root, encoding='unicode')
        + '\n'
    )


def _render_missing_summary(suite: str) -> str:
    """The report of a run that wrote no summary: one errored testcase, so
    that it is not read as a clean run of no tests."""
    root = ElementTree.Element('testsuites', name=_xml_text(suite))
    element = ElementTree.SubElement(root, 'testsuite', name=_xml_text(suite))
    testcase = ElementTree.SubElement(
        element, 'testcase', name=_xml_text(suite), classname=_xml_text(suite)
    )
    ElementTree.SubElement(
        testcase, 'error', message=MISSING_SUMMARY_MESSAGE, type='NOSUMMARY'
    )
    counts = {'tests': 1, 'failures': 0, 'errors': 1, 'skipped': 0, 'time': 0}
    _set_counts(element, counts)
    _set_counts(root, counts)
    return _serialise(root)


def render(summary: dict | None, suite: str) -> str:
    """The JUnit XML report of the gtest JSON `summary` of `suite`, which is
    None for a run that wrote none."""
    if summary is None:
        return _render_missing_summary(suite)
    cases: dict[str, list[tuple[str, TestResult]]] = defaultdict(list)
    for result in read_summary(summary):
        case, _, name = result.name.partition('.')
        cases[case].append((name or case, result))

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
        for name, result in tests:
            testcase = ElementTree.SubElement(
                element,
                'testcase',
                name=_xml_text(name),
                classname=_xml_text(case),
                time=f'{result.last.seconds:.3f}',
            )
            _add_testcase(testcase, result)
            counts['tests'] += 1
            counts['time'] += result.last.seconds
            if result.outcome is Outcome.FAILED:
                counts['failures'] += 1
            elif result.outcome is Outcome.ERRORED:
                counts['errors'] += 1
            elif result.outcome is Outcome.SKIPPED:
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
    return _serialise(root)


def write(path: Path, summary: dict | None, suite: str) -> None:
    """Writes the JUnit XML report of `summary` to `path`."""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(render(summary, suite), encoding='utf-8')
