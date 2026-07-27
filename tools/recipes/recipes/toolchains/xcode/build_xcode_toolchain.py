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

from typing import TYPE_CHECKING

import post_process
from PB.recipes.brave.toolchains.xcode.build_xcode_toolchain import (
    InputProperties)

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = ['path', 'step', 'platform', 'depot_tools', 'brave_core_checkout']

PROPERTIES = InputProperties


def RunSteps(api: RecipeScriptApi, properties: InputProperties) -> None:
    brave_core_root = api.brave_core_checkout.deploy('tools/cr/toolchains')

    vpython3 = api.depot_tools.vpython3()
    # `--clear` wipes any prior output so every run starts from a clean out dir.
    cmd = [
        vpython3,
        brave_core_root / 'tools/cr/toolchains/build_xcode_toolchain.py',
        '--out-dir',
        api.path.out,
        '--chromium-tag',
        properties.chromium_tag,
        '--clear',
    ]
    if properties.force_overwrite:
        cmd.append('--force-overwrite')
    api.step('build xcode toolchain', cmd)


def GenTests(api):
    # Happy path: deploy the build scripts on a mac host, then build.
    # `deployed` seeds brave_core_checkout's post-checkout path check.
    yield api.test(
        'mac',
        api.platform.name('mac'),
        api.brave_core_checkout.deployed('tools/cr/toolchains'),
        api.properties(chromium_tag='150.0.7841.1'),
        api.post_process(post_process.MustRun, 'build xcode toolchain'),
        api.post_process(post_process.StepCommandContains,
                         'build xcode toolchain',
                         ['--chromium-tag', '150.0.7841.1']),
        api.post_process(post_process.StepCommandContains,
                         'build xcode toolchain', ['--clear']),
        api.post_process(post_process.StatusSuccess),
    )
    # `force_overwrite` appends `--force-overwrite` to the build command.
    yield api.test(
        'force overwrite',
        api.platform.name('mac'),
        api.brave_core_checkout.deployed('tools/cr/toolchains'),
        api.properties(chromium_tag='150.0.7841.1', force_overwrite=True),
        api.post_process(post_process.StepCommandContains,
                         'build xcode toolchain', ['--force-overwrite']),
        api.post_process(post_process.StatusSuccess),
        api.post_process(post_process.DropExpectation),
    )
