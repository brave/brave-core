# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Download and package Node into version-free tarballs for the build-deps
bucket.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

import post_process

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = ['path', 'step', 'depot_tools', 'brave_core_checkout']


def RunSteps(api: RecipeScriptApi) -> None:
    brave_root = api.brave_core_checkout.deploy('third_party/node')

    vpython3 = api.depot_tools.vpython3()
    node_dir = brave_root / 'third_party/node'
    # `--clear` wipes any prior node-* deployment so every run starts clean.
    api.step('download node', [
        vpython3,
        node_dir / 'download_node.py',
        '--clear',
    ])
    api.step('package node', [
        vpython3,
        node_dir / 'package_node.py',
        '--output-dir',
        api.path.out,
    ])


def GenTests(api):
    # brave-core is deployed (sparse), then node is downloaded and packaged.
    # `deployed(...)` seeds the sparse path so the existence check passes.
    yield api.test(
        'basic',
        api.brave_core_checkout.deployed('third_party/node'),
        api.post_process(post_process.MustRun,
                         'clone brave-core (shallow, sparse)'),
        api.post_process(post_process.MustRun, 'download node'),
        api.post_process(post_process.MustRun, 'package node'),
        api.post_process(post_process.StatusSuccess),
    )
    # An existing brave-core checkout is fetched/updated rather than re-cloned.
    yield api.test(
        'reuse checkout',
        api.brave_core_checkout.existing_checkout(),
        api.brave_core_checkout.deployed('third_party/node'),
        api.post_process(post_process.MustRun, 'fetch brave-core ref'),
        api.post_process(post_process.DoesNotRun,
                         'clone brave-core (shallow, sparse)'),
        api.post_process(post_process.MustRun, 'package node'),
        api.post_process(post_process.StatusSuccess),
    )
