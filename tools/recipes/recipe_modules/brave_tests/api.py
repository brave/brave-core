# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""The `brave_tests` module API."""

from __future__ import annotations

from typing import TYPE_CHECKING

from PB.recipe_modules.brave.brave_tests.properties import InputProperties
from recipe_api import RecipeApi

if TYPE_CHECKING:
    from recipe_modules import brave_tests
    from recipe_modules.brave_tests_builder_config import BuilderId


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

    def run_tests(self, builder_id: BuilderId) -> None:
        """Run each of the builder's gtests through `runtest.py --builder`,
        in an existing, built checkout.

        Every suite runs even if an earlier one fails; the failures are raised
        together at the end. With the `junit_xml_dir` property, each suite's
        JUnit XML report is written there as `<suite>.xml`.

        Args:
            builder_id: The builder the build runs as.

        Raises:
            StepFailure: If any suite failed.
        """
        tests = self._read_spec(builder_id).get('gtest_tests', [])
        runtest = self.m.path.brave_core / 'tools' / 'recipes' / 'runtest.py'
        build_dir = self.m.path.chromium_src / 'out' / builder_id.builder
        failed = []
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
                ]
                if self._properties.junit_xml_dir:
                    cmd += [
                        '--junit-xml-output',
                        f'{self._properties.junit_xml_dir}/{name}.xml',
                    ]
                cmd.append(test['test'])
                result = self.m.step(name, cmd, check=False)
                if result.retcode != 0:
                    failed.append(name)
        if failed:
            raise self.m.step.StepFailure(
                f'test suites failed: {", ".join(failed)}'
            )
