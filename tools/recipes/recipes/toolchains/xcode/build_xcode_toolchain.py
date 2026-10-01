# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Build a hermetic, reproducible Xcode toolchain archive for a Chromium tag.

Wraps `tools/cr/toolchains/build_xcode_toolchain.py`, which reads the macOS SDK
Chromium pins at `--chromium-tag`, deploys the exact released Xcode that ships
it, and packs a deterministic `.tar.gz` (plus a sibling YAML index) into the
output directory. macOS only; everything it needs is fetched from gitiles and
xcodereleases.com, so -- unlike the Rust toolchain recipe -- no Chromium
checkout is required, only a shallow deploy of the build script itself.
"""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from PB.recipes.brave.toolchains.xcode.build_xcode_toolchain import (
    InputProperties,
)
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    depot_tools,
    path,
    platform,
    step,
)
from recipe_test_api import RecipeTestApi


@dataclass
class DEPS(RecipeScriptApi):
    brave_core_checkout: brave_core_checkout.API
    depot_tools: depot_tools.API
    path: path.API
    step: step.API


@dataclass
class TEST_DEPS(RecipeTestApi):
    brave_core_checkout: brave_core_checkout.TEST_API
    platform: platform.TEST_API


PROPERTIES = InputProperties


def RunSteps(api: DEPS, properties: InputProperties) -> None:
    brave_core_root = api.brave_core_checkout.deploy('tools/cr')

    vpython3 = api.depot_tools.vpython3()
    # `--clear` wipes any prior output so every run starts from a clean out
    # dir. `--upload` publishes the archive + index to the internal build-deps
    # bucket.
    cmd = [
        vpython3,
        brave_core_root / 'tools/cr/toolchains/build_xcode_toolchain.py',
        '--out-dir',
        api.path.out,
        '--chromium-tag',
        properties.chromium_tag,
        '--clear',
        '--upload',
    ]
    api.step('build xcode toolchain', cmd)


def GenTests(api: TEST_DEPS):
    # Happy path: deploy the build scripts on a mac host, then build.
    # `deployed` seeds brave_core_checkout's post-checkout path check.
    yield api.test(
        'mac',
        api.platform.name('mac'),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.deployed('tools/cr'),
        api.properties(chromium_tag='150.0.7841.1'),
        api.post_process(post_process.MustRun, 'build xcode toolchain'),
        api.post_process(
            post_process.StepCommandContains,
            'build xcode toolchain',
            ['--chromium-tag', '150.0.7841.1'],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'build xcode toolchain',
            ['--clear'],
        ),
        api.post_process(
            post_process.StepCommandContains,
            'build xcode toolchain',
            ['--upload'],
        ),
        api.post_process(post_process.StatusSuccess),
    )
