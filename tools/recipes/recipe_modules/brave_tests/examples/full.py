# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `brave_tests` module."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    brave_tests,
    brave_tests_builder_config,
    platform,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    brave_tests: brave_tests.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    brave_core_checkout: brave_core_checkout.TEST_API
    brave_tests: brave_tests.TEST_API
    platform: platform.TEST_API


def RunSteps(api: DEPS):
    api.brave_tests.run_tests(
        brave_tests_builder_config.BuilderId(
            group='brave.sanitisers', builder='linux-x64-asan-brave'
        )
    )


def GenTests(api: TEST_DEPS):
    # Each of the spec's gtests runs through runtest.py, as the builder.
    yield api.test(
        'basic',
        api.platform.name('linux'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_tests.test_spec('brave_unit_tests', 'brave_browser_tests'),
        api.post_process(
            post_process.StepCommandContains,
            'brave_unit_tests',
            [
                '[WORKSPACE]/b/src/brave/tools/recipes/runtest.py',
                '--build-dir',
                '[WORKSPACE]/b/src/out/linux-x64-asan-brave',
                '--builder',
                'linux-x64-asan-brave',
                '--builder-name=linux-x64-asan-brave',
                '--builder-group=brave.sanitisers',
                '--test-type=brave_unit_tests',
                '--parse-gtest-output',
                'brave_unit_tests',
            ],
        ),
        api.post_process(post_process.MustRun, 'brave_browser_tests'),
        api.post_process(post_process.StatusSuccess),
    )
    # Each suite's JUnit XML report is written where CI publishes test results
    # from.
    yield api.test(
        'junit xml',
        api.platform.name('linux'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_tests.junit_xml_dir('/b/test_results'),
        api.brave_tests.test_spec('brave_unit_tests'),
        api.post_process(
            post_process.StepCommandContains,
            'brave_unit_tests',
            [
                '--junit-xml-output',
                '/b/test_results/brave_unit_tests.xml',
                'brave_unit_tests',
            ],
        ),
        api.post_process(post_process.DropExpectation),
    )
    # A failing suite does not stop the others; the build fails at the end.
    yield api.test(
        'suite fails',
        api.platform.name('linux'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_tests.test_spec('brave_unit_tests', 'brave_browser_tests'),
        api.step_data('brave_unit_tests', retcode=1),
        api.post_process(post_process.MustRun, 'brave_browser_tests'),
        api.post_process(post_process.StatusFailure),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )
    # A builder whose spec has no gtests runs none.
    yield api.test(
        'no tests',
        api.platform.name('linux'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_tests.test_spec(),
        api.post_process(post_process.DoesNotRun, 'brave_unit_tests'),
        api.post_process(post_process.DropExpectation),
    )
