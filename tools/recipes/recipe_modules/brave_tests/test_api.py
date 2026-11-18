# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Test API for `brave_tests`: seed the builder's test spec."""

from __future__ import annotations

from recipe_test_api import RecipeTestApi, TestData


class BraveTestsTestApi(RecipeTestApi):
    def test_spec(
        self,
        *suites: str,
        builder: str = 'linux-x64-asan-brave',
        builder_group: str = 'brave.sanitisers',
    ) -> TestData:
        """Seed the builder's generated targets spec with a gtest for each of
        *suites*."""
        return self.step_data(
            f'read test spec ({builder_group}.json)',
            self.m.file.read_json(
                {
                    builder: {
                        'gtest_tests': [
                            {'name': suite, 'test': suite} for suite in suites
                        ]
                    }
                }
            ),
        )

    def junit_xml_dir(self, path: str) -> TestData:
        """Write the suites' JUnit XML reports to *path*."""
        return self.properties(**{'$brave_tests': {'junit_xml_dir': path}})
