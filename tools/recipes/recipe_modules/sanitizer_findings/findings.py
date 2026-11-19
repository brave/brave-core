# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The bugs a build's sanitisers found: its suites' reports, one finding per
signature."""

from __future__ import annotations

from collections.abc import Iterable, Mapping
from dataclasses import dataclass
from typing import Any

from sanitizer_reports import Report, RunReports


@dataclass(frozen=True)
class Finding:
    """A bug a sanitiser found, as one or more reports of it."""

    # Its first report, which stands for the others: they share its
    # signature.
    report: Report
    # The suites and tests that reported it, in the order they did.
    suites: tuple[str, ...]
    tests: tuple[str, ...]
    # How many times it was reported.
    occurrences: int

    @property
    def id(self) -> str:
        return self.report.id

    @property
    def signature(self) -> str:
        return self.report.signature

    @property
    def headline(self) -> str:
        return self.report.headline

    def to_json(self) -> dict[str, Any]:
        return {
            'id': self.id,
            'signature': self.signature,
            'headline': self.headline,
            'sanitizer': str(self.report.sanitizer),
            'kind': self.report.kind,
            'suites': list(self.suites),
            'tests': list(self.tests),
            'occurrences': self.occurrences,
        }


@dataclass(frozen=True)
class Findings:
    """The findings of a build, in the order they were first reported."""

    findings: tuple[Finding, ...] = ()
    # The suites whose reports may be the sanitiser's failures rather than
    # the code's, as it ran out of memory.
    unreliable_suites: tuple[str, ...] = ()

    @classmethod
    def gather(cls, reports_by_suite: Mapping[str, RunReports]) -> Findings:
        """The findings in each suite's reports, one per signature."""
        occurrences: dict[str, list[tuple[str, Report]]] = {}
        for suite, run in reports_by_suite.items():
            for report in run.reports:
                occurrences.setdefault(report.id, []).append((suite, report))
        return cls(
            findings=tuple(
                Finding(
                    report=reported[0][1],
                    suites=_unique(suite for suite, _ in reported),
                    tests=_unique(
                        report.test for _, report in reported if report.test
                    ),
                    occurrences=len(reported),
                )
                for reported in occurrences.values()
            ),
            unreliable_suites=tuple(
                suite
                for suite, run in reports_by_suite.items()
                if run.unreliable
            ),
        )

    def to_json(self) -> dict[str, Any]:
        return {
            'findings': [finding.to_json() for finding in self.findings],
            'unreliable_suites': list(self.unreliable_suites),
        }


def _unique(values: Iterable[str]) -> tuple[str, ...]:
    return tuple(dict.fromkeys(values))
