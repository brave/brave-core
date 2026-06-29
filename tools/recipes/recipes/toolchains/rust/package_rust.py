# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Recipe: build and package Brave's Rust toolchain overlay.

Wires together the recipe modules to drive
`tools/cr/toolchains/build_rust_toolchain.py`:

  1. `chromium_checkout` prepares the Chromium `src/` tree -- cloning when
     asked and checking out the requested ref/tag. The ref is handled here, NOT
     by passing `--use-ref` to the build script, so the script always operates
     on an already-prepared checkout.
  2. `brave_core_shallow` sparse-deploys `tools/cr/toolchains/` from brave-core
     so the build script is available without a full Brave checkout.
  3. The build script is run with `--no-full-toolchain` to produce the minimal
     rust-lld + wasm32 overlay archive.

Run it via the engine (from tools/recipes/):

    python3 engine.py toolchains/rust/package_rust \\
        --properties '{"chromium_src": "~/dev/chromium/src",
                       "brave_subrevision": 1,
                       "chromium_ref": "refs/tags/150.0.7850.1"}'
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from engine import RecipeScriptApi

DEPS = ['step', 'depot_tools', 'chromium_checkout', 'brave_core_shallow']

# Repo-relative path of the toolchain scripts inside brave-core, and the build
# entry point within it.
BRAVE_TOOLCHAINS_PATH = 'tools/cr/toolchains'
BUILD_SCRIPT = 'build_rust_toolchain.py'

# brave-core branch the build script is always fetched from.
BRAVE_CORE_REF = 'master'


@dataclass(frozen=True)
class InputProperties:
    """Inputs for the package_rust recipe.

    Attributes:
        chromium_src: Path to the Chromium `src/` directory.
        brave_subrevision: Integer respin counter passed as
            `--brave-subrevision` to the build script.
        chromium_ref: Git ref/tag for the Chromium checkout. Applied by
            `chromium_checkout`, not by the build script.
        out_dir: Output directory passed as `--out-dir` to the build script.
        brave_core_dest: Directory the shallow brave-core checkout is deployed
            to (the build script is run from within it).
        git_cache: Sets `GIT_CACHE_PATH`. A shared git cache is required: if
            None, `GIT_CACHE_PATH` must already be set in the environment or the
            run aborts; an empty string sets it to the default `<home>/cache`;
            any other value sets it to that explicit path. Mirrors
            `--with-git-cache`.
    """

    chromium_src: str
    brave_subrevision: int
    chromium_ref: str
    out_dir: str = 'out'
    brave_core_dest: str = 'brave_core'
    git_cache: str | None = None


PROPERTIES = InputProperties


def RunSteps(api: RecipeScriptApi, properties: InputProperties) -> None:
    # A shared git cache is mandatory: ensure_checkout aborts if GIT_CACHE_PATH
    # is not set. Seed it from the provided value when given; otherwise it must
    # already be present in the environment.
    if properties.git_cache is not None:
        api.chromium_checkout.set_git_cache(properties.git_cache or None)

    # Prepare the Chromium checkout and pin it to the requested ref. The build
    # script is deliberately not told about the ref (no --use-ref); it just
    # consumes the tree we hand it.
    chromium_src = api.chromium_checkout.ensure_checkout(
        properties.chromium_src, ref=properties.chromium_ref)

    # Sparse-deploy the brave-core toolchain scripts. deploy() returns the
    # brave-core root with the subtree at its original repo-relative path.
    brave_core_root = api.brave_core_shallow.deploy(properties.brave_core_dest,
                                                    BRAVE_TOOLCHAINS_PATH,
                                                    ref=BRAVE_CORE_REF)
    toolchains_dir = brave_core_root / BRAVE_TOOLCHAINS_PATH

    # build_rust_toolchain.py carries a VPYTHON spec (it needs the pyyaml
    # wheel), so it must run under depot_tools' vpython3.
    vpython3 = api.depot_tools.vpython3(chromium_src)

    # No cwd override: the script resolves --out-dir against the current
    # working directory, so `out` lands relative to where the recipe is run.
    api.step('build rust toolchain', [
        vpython3,
        toolchains_dir / BUILD_SCRIPT,
        '--out-dir',
        properties.out_dir,
        '--chromium-src',
        chromium_src,
        '--brave-subrevision',
        str(properties.brave_subrevision),
        '--clear',
        '--no-full-toolchain',
    ])
