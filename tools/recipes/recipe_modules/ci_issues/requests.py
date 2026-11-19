# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""What a build would have issues say: one request per bug it found, from
its sanitiser findings and its failing tests.

A sanitiser finding is a request of its own, by its signature. A failing
test is too, by its name, unless a sanitiser finding already explains it,
or as many tests or more as `SHARED_REASON_MIN_TESTS` failed with the same
reason, which makes them one bug: a request for the reason. A test that
passed once retried is flaky, and is no request.
"""

from __future__ import annotations

from collections.abc import Iterable, Mapping
from dataclasses import dataclass
import hashlib
import re

from recipe_modules.sanitizer_findings import Finding, Findings
from sanitizer_reports import Sanitizer
from test_results import SuiteOutcome, TestFailure, trim_output

from . import policy
from .policy import Kind, Where

# The kind of issue each sanitiser's findings are.
_SANITIZER_KINDS: dict[Sanitizer, Kind] = {
    Sanitizer.ADDRESS: Kind.ASAN,
    Sanitizer.LEAK: Kind.LSAN,
    Sanitizer.MEMORY: Kind.MSAN,
    Sanitizer.THREAD: Kind.TSAN,
    Sanitizer.UNDEFINED: Kind.UBSAN,
}

# How many tests failing with the same reason make the reason one bug.
SHARED_REASON_MIN_TESTS = 3

# The most requests a build makes for its failing tests. A build with more
# is broken as a whole, which issues per test would not say: it makes none.
MAX_TEST_REQUESTS = 15

# What varies in a failure's reason from run to run, which telling reasons
# apart leaves out: addresses, numbers and quoted values.
_REASON_VARIABLES: tuple[tuple[re.Pattern[str], str], ...] = (
    (re.compile(r'0x[0-9a-fA-F]+'), '0x…'),
    (re.compile(r'"[^"]*"'), '"…"'),
    (re.compile(r"'[^']*'"), "'…'"),
    (re.compile(r'\b\d+\b'), 'N'),
)

# The prefixes gtest gives tests that are not run on their own, which a
# test's identity leaves out, so that `PRE_Foo` is filed as `Foo`.
_TEST_PREFIXES = re.compile(r'(?<=\.)(?:PRE_|MANUAL_)+')


@dataclass(frozen=True)
class IssueRequest:
    """What one issue would say about one bug a build found."""

    # The bug's identity, which the issue carries: the same bug makes the
    # same key from build to build.
    key: str
    kind: Kind
    # The bug in a few words, e.g. a finding's error and where, or a test's
    # name, which its title and notifications say.
    headline: str
    title: str
    # What was found, for a new issue.
    summary: str
    # Where it was seen, for a new issue and for a comment on an existing
    # one.
    seen: str
    # The output that says what happened, folded away.
    details_title: str
    details: str

    def body(self, where: Where) -> str:
        return policy.body(
            self.key,
            f'{self.summary}\n\nSeen on {where.link()}: {self.seen}.',
            self.details_title,
            self.details,
        )

    def comment(self, where: Where) -> str:
        return policy.comment(
            where, self.seen, self.details_title, self.details
        )


@dataclass(frozen=True)
class Requests:
    """A build's requests, and why it makes none for what it withheld."""

    requests: tuple[IssueRequest, ...] = ()
    withheld: tuple[str, ...] = ()


def from_sanitizer_findings(findings: Findings) -> Requests:
    """A request for each finding, but those only suites whose sanitiser ran
    out of memory reported."""
    unreliable = set(findings.unreliable_suites)
    requests, withheld = [], []
    for finding in findings.findings:
        if set(finding.suites) <= unreliable:
            withheld.append(
                f'{finding.signature}: only reported by suites whose '
                'sanitizer ran out of memory'
            )
            continue
        requests.append(_sanitizer_request(finding))
    return Requests(tuple(requests), tuple(withheld))


