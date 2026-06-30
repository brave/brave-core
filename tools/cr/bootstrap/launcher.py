#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Shared launcher behind the shims for our tooling.

This launcher provides a mechanism similar to how `depot_tools` routes `gn`
call relative to the `gn` binary for the repository in the current directory.
The main role of this script is path resolution. After that, all arguments are
passed through verbatim:

    python3 launcher.py brockit lift --to=1.2.3.4

This runs under plain `python3` (not `vpython3`), so it must stay stdlib-only,
and it should be kept self-contained from other python code.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

# A brave checkout's path relative to the workspace root.
_SRC_BRAVE = Path('src') / 'brave'


@dataclass(frozen=True)
class Shim:
    """A shim target listed in SHIM_TARGETS."""

    # The workspace-root-relative path the shim runs.
    path: str

    # How the target is run:
    #   'vpython' — a vpython3 script
    #   'node'    — a script run with `node` from `$PATH`
    #   None      — `path` is itself the executable (node)
    runtime: str | None = None


# SHIM_TARGETS maps the shims we have with their desired targets. The keys are
# the names of the shims. Platform-specific suffixes are used for shims that
# have different paths in different platforms.
SHIM_TARGETS: dict[str, Shim] = {
    'brockit': Shim('src/brave/tools/cr/brockit.py', 'vpython'),
    'plaster': Shim('src/brave/tools/cr/plaster.py', 'vpython'),
    'node-linux': Shim(
        'src/brave/third_party/node/node-v24.17.0-linux-x64/bin/node'),
    'node-mac': Shim(
        'src/brave/third_party/node/node-v24.17.0-darwin-x64/bin/node'),
    'node-mac_arm64': Shim(
        'src/brave/third_party/node/node-v24.17.0-darwin-arm64/bin/node'),
    'node-win': Shim(
        'src/brave/third_party/node/node-v24.17.0-win-x64/node.exe'),
    'npm-linux': Shim(
        'src/brave/third_party/node/node-v24.17.0-linux-x64/lib/node_modules/npm/bin/npm-cli.js',
        'node'),
    'npm-mac': Shim(
        'src/brave/third_party/node/node-v24.17.0-darwin-x64/lib/node_modules/npm/bin/npm-cli.js',
        'node'),
    'npm-mac_arm64': Shim(
        'src/brave/third_party/node/node-v24.17.0-darwin-arm64/lib/node_modules/npm/bin/npm-cli.js',
        'node'),
    'npm-win': Shim(
        'src/brave/third_party/node/node-v24.17.0-win-x64/node_modules/npm/bin/npm-cli.js',
        'node'),
}


def find_brave_checkout(start: Path) -> Path | None:
    """The `src/brave` checkout at or above `start`, or None.

    By layout, not git: the first ancestor `src/brave` carrying our sentinel.
    """
    start = start.resolve()
    sentinel = _SRC_BRAVE / 'tools' / 'cr' / 'bootstrap' / 'launcher.py'
    for directory in (start, *start.parents):
        if (directory / sentinel).is_file():
            return directory / _SRC_BRAVE
    return None


def _find_cwd_checkout() -> Path | None:
    """The `brave-core` checkout governing the current working directory."""
    return find_brave_checkout(Path.cwd())


def host_platform_key() -> str | None:
    """The SHIM_TARGETS platform suffix for the host, or None if unsupported."""
    system = platform.system()
    if system == 'Linux':
        return 'linux'
    if system == 'Darwin':
        is_arm = platform.machine().lower() in ('arm64', 'aarch64')
        return 'mac_arm64' if is_arm else 'mac'
    if system == 'Windows':
        return 'win'
    return None


class UnknownShimError(Exception):
    """Raised when a shim token resolves to no `SHIM_TARGETS` entry."""


