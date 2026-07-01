# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Download and package Node into version-free tarballs for the build-deps
bucket.

Runs `third_party/node/download_node.py` (which fetches the official Node
archives from nodejs.org and lays them out version-free) followed by
`package_node.py` (which tars each into `node-<version>-<suffix>.tar.gz` under
the job's output dir).

Unlike the ast-grep/rust recipes, this needs neither a Chromium checkout nor
depot_tools/vpython3: the scripts are stdlib-only `python3` and download
prebuilt binaries, so all this recipe needs is the scripts themselves (a sparse
brave-core checkout) and the interpreter already running the engine.
"""

from __future__ import annotations

import sys
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = ['path', 'step', 'brave_core_shallow']


def RunSteps(api: RecipeScriptApi, _: object) -> None:
    brave_root = api.brave_core_shallow.deploy(api.path.brave_core,
                                               'third_party/node')

    node_dir = brave_root / 'third_party/node'
    # `--clear` wipes any prior node-* deployment so every run starts clean.
    api.step('download node', [
        sys.executable,
        node_dir / 'download_node.py',
        '--clear',
    ])
    api.step('package node', [
        sys.executable,
        node_dir / 'package_node.py',
        '--output-dir',
        api.path.out,
    ])
