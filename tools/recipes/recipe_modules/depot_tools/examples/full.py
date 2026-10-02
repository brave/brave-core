# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Example recipe exercising the `depot_tools` module."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    depot_tools,
    env,
    path,
    platform,
    step,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    depot_tools: depot_tools.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    env: env.TEST_API
    path: path.TEST_API
    platform: platform.TEST_API


def RunSteps(api: DEPS):
    vpython3 = api.depot_tools.vpython3()
    api.step('use vpython3', [vpython3, '--version'])


def GenTests(api: TEST_DEPS):
    yield api.test(
        'clone',
        api.platform.name('linux'),
        api.post_process(post_process.MustRun, 'clone depot_tools'),
        api.post_process(post_process.MustRun, 'verify gclient'),
        api.post_process(
            post_process.StepCommandRE,
            'use vpython3',
            [r'.*vpython3', r'--version'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'already on path',
        api.env.on_path('gclient', '/existing/depot_tools/gclient'),
        api.post_process(post_process.DoesNotRun, 'clone depot_tools'),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'already deployed',
        api.platform.name('linux'),
        # A standalone depot_tools already sits inside the Chromium checkout.
        api.path.files('b/src/third_party/depot_tools/gclient'),
        api.post_process(post_process.DoesNotRun, 'clone depot_tools'),
        api.post_process(post_process.MustRun, 'verify gclient'),
        api.post_process(post_process.StatusSuccess),
    )
    yield api.test(
        'windows',
        api.platform.name('win'),
        api.post_process(
            post_process.StepCommandRE,
            'use vpython3',
            [r'.*vpython3\.bat', r'--version'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