def find_shim_target(tool: str) -> Shim:
    """The SHIM_TARGETS entry for `tool`, or raise UnknownShimError.

    Matches the token directly, else with the host suffix appended (`node` ->
    `node-linux`).
    """
    if tool in SHIM_TARGETS:
        return SHIM_TARGETS[tool]
    host = host_platform_key()
    if host is not None and f'{tool}-{host}' in SHIM_TARGETS:
        return SHIM_TARGETS[f'{tool}-{host}']
    raise UnknownShimError(tool)


def _resolve_vpython3(src: Path) -> Path:
    """The `vpython3` to run tools with: one on `$PATH`, else the depot_tools
    copy under `src` (mirrors `vpython_utils._compute_vpython3_path`)."""
    found = shutil.which('vpython3')
    if found is not None:
        return Path(found)
    name = 'vpython3.bat' if platform.system() == 'Windows' else 'vpython3'
    return src / 'third_party' / 'depot_tools' / name


def _resolve_system_binary(tool: str,
                           exclude_dir: Path | None = None) -> str | None:
    """Locate `tool` on `$PATH`, minus the shim dir.

    The shim dir is first on `$PATH`, so a plain `which` would find our own shim
    and recurse; drop it (this file's dir by default) before searching.
    """
    here = (exclude_dir or Path(__file__).parent).resolve()
    entries = [
        entry for entry in os.environ.get('PATH', '').split(os.pathsep)
        if entry and Path(entry).resolve() != here
    ]
    return shutil.which(tool, path=os.pathsep.join(entries))


def resolve_invocation(tool: str, checkout: Path | None,
                       allow_fallback: bool) -> list[str] | None:
    """The argv prefix to run `tool` from `checkout`.

    This function attempts to resolve the invocation for a given tool. Tools
    are usually run from checkout, but certain tools are allowed to fallback to
    system binaries if nothing is found in checkout, when `allow_fallback` is
    True.
    """
    shim = find_shim_target(tool)
    invocation = None
    if checkout is not None:
        target = checkout.parent.parent / shim.path
        if target.is_file():
            if shim.runtime == 'vpython':
                invocation = [
                    str(_resolve_vpython3(checkout.parent)),
                    str(target)
                ]
            elif shim.runtime == 'node':
                # Run with whatever `node` is on $PATH (our node shim, which
                # resolves the right node in turn).
                node = shutil.which('node')
                if node is not None:
                    invocation = [node, str(target)]
            else:
                invocation = [str(target)]

    if invocation is None and allow_fallback:
        system = _resolve_system_binary(tool.split('-', 1)[0])
        if system is not None:
            invocation = [system]
    return invocation


def main() -> int:
    """Resolve the tool from argv and run it, or report why it cannot."""
    parser = argparse.ArgumentParser(
        prog='launcher.py',
        description='Run a brave tool shim against the checkout in the cwd.',
        epilog='Arguments after TOOL are forwarded to it verbatim.',
        allow_abbrev=False)
    parser.add_argument(
        '--allow-fallback',
        action='store_true',
        help='Allows falling back to a binary in %PATH if outside a checkout.')
    parser.add_argument('tool',
                        metavar='TOOL',
                        help='shim to run (e.g. brockit, plaster, node, npm)')
    parsed, tool_args = parser.parse_known_args()
    tool = parsed.tool

    checkout = _find_cwd_checkout()
    try:
        invocation = resolve_invocation(tool, checkout, parsed.allow_fallback)
    except UnknownShimError:
        sys.stderr.write(f"launcher.py: unknown tool '{tool}'\n")
        return 2

    if invocation is not None:
        # Intentionally do not change cwd: the tool runs relative to the current
        # path.
        return subprocess.call([*invocation, *tool_args])

    # Resolved a known tool but produced nothing to run — report why.
    if parsed.allow_fallback:
        family = tool.split('-', 1)[0]
        sys.stderr.write(f'{family}: no checkout-local binary found and no '
                         f'{family} on $PATH.\n')
        return 1
    if checkout is None:
        sys.stderr.write(f'{tool}: must be run inside a brave checkout '
                         f'(no src/brave found for the current directory).\n')
    else:
        sys.stderr.write(
            f'{tool}: target not found in the resolved checkout.\n')
    return 2


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
