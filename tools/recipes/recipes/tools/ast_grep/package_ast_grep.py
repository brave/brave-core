# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from __future__ import annotations

from typing import TYPE_CHECKING

import post_process
from PB.recipes.brave.tools.ast_grep.package_ast_grep import (EnvProperties,
                                                              InputProperties)

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = [
    'path', 'step', 'depot_tools', 'chromium_checkout', 'brave_core_checkout'
]

PROPERTIES = InputProperties
ENV_PROPERTIES = EnvProperties


def RunSteps(api: RecipeScriptApi, properties: InputProperties,
             env_properties: EnvProperties) -> None:
    api.chromium_checkout.ensure_checkout(ref=properties.chromium_ref,
                                          git_cache=env_properties.GIT_CACHE
                                          or None)

    brave_root = api.brave_core_checkout.deploy([
        'third_party/ast-grep',
        'tools/cr/toolchains',
    ])

    vpython3 = api.depot_tools.vpython3()
    api.step('package ast-grep', [
        vpython3,
        brave_root / 'third_party/ast-grep/package_ast_grep.py',
        '--clean',
        '--out-dir',
        api.path.out,
        '--upload',
    ])


def GenTests(api):
    yield api.test(
        'linux',
        api.chromium_checkout.with_git_cache(),
        api.brave_core_checkout.deployed('third_party/ast-grep',
                                         'tools/cr/toolchains'),
        api.properties(chromium_ref='151.0.7917.1'),
        api.post_process(post_process.MustRun, 'fetch chromium'),
        api.post_process(post_process.MustRun, 'package ast-grep'),
        api.post_process(post_process.StatusSuccess),
    )
