#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Self-contained launcher for the Brave recipes engine.

Lets a pipeline run a recipe straight from GitHub without a brave-core checkout:

    curl -sL https://raw.githubusercontent.com/brave/brave-core/refs/heads/\\
master/tools/recipes/engine_bootstrap.py \\
        | python3 - toolchains/rust/package_rust \\
            --properties '{"chromium_src": "~/dev/brave-next/src/",
                           "brave_subrevision": 1,
                           "chromium_ref": "151.0.7917.1"}'

That is the bootstrapping equivalent to running, from a checkout:

    python3 tools/recipes/engine.py \\
        toolchains/rust/package_rust --properties ...

The bootstrap has to deploy a sparse checkout of brave-core's `tools/recipes/`
to get the engine, and it has to deploy depot_tools if `vpython3` is not already
on PATH, so it can run the engine.
"""

from __future__ import annotations

import argparse
import logging
import os
from pathlib import Path
import shutil
import subprocess
import sys

# brave-core remote + branch the engine code is fetched from. SSH, matching the
# `brave_core_shallow` recipe module.
REPO_URL = 'git@github.com:brave/brave-core.git'
BRAVE_CORE_REF = 'master'

# The relative path to the recipes engine in the bootstrapped checkout for it.
RECIPES_PATH = 'tools/recipes'

# The path relative to workspace where the shallow `brave-core` clone for the
# bootstrap is deployed. This checkout is used exclusively for the engine.
RECIPES_ENGINE_DEST = 'recipes_engine_bootstrap'

# the vpython3 runtime.
IS_WIN = sys.platform.startswith(('win', 'cygwin'))
VPYTHON3 = 'vpython3' + ('.bat' if IS_WIN else '')
VPYTHON_SPEC = '.vpython3'

# The URL to clone depot_tools from if vpython3 is not found.
DEPOT_TOOLS_URL = 'https://chromium.googlesource.com/chromium/tools/depot_tools'
DEPOT_TOOLS_DEST = 'depot_tools_bootstrap'


def _run(*cmd: str | Path, cwd: str | Path | None = None) -> None:
    """Run *cmd*, logging the invocation, and raise on a non-zero exit."""
    logging.info(' >>>> %s', ' '.join(str(arg) for arg in cmd))
    subprocess.run([str(arg) for arg in cmd], cwd=cwd, check=True)


def _deploy_recipes(dest: str | Path) -> Path:
    """Shallow-, sparse-clone brave-core's `tools/recipes/` into *dest*.

    *dest* is always wiped first so every run starts from a clean checkout --
    no stale sparse state, and no partial clone left by a failed prior run.

    Returns the path to the checked-out `engine.py`.
    """
    dest = Path(dest).expanduser().resolve()

    if dest.exists():
        logging.info('Removing existing checkout at %s', dest)
        shutil.rmtree(dest)

    dest.parent.mkdir(parents=True, exist_ok=True)
    _run('git', 'clone', '--depth', '2', '--filter=blob:none', '--sparse',
         '--branch', BRAVE_CORE_REF, REPO_URL, dest)
    _run('git', '-C', dest, 'sparse-checkout', 'add', RECIPES_PATH)

    engine = dest / RECIPES_PATH / 'engine.py'
    if not engine.is_file():
        raise RuntimeError(f'engine not found after checkout: {engine}')
    spec = engine.parent / VPYTHON_SPEC
    if not spec.is_file():
        raise RuntimeError(f'vpython spec not found after checkout: {spec}')
    return engine


def _deploy_depot_tools(dest: str | Path) -> Path:
    """Shallow-clone depot_tools into *dest* and return its directory.

    An existing checkout that already has a `vpython3` is reused if found,
    otherwise a fresh shallow clone is made.
    """
    dest = Path(dest).expanduser().resolve()
    vpython3 = dest / VPYTHON3

    if vpython3.is_file():
        logging.info('Reusing existing depot_tools at %s', dest)
        return dest

    if dest.exists():
        logging.info('Removing incomplete depot_tools at %s', dest)
        shutil.rmtree(dest)

    dest.parent.mkdir(parents=True, exist_ok=True)
    _run('git', 'clone', '--depth', '1', DEPOT_TOOLS_URL, dest)
    if not vpython3.is_file():
        raise RuntimeError(
            f'vpython3 not found after depot_tools clone: {vpython3}')
    return dest


def _ensure_vpython3(depot_tools_dest: str | Path) -> str:
    """Return a runnable `vpython3`, deploying depot_tools if none is found.

    If no `vpython3` is found on PATH, a shallow clone of depot_tools is
    and that clone is added to PATH.
    """
    if shutil.which(VPYTHON3):
        return VPYTHON3

    logging.info('%s not on PATH; deploying depot_tools', VPYTHON3)
    depot_tools = _deploy_depot_tools(depot_tools_dest)
    os.environ['PATH'] = os.pathsep.join(
        [str(depot_tools), os.environ.get('PATH', '')])
    return str(depot_tools / VPYTHON3)


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)

    parser = argparse.ArgumentParser(
        description='Bootstrap and run a Brave recipe from a shallow '
        'brave-core checkout.')
    parser.add_argument('recipe',
                        help='Recipe name (e.g. toolchains/rust/package_rust)')
    parser.add_argument(
        '--workspace',
        default=None,
        help='Root directory the job runs in (forwarded to '
        'engine.py); the bootstrap checkouts are deployed here')
    parser.add_argument('--verbose', action='store_true')
    args, _ = parser.parse_known_args(argv)

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    # All boostrap checkouts are placed under workspace.
    workspace = (Path(args.workspace).expanduser()
                 if args.workspace else Path.cwd())
    vpython3 = _ensure_vpython3(workspace / DEPOT_TOOLS_DEST)
    engine = _deploy_recipes(workspace / RECIPES_ENGINE_DEST)

    spec = engine.parent / VPYTHON_SPEC
    forwarded = [
        vpython3, '-vpython-spec',
        str(spec), '-u',
        str(engine), *argv
    ]
    logging.info('Launching engine: %s', ' '.join(forwarded))
    return subprocess.run(forwarded, check=False).returncode


if __name__ == '__main__':
    sys.exit(main())
