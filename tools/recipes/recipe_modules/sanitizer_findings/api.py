# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `sanitizer_findings` module API."""

from __future__ import annotations

from collections.abc import Mapping
import logging
from typing import TYPE_CHECKING

from recipe_api import RecipeApi
from sanitizer_reports import RunReports

from .findings import Findings

if TYPE_CHECKING:
    from recipe_modules import sanitizer_findings

# The output property the findings are reported in.
OUTPUT_PROPERTY = 'sanitizer_findings'


class SanitizerFindingsApi(RecipeApi):
    """Reports the bugs a build's sanitisers found."""

    m: sanitizer_findings.DEPS

    def report(self, reports_by_suite: Mapping[str, RunReports]) -> Findings:
        """Gather each suite's sanitiser reports into findings, one per
        signature, and report them as the build's `sanitizer_findings`
        output property.

        Args:
            reports_by_suite: The sanitiser reports in each suite's output.

        Returns:
            The findings.
        """
        findings = Findings.gather(reports_by_suite)
        step = self.m.step('sanitizer findings', None)
        step.presentation.properties[OUTPUT_PROPERTY] = findings.to_json()
        for finding in findings.findings:
            logging.info(
                'Sanitizer finding %s, %d time(s) in %s: %s',
                finding.id,
                finding.occurrences,
                ', '.join(finding.suites),
                finding.headline,
            )
        return findings
