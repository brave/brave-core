# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Tests for `brave_core_checkout` running a build as a builder, and for the
config validation."""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    brave_tests_builder_config,
    chromium_checkout,
    env,
    platform,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    brave_core_checkout: brave_core_checkout.API
    brave_tests_builder_config: brave_tests_builder_config.API
    env: env.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    brave_core_checkout: brave_core_checkout.TEST_API
    brave_tests_builder_config: brave_tests_builder_config.TEST_API
    chromium_checkout: chromium_checkout.TEST_API
    env: env.TEST_API
    platform: platform.TEST_API


def RunSteps(api: DEPS):
    mode = api.env.get('MODE')
    # Each of these raises.
    if mode == 'brave without builder':
        api.brave_core_checkout.set_config('brave')
    elif mode == 'deprecated':
        api.brave_core_checkout.set_config('deprecated_brave')
    else:
        _run_as_builder(api, mode)


def _run_as_builder(api: DEPS, mode: str) -> None:
    builder = api.brave_tests_builder_config.lookup_builder()
    assert builder is not None
    api.brave_core_checkout.configure_build(*builder)
    if mode == 'deprecated item':
        api.brave_core_checkout.apply_config('deprecated_asan')
    elif mode == 'deprecated target_os':
        api.brave_core_checkout.apply_config('deprecated_android')
    elif mode == 'compile target':
        api.brave_core_checkout.compile(deprecated_target='brave')
    elif mode == 'run_tests':
        api.brave_core_checkout.run_tests()
    elif mode:
        api.brave_core_checkout.ensure_checkout()


def GenTests(api: TEST_DEPS):
    def builder_test(name, *data, mode='ensure_checkout', status='SUCCESS'):
        return api.test(
            name,
            api.env.set('MODE', mode),
            api.brave_core_checkout.with_git_cache(),
            api.brave_core_checkout.git_cache_populated(),
            api.chromium_checkout.existing_checkout(),
            api.platform.name('linux'),
            *data,
            status=status,
        )

    # The `.env` carries only what stays machine-local, and `pnpm` syncs as the
    # builder.
    yield builder_test(
        'builder',
        api.brave_tests_builder_config.ci_build(),
        api.brave_core_checkout.rbe(siso_cache_dir='/b/siso'),
        api.post_process(
            post_process.StepCommandContains,
            'pnpm run sync',
            ['--builder', 'linux-x64-asan-brave'],
        ),
    )
    # Cross-compiling, with the host set explicitly.
    yield builder_test(
        'android builder',
        api.brave_tests_builder_config.ci_build(
            builder='android-arm64-rel',
            target_platform='android',
            target_arch='arm',
        ),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
    # The deprecated items set what the builder's own configuration does.
    for mode in ('deprecated item', 'deprecated target_os'):
        yield builder_test(
            mode,
            api.brave_tests_builder_config.ci_build(),
            api.post_process(post_process.StatusException),
            api.post_process(post_process.DropExpectation),
            mode=mode,
            status='EXCEPTION',
        )
    # A builder compiles its own targets, and its tests do not run through
    # `pnpm run test`.
    for mode in ('compile target', 'run_tests'):
        yield builder_test(
            mode,
            api.brave_tests_builder_config.ci_build(),
            api.post_process(post_process.DoesNotRun, 'build'),
            api.post_process(post_process.StatusException),
            api.post_process(post_process.DropExpectation),
            mode=mode,
            status='EXCEPTION',
        )
    # The builder runs on another platform.
    yield builder_test(
        'host mismatch',
        api.brave_tests_builder_config.ci_build(host_platform='mac'),
        api.post_process(post_process.DoesNotRun, 'pnpm run sync'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # Cross-compiling, without the host set: iOS is expected to build on Mac.
    yield builder_test(
        'implicit cross compile',
        api.brave_tests_builder_config.ci_build(
            target_platform='ios', target_arch='arm', host_platform=None
        ),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # A target the host cannot build.
    yield api.test(
        'cannot compile',
        api.env.set('MODE', 'ensure_checkout'),
        api.brave_core_checkout.with_git_cache(),
        api.platform.name('mac'),
        api.brave_tests_builder_config.ci_build(
            target_platform='win', host_platform='mac'
        ),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    yield api.test(
        'brave without builder',
        api.env.set('MODE', 'brave without builder'),
        api.brave_core_checkout.with_git_cache(),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
    # The switch to try builds without the deprecated mode.
    yield api.test(
        'deprecated mode disabled',
        api.env.set('MODE', 'deprecated'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.disable_deprecated_dotenv(),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
