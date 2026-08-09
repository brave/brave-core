# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

from typing import TYPE_CHECKING

import post_process
from PB.recipes.brave.toolchains.rust.package_rust import InputProperties

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = [
    'path', 'step', 'depot_tools', 'chromium_checkout', 'brave_core_checkout',
    'osx_sdk', 'platform'
]

PROPERTIES = InputProperties


def RunSteps(api: RecipeScriptApi, properties: InputProperties) -> None:
    chromium_src = api.chromium_checkout.ensure_checkout(
        ref=properties.chromium_ref, depth=1)

    brave_core_root = api.brave_core_checkout.deploy('tools/cr/toolchains')

    vpython3 = api.depot_tools.vpython3()
    with api.osx_sdk.ensure(chromium_src):
        api.step('build rust toolchain', [
            vpython3,
            brave_core_root / 'tools/cr/toolchains/build_rust_toolchain.py',
            '--out-dir',
            api.path.out,
            '--chromium-src',
            chromium_src,
            '--brave-subrevision',
            str(properties.brave_subrevision),
            '--clear',
            '--no-full-toolchain',
            '--upload',
        ])


def GenTests(api):
    # Happy path: checkout (with a seeded git cache), deploy the build scripts,
    # then build. `with_git_cache`/`deployed` seed chromium_checkout's and
    # brave_core_checkout's preconditions. Non-mac: osx_sdk.ensure() is a
    # no-op, so no Xcode install/reset around the build step.
    yield api.test(
        'linux',
        api.platform.name('linux'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.brave_core_checkout.deployed('tools/cr/toolchains'),
        api.properties(brave_subrevision=1, chromium_ref='151.0.7917.1'),
        api.post_process(post_process.MustRun, 'clone from git cache'),
        api.post_process(post_process.MustRun, 'checkout tag'),
        api.post_process(post_process.DoesNotRun, 'read mac_sdk.gni'),
        api.post_process(post_process.DoesNotRun, 'install xcode'),
        api.post_process(post_process.MustRun, 'build rust toolchain'),
        api.post_process(post_process.DoesNotRun, 'reset xcode'),
        api.post_process(post_process.StepCommandContains,
                         'build rust toolchain', ['--brave-subrevision', '1']),
        api.post_process(post_process.StepCommandContains,
                         'build rust toolchain', ['--upload']),
        api.post_process(post_process.StatusSuccess),
    )
    # On mac, the checkout's pinned Xcode is installed/selected around the
    # build step, then reset afterward.
    yield api.test(
        'mac',
        api.platform.name('mac'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.brave_core_checkout.deployed('tools/cr/toolchains'),
        api.osx_sdk.installed(),
        api.properties(brave_subrevision=1, chromium_ref='151.0.7917.1'),
        api.post_process(post_process.MustRun, 'install xcode'),
        api.post_process(post_process.MustRun, 'build rust toolchain'),
        api.post_process(post_process.MustRun, 'reset xcode'),
        api.post_process(post_process.StatusSuccess),
    )
    # Without a git cache, the checkout refuses to run.
    yield api.test(
        'no git cache',
        api.properties(brave_subrevision=1, chromium_ref='151.0.7917.1'),
        api.post_process(post_process.StatusException),
        api.post_process(post_process.DropExpectation),
        status='EXCEPTION',
    )
