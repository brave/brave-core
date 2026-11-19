# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""What a builder's tests did, suite by suite."""

from __future__ import annotations

from dataclasses import dataclass

from sanitizer_reports import RunReports
from test_results import SuiteOutcome


@dataclass(frozen=True)
class SuiteResult:
    """What one suite did."""

    # The suite's name, which its step is named after.
    name: str
    passed: bool
    # How its tests did, and the sanitiser reports in its output; either is
    # None when `runtest.py` wrote none, as when it could not run the suite.
    outcome: SuiteOutcome | None
    sanitizer_reports: RunReports | None


@dataclass(frozen=True)
class TestRun:
    """What a builder's suites did, in the order they ran."""

    suites: tuple[SuiteResult, ...] = ()

    @property
    def failed_suites(self) -> tuple[str, ...]:
        return tuple(suite.name for suite in self.suites if not suite.passed)

    @property
    def outcomes(self) -> dict[str, SuiteOutcome]:
        """How each suite's tests did, by suite, for those it is known of."""
        return {
            suite.name: suite.outcome
            for suite in self.suites
            if suite.outcome is not None
        }

    @property
    def sanitizer_reports(self) -> dict[str, RunReports]:
        """Each suite's sanitiser reports, by suite, for those that have
        them."""
        return {
            suite.name: suite.sanitizer_reports
            for suite in self.suites
            if suite.sanitizer_reports is not None
        }