def _sanitizer_request(finding: Finding) -> IssueRequest:
    report = finding.report
    kind = _SANITIZER_KINDS[report.sanitizer]
    top = report.own_frames[0] if report.own_frames else None
    where = (top.function or top.location) if top else None
    located = f' (`{top.location}`)' if top and top.location else ''
    headline = report.kind + (f' in {where}' if where else '')
    return IssueRequest(
        key=f'{kind}:{finding.id}',
        kind=kind,
        headline=headline,
        title=policy.title(f'{kind.short_name} finding: {headline}'),
        summary=(
            f'{report.sanitizer} reported a `{report.kind}`'
            + (f' in `{where}`{located}' if where else '')
            + f'.\n\nSignature: `{finding.signature}`'
        ),
        seen=_seen(finding.occurrences, finding.suites, finding.tests),
        details_title='Report',
        details=trim_output(report.text),
    )


def from_test_failures(
    outcomes: Mapping[str, SuiteOutcome], *, explained: Iterable[str] = ()
) -> Requests:
    """A request for each test that failed, and was neither flaky nor
    `explained`, by a sanitiser finding, or one for the reason they share
    where enough of them failed with the same one."""
    explained = set(explained)
    failures = [
        (suite, failure)
        for suite, outcome in outcomes.items()
        for failure in outcome.failures
        if not failure.flaky and failure.name not in explained
    ]
    by_reason: dict[str, list[tuple[str, TestFailure]]] = {}
    for suite, failure in failures:
        by_reason.setdefault(_reason(failure.message), []).append(
            (suite, failure)
        )
    requests = []
    for reason, failed in by_reason.items():
        if len(failed) >= SHARED_REASON_MIN_TESTS:
            requests.append(_reason_request(reason, failed))
        else:
            requests.extend(_test_request(suite, f) for suite, f in failed)
    if len(requests) > MAX_TEST_REQUESTS:
        return Requests(
            withheld=(
                f'{len(failures)} tests failed for {len(requests)} reasons, '
                f'more than the {MAX_TEST_REQUESTS} this files issues for',
            )
        )
    return Requests(tuple(requests))


def _test_request(suite: str, failure: TestFailure) -> IssueRequest:
    name = _TEST_PREFIXES.sub('', failure.name)
    return IssueRequest(
        key=f'{Kind.TEST}:{name}',
        kind=Kind.TEST,
        headline=name,
        title=policy.title(f'Test failure: {name}'),
        summary=f'`{failure.name}` failed: `{failure.message}`',
        seen=f'`{failure.name}` in `{suite}`, {failure.status}',
        details_title='Output',
        details=failure.output,
    )


def _reason_request(
    reason: str, failed: list[tuple[str, TestFailure]]
) -> IssueRequest:
    digest = hashlib.sha256(reason.encode()).hexdigest()[:16]
    tests = ', '.join(f'`{failure.name}`' for _, failure in failed)
    return IssueRequest(
        key=f'{Kind.TEST}-reason:{digest}',
        kind=Kind.TEST,
        headline=f'{len(failed)} tests failing with {reason}',
        title=policy.title(f'Test failures: {reason}'),
        summary=f'{len(failed)} tests failed with `{reason}`.',
        seen=f'{len(failed)} tests, in {_listed(s for s, _ in failed)}: {tests}',
        details_title=f'Output of `{failed[0][1].name}`',
        details=failed[0][1].output,
    )


def _reason(message: str) -> str:
    """`message` without what varies from run to run."""
    for pattern, replacement in _REASON_VARIABLES:
        message = pattern.sub(replacement, message)
    return message


def _seen(occurrences: int, suites: Iterable[str], tests: Iterable[str]) -> str:
    tests = tuple(tests)
    in_tests = f', in {_listed(tests)}' if tests else ''
    times = 'once' if occurrences == 1 else f'{occurrences} times'
    return f'{times} in {_listed(suites)}{in_tests}'


def _listed(names: Iterable[str]) -> str:
    return ', '.join(f'`{name}`' for name in dict.fromkeys(names))
