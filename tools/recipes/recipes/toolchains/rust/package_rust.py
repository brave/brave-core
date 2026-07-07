# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING

import post_process
from recipe_properties import Property

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = [
    'path', 'step', 'depot_tools', 'chromium_checkout', 'brave_core_shallow'
]

@dataclass(frozen=True)
class InputProperties:
    brave_subrevision: int
    chromium_ref: str
    git_cache: str | None = Property(default=None, from_environ='GIT_CACHE')


PROPERTIES = InputProperties


def RunSteps(api: RecipeScriptApi, properties: InputProperties) -> None:
    chromium_src = api.chromium_checkout.ensure_checkout(
        ref=properties.chromium_ref, git_cache=properties.git_cache)

    brave_core_root = api.brave_core_shallow.deploy('tools/cr/toolchains')

    vpython3 = api.depot_tools.vpython3()
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
    ])


def GenTests(api):
    # Happy path: checkout (with a seeded git cache), deploy the build scripts,
    # then build. `with_git_cache`/`deployed` seed chromium_checkout's and
    # brave_core_shallow's preconditions.
    yield api.test(
        'linux',
        api.chromium_checkout.with_git_cache(),
        api.brave_core_shallow.deployed('tools/cr/toolchains'),
        api.properties(brave_subrevision=1, chromium_ref='151.0.7917.1'),
        api.post_process(post_process.MustRun, 'fetch chromium'),
        api.post_process(post_process.MustRun, 'fetch tag'),
        api.post_process(post_process.MustRun, 'build rust toolchain'),
        api.post_process(post_process.StepCommandContains,
                         'build rust toolchain', ['--brave-subrevision', '1']),
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
