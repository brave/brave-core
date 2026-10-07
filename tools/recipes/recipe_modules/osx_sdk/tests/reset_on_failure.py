# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`reset xcode` must run even when `install xcode` itself fails.

Mirrors `EphemeralXcode.deploy()`'s own guarantee that the machine is never
left pointing at an ephemeral Xcode -- here at the recipe-module level, where
`install`/`reset` are two separate steps rather than one Python `with` block.
"""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    osx_sdk,
    platform,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    osx_sdk: osx_sdk.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    brave_core_checkout: brave_core_checkout.TEST_API
    osx_sdk: osx_sdk.TEST_API
    platform: platform.TEST_API


def RunSteps(api: DEPS):
    with api.osx_sdk.ensure('/b/checkout/src'):
        pass


def GenTests(api: TEST_DEPS):
    yield api.test(
        'install fails',
        api.platform.name('mac'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.deployed('tools/cr'),
        api.osx_sdk.mac_sdk_gni(),
        api.step_data('install xcode', retcode=1),
        api.post_process(post_process.StepFailure, 'install xcode'),
        api.post_process(post_process.MustRun, 'reset xcode'),
        api.post_process(post_process.StatusFailure),
        api.post_process(post_process.DropExpectation),
    )
    # Covers the `with` block's (no-op) body, which the failure case above
    # never reaches.
    yield api.test(
        'install succeeds',
        api.platform.name('mac'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.deployed('tools/cr'),
        api.osx_sdk.installed(),
        api.post_process(post_process.MustRun, 'reset xcode'),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
