# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

from dataclasses import dataclass

import post_process
from PB.recipes.brave.tools.ast_grep.package_ast_grep import InputProperties
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    chromium_checkout,
    depot_tools,
    osx_sdk,
    path,
    platform,
    step,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    brave_core_checkout: brave_core_checkout.API
    chromium_checkout: chromium_checkout.API
    depot_tools: depot_tools.API
    osx_sdk: osx_sdk.API
    path: path.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    brave_core_checkout: brave_core_checkout.TEST_API
    chromium_checkout: chromium_checkout.TEST_API
    osx_sdk: osx_sdk.TEST_API
    platform: platform.TEST_API


PROPERTIES = InputProperties


def RunSteps(api: DEPS, properties: InputProperties) -> None:
    chromium_src = api.chromium_checkout.ensure_checkout(
        ref=properties.chromium_ref
    )

    brave_root = api.brave_core_checkout.deploy(
        [
            'third_party/ast-grep',
            'tools/cr',
        ]
    )

    vpython3 = api.depot_tools.vpython3()
    with api.osx_sdk.ensure(chromium_src):
        api.step(
            'package ast-grep',
            [
                vpython3,
                brave_root / 'third_party/ast-grep/package_ast_grep.py',
                '--clean',
                '--out-dir',
                api.path.out,
                '--upload',
            ],
        )


def GenTests(api: TEST_DEPS):
    # Happy path: checkout (with a seeded git cache), deploy the build
    # scripts, then package. Non-mac: osx_sdk.ensure() is a no-op, so no
    # Xcode install/reset around the packaging step.
    yield api.test(
        'linux',
        api.platform.name('linux'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.brave_core_checkout.deployed('third_party/ast-grep', 'tools/cr'),
        api.properties(chromium_ref='refs/tags/151.0.7917.1'),
        api.post_process(post_process.MustRun, 'clone from git cache'),
        api.post_process(post_process.DoesNotRun, 'read mac_sdk.gni'),
        api.post_process(post_process.DoesNotRun, 'install xcode'),
        api.post_process(post_process.MustRun, 'package ast-grep'),
        api.post_process(post_process.DoesNotRun, 'reset xcode'),
        api.post_process(post_process.StatusSuccess),
    )
    # On mac, the checkout's pinned Xcode is installed/selected around the
    # packaging step, then reset afterward -- ast-grep's build compiles the
    # tree-sitter-gn grammar with Chromium's clang, which needs a valid Xcode
    # SDK to find (`xcrun --show-sdk-path`).
    yield api.test(
        'mac',
        api.platform.name('mac'),
        api.chromium_checkout.with_git_cache(),
        api.chromium_checkout.git_cache_populated(),
        api.brave_core_checkout.deployed('third_party/ast-grep', 'tools/cr'),
        api.osx_sdk.installed(),
        api.properties(chromium_ref='refs/tags/151.0.7917.1'),
        api.post_process(post_process.MustRun, 'install xcode'),
        api.post_process(post_process.MustRun, 'package ast-grep'),
        api.post_process(post_process.MustRun, 'reset xcode'),
        api.post_process(post_process.StatusSuccess),
    )
