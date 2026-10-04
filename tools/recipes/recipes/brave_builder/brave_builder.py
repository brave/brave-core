# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Check out, build and test Brave."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    chromium_checkout,
    platform,
    raw_io,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    brave_core_checkout: brave_core_checkout.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    brave_core_checkout: brave_core_checkout.TEST_API
    chromium_checkout: chromium_checkout.TEST_API
    platform: platform.TEST_API
    raw_io: raw_io.TEST_API


def RunSteps(api: DEPS) -> None:
    api.brave_core_checkout.set_config('brave')
    api.brave_core_checkout.ensure_checkout()
    api.brave_core_checkout.compile()
    api.brave_core_checkout.run_tests()


def GenTests(api: TEST_DEPS):
    # Fresh workspace: brave-core and Chromium are deployed and synced, then
    # built and tested.
    yield api.test(
        'fresh',
        api.brave_core_checkout.with_git_cache(),
        api.step_data(
            'git cache exists',
            stdout=api.raw_io.output_text('/b/cache/brave-core'),
        ),
        api.step_data(
            'git cache exists (2)',
            stdout=api.raw_io.output_text('/b/cache/chromium'),
        ),
        api.brave_core_checkout.brave_core_ref('refs/heads/1.80.x'),
        api.brave_core_checkout.rbe(siso_cache_dir='/b/siso'),
        api.post_process(
            post_process.StepCommandContains,
            'brave-core checkout ref',
            ['origin/1.80.x'],
        ),
        api.post_process(post_process.MustRun, 'write .env'),
        api.post_process(post_process.MustRun, 'pnpm run sync'),
        api.post_process(
            post_process.StepCommandContains, 'build', ['--target=brave:all']
        ),
        api.platform.name('linux'),
        api.post_process(post_process.MustRun, 'test brave_all_unit_tests'),
        api.post_process(post_process.MustRun, 'test brave_browser_tests'),
        api.post_process(
            post_process.MustRun, 'test brave_interactive_ui_tests'
        ),
        api.post_process(post_process.MustRun, 'test brave_network_tests'),
        api.post_process(
            post_process.StepCommandContains,
            'test brave_all_unit_tests',
            ['pnpm', 'run', 'test', 'brave_all_unit_tests'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
    # Chromium already deployed: no sync, straight to build and test.
    yield api.test(
        'chromium present',
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.post_process(post_process.DoesNotRun, 'pnpm run sync'),
        api.platform.name('mac'),
        api.post_process(post_process.MustRun, 'build'),
        api.post_process(
            post_process.StepCommandContains,
            'test brave_network_tests',
            ['pnpm', 'run', 'test', 'brave_network_tests'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
    # A failing suite does not stop the others; the run still fails.
    yield api.test(
        'suite fails',
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.step_data('test brave_browser_tests', retcode=1),
        api.post_process(
            post_process.MustRun, 'test brave_interactive_ui_tests'
        ),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # A failing build stops the run before any tests.
    yield api.test(
        'build fails',
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.git_cache_populated(),
        api.chromium_checkout.existing_checkout(),
        api.step_data('build', retcode=1),
        api.post_process(post_process.DoesNotRun, 'test brave_all_unit_tests'),
        api.post_process(post_process.StatusFailure),
        api.post_process(post_process.DropExpectation),
        status='FAILURE',
    )
