# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Download and package Node into version-free tarballs for the build-deps
bucket.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = ['path', 'step', 'depot_tools', 'brave_core_shallow']


def RunSteps(api: RecipeScriptApi, _: object) -> None:
    brave_root = api.brave_core_shallow.deploy('third_party/node')

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
