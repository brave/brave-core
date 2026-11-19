# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `brave_tests` module API."""

from __future__ import annotations

from typing import TYPE_CHECKING

from PB.recipe_modules.brave.brave_tests.properties import InputProperties
from recipe_api import RecipeApi
from sanitizer_reports import RunReports
from test_results import SuiteOutcome

from .results import SuiteResult, TestRun

if TYPE_CHECKING:
    from recipe_modules import brave_tests
    from recipe_modules.brave_tests_builder_config import BuilderId


# The names of the `json.output()`s each suite's results come back in.
_TEST_RESULTS = 'test_results'
_SANITIZER_REPORTS = 'sanitizer_reports'


class BraveTestsApi(RecipeApi):
    """Runs the tests of a builder, as its generated targets spec lists
    them."""

    m: brave_tests.DEPS

    def __init__(self, properties: InputProperties) -> None:
        super().__init__()
        self._properties = properties

    def _read_spec(self, builder_id: BuilderId) -> dict:
        """The builder's own entry in its group's generated targets spec."""
        spec_file = f'{builder_id.group}.json'
        spec_path = (
            self.m.path.brave_core
            / 'infra'
            / 'config'
            / 'generated'
            / 'builders'
            / builder_id.builder
            / 'targets'
            / spec_file
        )
        spec = self.m.file.read_json(
            f'read test spec ({spec_file})',
            spec_path,
            test_data={builder_id.builder: {}},
        )
        return spec[builder_id.builder]

    def run_tests(self, builder_id: BuilderId) -> TestRun:
        """Run each of the builder's gtests through `runtest.py --builder`,
        in an existing, built checkout.

        Every suite runs even if an earlier one fails; `raise_on_failure()`
        then fails the build for those that did. With the `junit_xml_dir`
        property, each suite's JUnit XML report is written there as
        `<suite>.xml`.

        Args:
            builder_id: The builder the build runs as.

        Returns:
            What each suite did: how its tests did, and the sanitiser reports
            in its output.
        """
        tests = self._read_spec(builder_id).get('gtest_tests', [])
        runtest = self.m.path.brave_core / 'tools' / 'recipes' / 'runtest.py'
        build_dir = self.m.path.chromium_src / 'out' / builder_id.builder
        suites = []
        with self.m.brave_core_checkout.brave_core_layout():
            for test in tests:
                name = test['name']
                cmd = [
                    self.m.depot_tools.vpython3(),
                    runtest,
                    '--build-dir',
                    build_dir,
                    '--builder',
                    builder_id.builder,
                    f'--builder-name={builder_id.builder}',
                    f'--builder-group={builder_id.group}',
                    f'--test-type={name}',
                    '--parse-gtest-output',
                    '--test-results-output',
                    self.m.json.output(name=_TEST_RESULTS),
                    '--sanitizer-reports-output',
                    self.m.json.output(name=_SANITIZER_REPORTS),
                ]
                if self._properties.junit_xml_dir:
                    cmd += [
                        '--junit-xml-output',
                        f'{self._properties.junit_xml_dir}/{name}.xml',
                    ]
                cmd.append(test['test'])
                result = self.m.step(name, cmd, check=False)
                outcome = result.json.outputs.get(_TEST_RESULTS)
                reports = result.json.outputs.get(_SANITIZER_REPORTS)
                suites.append(
                    SuiteResult(
                        name=name,
                        passed=result.retcode == 0,
                        outcome=(
                            SuiteOutcome.from_json(outcome) if outcome else None
                        ),
                        sanitizer_reports=(
                            RunReports.from_json(reports) if reports else None
                        ),
                    )
                )
        return TestRun(tuple(suites))

    def raise_on_failure(self, test_run: TestRun) -> None:
        """Fail the build if any of `test_run`'s suites failed.

        Raises:
            StepFailure: If any suite failed.
        """
        if failed := test_run.failed_suites:
            raise self.m.step.StepFailure(
                f'test suites failed: {", ".join(failed)}'
            )
