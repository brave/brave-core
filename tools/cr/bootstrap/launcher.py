#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Shared launcher behind the shims for our tooling

This launcher provides a mechanism similar to how `depot_tools` routes `gn`
call relative to the `gn` binary for the repository in the current directory.
The main role of this script is path resolution. After that, all arguments are
passed through verbatim:

    python3 launcher.py brockit lift --to=1.2.3.4

Resolution follows the same invariant as `tools/cr/repository.py`: the work
tree containing the cwd must be a `brave` checkout. The cwd is never changed,
so the tool runs relative to the current path, exactly as if it had been
launched as `tools/cr/<tool>.py` from there.

This runs under plain `python3` (not `vpython3`), so it must stay stdlib-only,
and it should be kept self-contained from other python code.
"""

from __future__ import annotations

from pathlib import Path
import platform
import shutil
import subprocess
import sys

# The basename a brave-core work-tree root must have. Kept in sync with
# `repository.BRAVE_DIR_NAME`.
BRAVE_DIR_NAME = 'brave'

# The tools this launcher knows how to dispatch, mapped to their script paths
# relative to the brave-core root.
TOOL_PATHS: dict[str, Path] = {
    'brockit': Path('tools') / 'cr' / 'brockit.py',
    'plaster': Path('tools') / 'cr' / 'plaster.py',
}


def find_brave_core(relpath: Path) -> Path | None:
    """Locates the brave-core work tree containing the current directory.

    `relpath` is the tool's script path relative to the brave-core root (a
    value from `TOOL_PATHS`). Returns the resolved root when the cwd sits in a
    `brave` work tree that actually ships that script, else None. Uses
    `git rev-parse --show-toplevel`, so it honours the cwd (and any git work
    tree / submodule boundaries) the same way the rest of tools/cr does.
    """
    try:
        result = subprocess.run(['git', 'rev-parse', '--show-toplevel'],
                                capture_output=True,
                                text=True,
                                check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None
    toplevel = result.stdout.strip()
    if not toplevel:
        return None
    root = Path(toplevel)
    if root.name != BRAVE_DIR_NAME:
        return None
    if not (root / relpath).is_file():
        return None
    return root


def _resolve_vpython3(brave: Path) -> Path:
    """Returns the `vpython3` interpreter to run the tool with.

    Precedence mirrors `vpython_utils._compute_vpython3_path`: prefer a
    `vpython3` already on `$PATH`, otherwise fall back to the Chromium-bundled
    `third_party/depot_tools/vpython3` next to this checkout.
    """
    found = shutil.which('vpython3')
    if found is not None:
        return Path(found)
    name = 'vpython3.bat' if platform.system() == 'Windows' else 'vpython3'
    return brave.parent / 'third_party' / 'depot_tools' / name


def main() -> int:
    if len(sys.argv) < 2:
        sys.stderr.write(
            'launcher.py: missing tool name (the tools/cr script to run)\n')
        return 2
    tool, args = sys.argv[1], sys.argv[2:]

    relpath = TOOL_PATHS.get(tool)
    if relpath is None:
        known = ', '.join(sorted(TOOL_PATHS))
        sys.stderr.write(f"launcher.py: unknown tool '{tool}' (known: "
                         f'{known})\n')
        return 2

    brave = find_brave_core(relpath)
    if brave is None:
        sys.stderr.write(
            f'{tool}: must be run inside a brave-core git work tree '
            f'(could not locate {relpath.as_posix()} for the current '
            f'directory).\n')
        return 1

    # Intentionally do not change cwd: the tool must run relative to the
    # current path.
    invocation = [str(_resolve_vpython3(brave)), str(brave / relpath), *args]
    return subprocess.call(invocation)


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
