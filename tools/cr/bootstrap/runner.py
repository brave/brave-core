#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Prints the `vpython3` the shims should run `launcher.py` with.

The shims call this script first, and then run `launcher.py` with what it
prints, so no process is left waiting on the launcher:

    exec "$(python3 runner.py)" launcher.py brockit lift --to=1.2.3.4

A `vpython3` already on `$PATH` is used as is. Otherwise it is the one vendored
in the governing checkout (see `launcher._resolve_checkout`), or, outside any
checkout, in the checkout these shims live in. That `vendor/depot_tools` is
deployed first if missing, as a shared clone of the `$GIT_CACHE_PATH` mirror
when there is one, or as a normal clone otherwise. If no `vpython3` can be
provided, nothing is printed and this exits non-zero.

Only the interpreter path goes to stdout, as the shims capture it.

This runs under plain `python3`, so it must stay stdlib-only.
"""

from __future__ import annotations

import os
from pathlib import Path
import platform
import shutil
import stat
import subprocess
import sys

import launcher

DEPOT_TOOLS_URL = (
    'https://chromium.googlesource.com/chromium/tools/depot_tools.git')

# The hardcoded name for the depot_tools mirror directory within the git cache.
_DEPOT_TOOLS_MIRROR_DIR = 'chromium.googlesource.com-chromium-tools-depot_tools'

# Present while a deploy of depot_tools is underway, or after one was
# interrupted. It is the same guard `pnpm run init` uses (see `depotTools.js`),
# and both replace a checkout they find guarded with a fresh clone.
_GUARD_NAME = 'depot_tools_install.guard'

VPYTHON3 = 'vpython3.bat' if platform.system() == 'Windows' else 'vpython3'

# The checkout these shims live in: `<checkout>/tools/cr/bootstrap/runner.py`.
_OWN_CHECKOUT = Path(__file__).resolve().parents[3]


def _rmtree(path: Path) -> None:
    """Removes `path`, clearing the read-only bit git sets on packs (Windows).
    """

    def _make_writable_and_retry(func, target, _exc_info):
        os.chmod(target, stat.S_IWRITE)
        func(target)

    # `onerror` is deprecated from 3.12, and this runs under any `python3`.
    if sys.version_info >= (3, 12):
        shutil.rmtree(path, onexc=_make_writable_and_retry)  # pylint: disable=unexpected-keyword-arg
    else:
        shutil.rmtree(path, onerror=_make_writable_and_retry)


def _run(*cmd: str | Path) -> None:
    # Output goes to stderr, so it never mixes with the tool's stdout.
    subprocess.check_call([str(arg) for arg in cmd], stdout=sys.stderr)


def _depot_tools_mirror() -> Path | None:
    """The depot_tools mirror in `$GIT_CACHE_PATH`, or None if there is none."""
    cache_path = os.environ.get('GIT_CACHE_PATH')
    if not cache_path:
        return None
    mirror = Path(cache_path).expanduser() / _DEPOT_TOOLS_MIRROR_DIR
    return mirror if (mirror / 'config').is_file() else None


def _clone_depot_tools(dest: Path) -> None:
    """Clones depot_tools into `dest`, from git cache if possible."""
    mirror = _depot_tools_mirror()
    if mirror is None:
        _run('git', 'clone', DEPOT_TOOLS_URL, dest)
        return
    _run('git', 'clone', '--shared', mirror, dest)
    # Point at the real remote, so depot_tools self-updates from it.
    _run('git', '-C', dest, 'remote', 'set-url', 'origin', DEPOT_TOOLS_URL)


def ensure_depot_tools(checkout: Path) -> Path | None:
    """The checkout's `vendor/depot_tools`, deployed if missing.

    Guarded as in `pnpm run init`: a checkout left behind by a failed or
    interrupted deploy is removed and cloned afresh. Returns None if
    depot_tools cannot be made usable.
    """
    dest = checkout / 'vendor' / 'depot_tools'
    guard = dest.parent / _GUARD_NAME
    interrupted = guard.exists()
    if not interrupted and (dest / VPYTHON3).is_file():
        return dest
    if not interrupted and dest.exists():
        sys.stderr.write(f'runner.py: {dest} has no {VPYTHON3}.\n')
        return None

    dest.parent.mkdir(parents=True, exist_ok=True)
    guard.write_bytes(b'runner.py\n')
    try:
        if interrupted and dest.exists():
            sys.stderr.write(
                f'runner.py: removing the interrupted deploy at {dest}\n')
            _rmtree(dest)
        sys.stderr.write(f'runner.py: deploying depot_tools to {dest}\n')
        _clone_depot_tools(dest)
    except (OSError, subprocess.CalledProcessError) as error:
        # The guard stays behind, marking `dest` for repair.
        sys.stderr.write(f'runner.py: could not deploy depot_tools: {error}\n')
        return None
    guard.unlink()
    return dest


def main() -> int:
    on_path = shutil.which('vpython3')
    if on_path:
        print(on_path)
        return 0
    checkout = launcher._resolve_checkout() or _OWN_CHECKOUT  # pylint: disable=protected-access
    depot_tools = ensure_depot_tools(checkout)
    if depot_tools is None:
        return 1
    print(depot_tools / VPYTHON3)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
