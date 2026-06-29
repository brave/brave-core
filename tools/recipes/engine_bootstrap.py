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

That is equivalent to running, from a checkout:

    python3 tools/recipes/engine.py \\
        toolchains/rust/package_rust --properties ...

It shallow-, sparse-clones brave-core's `tools/recipes/` tree into the recipe's
`brave_core_dest` (default `brave_core`), then re-dispatches the original
arguments to the checked-out `engine.py`. Because it clones into the *same*
destination the `brave_core_shallow` recipe module uses, the recipe reuses this
checkout (adding its own paths to it) instead of cloning brave-core a second
time.

This script is intentionally dependency-free (standard library only) so it can
run via `curl ... | python3 -` on a bare machine.
"""

from __future__ import annotations

import argparse
import json
import logging
from pathlib import Path
import subprocess
import sys

# brave-core remote + branch the engine code is fetched from. SSH, matching the
# `brave_core_shallow` recipe module.
REPO_URL = 'git@github.com:brave/brave-core.git'
BRAVE_CORE_REF = 'master'

# Repo-relative path of the recipes engine tree, and the default checkout dir.
# The default MUST match `recipes/toolchains/rust/package_rust.py`'s
# `brave_core_dest` default (and the `brave_core_shallow` module) so the recipe
# reuses this same checkout.
RECIPES_PATH = 'tools/recipes'
DEFAULT_BRAVE_CORE_DEST = 'brave_core'


def _run(*cmd: str | Path, cwd: str | Path | None = None) -> None:
    """Run *cmd*, logging the invocation, and raise on a non-zero exit."""
    logging.info(' >>>> %s', ' '.join(str(arg) for arg in cmd))
    subprocess.run([str(arg) for arg in cmd], cwd=cwd, check=True)


def _brave_core_dest(properties_json: str) -> str:
    """Read `brave_core_dest` from the --properties JSON, or the default.

    The destination must agree with the recipe so both share one checkout. A
    missing key or unparseable JSON falls back to the default; the engine
    validates the properties for real once it runs.
    """
    try:
        properties = json.loads(properties_json)
    except json.JSONDecodeError:
        return DEFAULT_BRAVE_CORE_DEST
    if isinstance(properties, dict):
        return properties.get('brave_core_dest') or DEFAULT_BRAVE_CORE_DEST
    return DEFAULT_BRAVE_CORE_DEST


def _deploy_recipes(dest: str | Path) -> Path:
    """Shallow-, sparse-clone brave-core's `tools/recipes/` into *dest*.

    Mirrors the `brave_core_shallow` recipe module (which it cannot import yet,
    since nothing is checked out): clone when *dest* is not a checkout, then
    `sparse-checkout add` the engine tree. `add` (not `set`) leaves the recipe
    free to add its own paths (e.g. `tools/cr/toolchains`) to this checkout.

    Returns the path to the checked-out `engine.py`.
    """
    dest = Path(dest).expanduser().resolve()

    if (dest / '.git').is_dir():
        logging.info('brave-core checkout already present at %s', dest)
    else:
        dest.parent.mkdir(parents=True, exist_ok=True)
        _run('git', 'clone', '--depth', '2', '--filter=blob:none', '--sparse',
             '--branch', BRAVE_CORE_REF, REPO_URL, dest)

    _run('git', '-C', dest, 'sparse-checkout', 'add', RECIPES_PATH)

    engine = dest / RECIPES_PATH / 'engine.py'
    if not engine.is_file():
        raise RuntimeError(f'engine not found after checkout: {engine}')
    return engine


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)

    # Peek at --properties to find the checkout destination, but forward the
    # arguments to engine.py verbatim so the engine remains the source of truth
    # for parsing and validation.
    parser = argparse.ArgumentParser(
        description='Bootstrap and run a Brave recipe from a shallow '
        'brave-core checkout.')
    parser.add_argument('recipe',
                        help='Recipe name (e.g. toolchains/rust/package_rust)')
    parser.add_argument('--properties',
                        default='{}',
                        help='JSON object of recipe properties')
    parser.add_argument('--verbose', action='store_true')
    args, _ = parser.parse_known_args(argv)

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)

    engine = _deploy_recipes(_brave_core_dest(args.properties))

    forwarded = [sys.executable, str(engine), *argv]
    logging.info('Launching engine: %s', ' '.join(forwarded))
    return subprocess.run(forwarded, check=False).returncode


if __name__ == '__main__':
    sys.exit(main())
