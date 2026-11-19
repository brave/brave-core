# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `flake_analysis`: seed reruns and upstream's answers."""

from __future__ import annotations

from collections.abc import Mapping

from recipe_test_api import RecipeTestApi, StepTestData, TestData

from .api import REPEATS


class FlakeAnalysisTestApi(RecipeTestApi):
    """Seeds the steps `analyse()` runs for a failing test."""

    @staticmethod
    def summary(failures: Mapping[str, int]) -> dict:
        """The launcher's summary of a rerun in which each test failed
        *failures[test]* of `REPEATS` times."""
        return {
            'per_iteration_data': [
                {
                    test: [
                        {
                            'status': (
                                'FAILURE' if iteration < count else 'SUCCESS'
                            ),
                            'elapsed_time_ms': 1,
                            'output_snippet': '',
                        }
                    ]
                    for test, count in failures.items()
                }
                for iteration in range(REPEATS)
            ]
        }

    def rerun(self, suite: str, failures: Mapping[str, int]) -> TestData:
        """Seeds *suite*'s rerun, in which each test failed *failures[test]*
        of `REPEATS` times."""
        return self.step_data(
            f'flake analysis.{suite}.read rerun results',
            self.m.file.read_json(self.summary(failures)),
        )

    def rerun_missing(self, suite: str) -> TestData:
        """Seeds *suite*'s rerun as having written no results."""
        return self.step_data(
            f'flake analysis.{suite}.read rerun results',
            self.m.file.read_json(errno_name='ENOENT'),
        )

    def upstream(self, *statuses: str, retcode: int = 0) -> StepTestData:
        """What `is_upstream_flake.py --json` prints for a test with a
        variant in each of *statuses*."""
        return self.m.json.output_stream(
            {
                'confirmed': bool({'flaky', 'failing'} & set(statuses)),
                'variants': [{'status': status} for status in statuses],
            },
            retcode=retcode,
        )

    def upstream_status(
        self, suite: str, test: str, *statuses: str, retcode: int = 0
    ) -> TestData:
        """Seeds upstream's answer for *test* in *suite*."""
        return self.step_data(
            f'flake analysis.{suite}.upstream {test}',
            self.upstream(*statuses, retcode=retcode),
        )
