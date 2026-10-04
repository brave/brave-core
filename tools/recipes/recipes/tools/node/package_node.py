# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Download and package Node into version-free tarballs for the build-deps
bucket.
"""

from __future__ import annotations

from dataclasses import dataclass

import post_process
from recipe_api import RecipeScriptApi
from recipe_modules import (
    brave_core_checkout,
    depot_tools,
    path,
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


def RunSteps(api: DEPS) -> None:
    brave_root = api.brave_core_checkout.deploy(
        [
            'third_party/node',
            'tools/cr',
        ]
    )

    vpython3 = api.depot_tools.vpython3()
    node_dir = brave_root / 'third_party/node'
    # `--clear` wipes any prior node-* deployment so every run starts clean.
    api.step(
        'download node',
        [
            vpython3,
            node_dir / 'download_node.py',
            '--clear',
        ],
    )
    api.step(
        'package node',
        [
            vpython3,
            node_dir / 'package_node.py',
            '--output-dir',
            api.path.out,
            '--upload',
        ],
    )


def GenTests(api: TEST_DEPS):
    # brave-core is deployed (sparse), then node is downloaded and packaged.
    # `deployed(...)` seeds the sparse path so the existence check passes.
    yield api.test(
        'basic',
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.deployed('third_party/node', 'tools/cr'),
        api.post_process(
            post_process.MustRun, 'clone brave-core (shallow, sparse)'
        ),
        api.post_process(post_process.MustRun, 'download node'),
        api.post_process(post_process.MustRun, 'package node'),
        api.post_process(post_process.StatusSuccess),
    )
    # An existing brave-core checkout is fetched/updated rather than re-cloned.
    yield api.test(
        'reuse checkout',
        api.brave_core_checkout.existing_checkout(),
        api.brave_core_checkout.with_git_cache(),
        api.brave_core_checkout.deployed('third_party/node', 'tools/cr'),
        api.post_process(post_process.MustRun, 'fetch brave-core ref'),
        api.post_process(
            post_process.DoesNotRun, 'clone brave-core (shallow, sparse)'
        ),
        api.post_process(post_process.MustRun, 'package node'),
        api.post_process(post_process.StatusSuccess),
    )
