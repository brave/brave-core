# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""`ensure()` reads the gni pin from the `path` module's `chromium_src` when
no explicit checkout is given.
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
    with api.osx_sdk.ensure():
        pass


def GenTests(api: TEST_DEPS):
    yield api.test(
        'mac',
        api.platform.name('mac'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.deployed('tools/cr'),
        api.osx_sdk.installed(),
        api.post_process(
            post_process.StepCommandContains,
            'read mac_sdk.gni',
            ['[WORKSPACE]/b/src/build/config/mac/mac_sdk.gni'],
        ),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
