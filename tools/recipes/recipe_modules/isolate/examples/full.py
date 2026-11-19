# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `isolate` module."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import cas, isolate, path
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    isolate: isolate.API
    path: path.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    cas: cas.TEST_API
    isolate: isolate.TEST_API


def RunSteps(api: DEPS):
    if not api.isolate.enabled:
        return
    isolates = api.isolate.isolate_tests(
        api.path.chromium_src, 'out/Release', ['base_unittests']
    )
    assert isolates['base_unittests'].cwd == 'out/Release'
    isolate_ = isolate.Isolate.from_json(isolates['base_unittests'].to_json())
    result = api.isolate.run('rerun', isolate_, ['--gtest_repeat=2'])
    assert result.retcode == 0


def GenTests(api: TEST_DEPS):
    yield api.test(
        'enabled',
        api.isolate.enabled(),
        api.cas.configured(),
        api.cas.archived('isolate tests.base_unittests.archive', 'b' * 64),
        api.post_process(
            post_process.StepCommandContains,
            'isolate tests.base_unittests.mb isolate',
            ['isolate', '--no-build', 'out/Release', 'base_unittests'],
        ),
        api.post_process(post_process.MustRun, 'isolate tests.isolates'),
        api.post_process(
            post_process.StepCommandContains,
            'rerun.download',
            ['--digest', f'{"b" * 64}/100'],
        ),
        api.post_process(
            post_process.StepCommandRE,
            'rerun.run',
            [
                'vpython3',
                r'.*test_env\.py',
                r'\./base_unittests',
                '--gtest_repeat=2',
            ],
        ),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'disabled',
        api.post_process(post_process.DoesNotRun, 'isolate tests'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
