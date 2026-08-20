# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Build a hermetic, reproducible Windows toolchain archive for a Chromium tag.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

import post_process
from PB.recipes.brave.toolchains.windows.build_windows_toolchain import (
    InputProperties)

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = ['path', 'step', 'platform', 'depot_tools', 'brave_core_checkout']

PROPERTIES = InputProperties


def RunSteps(api: RecipeScriptApi, properties: InputProperties) -> None:
    brave_core_root = api.brave_core_checkout.deploy('tools/cr/toolchains')

    vpython3 = api.depot_tools.vpython3()
    # `--clear` wipes any prior output so every run starts from a clean out
    # dir. `--upload` publishes the archive + index to the internal build-deps
    # bucket, replacing the native `s3Upload` pipeline step the Jenkins job
    # used to perform this same upload with.
    cmd = [
        vpython3,
        brave_core_root / 'tools/cr/toolchains/build_windows_toolchain.py',
        '--out-dir',
        api.path.out,
        '--chromium-tag',
        properties.chromium_ref,
        '--clear',
        '--upload',
    ]
    api.step('build windows toolchain', cmd)


def GenTests(api):
    # Happy path: deploy the build scripts on a windows host, then build.
    # `deployed` seeds brave_core_checkout's post-checkout path check.
    yield api.test(
        'win',
        api.platform.name('win'),
        api.brave_core_checkout.deployed('tools/cr/toolchains'),
        api.properties(chromium_ref='150.0.7841.1'),
        api.post_process(post_process.MustRun, 'build windows toolchain'),
        api.post_process(post_process.StepCommandContains,
                         'build windows toolchain',
                         ['--chromium-tag', '150.0.7841.1']),
        api.post_process(post_process.StepCommandContains,
                         'build windows toolchain', ['--clear']),
        api.post_process(post_process.StepCommandContains,
                         'build windows toolchain', ['--upload']),
        api.post_process(post_process.StatusSuccess),
    )
