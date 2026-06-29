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

Resolution is by path layout: a brave checkout always lives at
`<workspace>/src/brave`, so the launcher finds the checkout governing the cwd by
walking up to the first `src/brave` directory (see `find_brave_checkout`). This
single rule serves every cwd-relative tool — `brockit` / `plaster` and
`node` / `npm` alike — with no git involved, so it works from inside the
checkout, from nested DEPS repos, and from above `src/brave`. The cwd is never
changed, so the tool runs relative to the current path, exactly as if it had
been launched as `tools/cr/<tool>.py` from there.

This runs under plain `python3` (not `vpython3`), so it must stay stdlib-only,
and it should be kept self-contained from other python code.
"""

from __future__ import annotations

import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

# The conventional location of a brave checkout under a workspace root. Every
# cwd-relative tool resolves against this layout (see `find_brave_checkout`)
# rather than git, so resolution works identically from inside the checkout,
# from nested DEPS repos, and from above `src/brave`.
_SRC_BRAVE = Path('src') / 'brave'

# node/npm shims come in a family (`node`, `npm`) with one entry per platform,
# keyed `<family>-<platform>` (see SHIM_TARGETS). A shim that knows its platform
# passes the qualified name (the `.bat` variants pass `node-win` / `npm-win`);
# the POSIX shims pass the bare family and the launcher appends the host key.
BINARY_FAMILIES: tuple[str, ...] = ('node', 'npm')

# Every shim's target path, relative to the workspace root, and the single
# source of truth for what each shim runs. Each path is explicit about its
# origin: brave-owned targets go through `_SRC_BRAVE` (`src/brave/…`), so a
# chromium-owned script could be added on the same base as `src/…`.
#   - `brockit` / `plaster`        a tools/cr script, run via vpython3 from the
#                                  checkout containing the cwd,
#   - `fetch_brave` (SELF_TOOLS)   a script run via the system python from the
#                                  bootstrap's *own* checkout,
#   - `node-<plat>` / `npm-<plat>` the downloaded node/npm delivered by `gclient
#                                  sync` (see download_node.py). `npm-*` points
#                                  straight at `npm-cli.js`, which the launcher
#                                  runs with the matching `node-*` so npm needs
#                                  no shebang or `.cmd` shell wrapper.
SHIM_TARGETS: dict[str, Path] = {
    'brockit': _SRC_BRAVE / 'tools' / 'cr' / 'brockit.py',
    'plaster': _SRC_BRAVE / 'tools' / 'cr' / 'plaster.py',
    'fetch_brave': _SRC_BRAVE / 'tools' / 'cr' / 'fetch_brave.py',
    'node-linux': _SRC_BRAVE / 'third_party' / 'node' / 'linux' /
    'node-linux-x64' / 'bin' / 'node',
    'node-mac': _SRC_BRAVE / 'third_party' / 'node' / 'mac' /
    'node-darwin-x64' / 'bin' / 'node',
    'node-mac_arm64': _SRC_BRAVE / 'third_party' / 'node' / 'mac_arm64' /
    'node-darwin-arm64' / 'bin' / 'node',
    'node-win': _SRC_BRAVE / 'third_party' / 'node' / 'win' / 'node-win-x64' /
    'node.exe',
    'npm-linux': _SRC_BRAVE / 'third_party' / 'node' / 'linux' /
    'node-linux-x64' / 'lib' / 'node_modules' / 'npm' / 'bin' / 'npm-cli.js',
    'npm-mac': _SRC_BRAVE / 'third_party' / 'node' / 'mac' /
    'node-darwin-x64' / 'lib' / 'node_modules' / 'npm' / 'bin' / 'npm-cli.js',
    'npm-mac_arm64': _SRC_BRAVE / 'third_party' / 'node' / 'mac_arm64' /
    'node-darwin-arm64' / 'lib' / 'node_modules' / 'npm' / 'bin' /
    'npm-cli.js',
    'npm-win': _SRC_BRAVE / 'third_party' / 'node' / 'win' / 'node-win-x64' /
    'node_modules' / 'npm' / 'bin' / 'npm-cli.js',
}

# Tools that run from the bootstrap's *own* checkout (resolved from this
# launcher's location, not the cwd) under the system interpreter. fetch_brave
# bootstraps a brand-new checkout into an empty directory: there is no checkout
# in the cwd to resolve against, and no vpython3 yet, so it must come from the
# installed bootstrap and run under plain python3.
SELF_TOOLS: frozenset[str] = frozenset({'fetch_brave'})


def find_brave_checkout(start: Path | None = None) -> Path | None:
    """Resolves the `src/brave` checkout governing `start` via the path layout.

    A brave checkout always lives at `<workspace>/src/brave`. Starting at
    `start` (the cwd by default) and walking up, this returns the first
    `<ancestor>/src/brave` that carries the bootstrap sentinel. It uses no git,
    so a single rule covers every cwd-relative tool:

    - inside the checkout (`…/src/brave/components`),
    - inside a nested DEPS git repo (`…/src/brave/vendor/web-discovery-project`,
      where `git rev-parse` would report the nested repo, not brave),
    - and *above* `src/brave` — e.g. after `npm run init` runs
      `cd ../../ && npm run --prefix src/brave …`, leaving the cwd at the
      workspace root, with `src/brave` as a child.

    Returns None when no such checkout is found, so callers can report the
    error (brockit/plaster) or fall back to the system tool (node/npm).
    """
    start = (start or Path.cwd()).resolve()
    sentinel = _SRC_BRAVE / 'tools' / 'cr' / 'bootstrap' / 'launcher.py'
    for directory in (start, *start.parents):
        if (directory / sentinel).is_file():
            return directory / _SRC_BRAVE
    return None


def host_platform_key() -> str | None:
    """The SHIM_TARGETS platform suffix for the host (e.g. `linux`, `win`).

    Used to qualify the bare `node` / `npm` a POSIX shim passes, since one POSIX
    shim serves Linux and both macOS variants. Returns None on an unsupported
    host.
    """
    system = platform.system()
    if system == 'Linux':
        return 'linux'
    if system == 'Darwin':
        is_arm = platform.machine().lower() in ('arm64', 'aarch64')
        return 'mac_arm64' if is_arm else 'mac'
    if system == 'Windows':
        return 'win'
    return None


def split_binary(tool: str) -> tuple[str, str | None] | None:
    """Parses a node/npm shim token into `(family, platform_key)`.

    Accepts a platform-qualified name from a shim that knows its platform
    (`node-win` from the `.bat`) or the bare family from a POSIX shim (`node`),
    which the launcher qualifies with the host key. Returns None for non-binary
    tools (brockit/plaster/fetch_brave).
    """
    for family in BINARY_FAMILIES:
        if tool == family:
            return family, host_platform_key()
        if tool.startswith(family + '-'):
            return family, tool[len(family) + 1:]
    return None


def resolve_repo_binary(family: str, root: Path,
                        key: str | None) -> list[str] | None:
    """The invocation for a node/npm `family` from the checkout's node, or None.

    Looks up `node-<key>` (and `npm-<key>`) in SHIM_TARGETS under the workspace
    `root`. Returns a full argv prefix: npm is launched as the platform's node
    running `npm-cli.js`, which avoids both the `#!/usr/bin/env node` shebang
    (POSIX) and invoking `npm.cmd` through a shell (Windows). Returns None when
    there is no such downloaded node, so the caller can fall back to the system
    tool.
    """
    node_rel = SHIM_TARGETS.get(f'node-{key}')
    if node_rel is None:
        return None
    node = root / node_rel
    if not node.is_file():
        return None
    if family == 'node':
        return [str(node)]
    npm_rel = SHIM_TARGETS.get(f'npm-{key}')
    if npm_rel is None:
        return None
    return [str(node), str(root / npm_rel)]


def resolve_system_binary(tool: str,
                          exclude_dir: Path | None = None) -> str | None:
    """Locates `tool` on `$PATH`, excluding the shim directory.

    The bootstrap directory is first on `$PATH`, so a plain `shutil.which(tool)`
    would resolve back to our own shim and recurse forever. We drop that
    directory (this file's directory by default) from the search path first.
    """
    here = (exclude_dir or Path(__file__).parent).resolve()
    entries = [
        entry for entry in os.environ.get('PATH', '').split(os.pathsep)
        if entry and Path(entry).resolve() != here
    ]
    return shutil.which(tool, path=os.pathsep.join(entries))


def _resolve_vpython3(src: Path) -> Path:
    """Returns the `vpython3` interpreter to run the tool with.

    Precedence mirrors `vpython_utils._compute_vpython3_path`: prefer a
    `vpython3` already on `$PATH`, otherwise fall back to the Chromium-bundled
    `<src>/third_party/depot_tools/vpython3`.
    """
    found = shutil.which('vpython3')
    if found is not None:
        return Path(found)
    name = 'vpython3.bat' if platform.system() == 'Windows' else 'vpython3'
    return src / 'third_party' / 'depot_tools' / name


def _run_binary_tool(family: str, key: str | None, args: list[str]) -> int:
    """Dispatches a node/npm `family` (a `<family>-<platform>` SHIM_TARGETS).

    Prefers the checkout's downloaded node when the cwd resolves to a
    `src/brave` checkout and that node exists, else falls back to the system.
    Unlike the python tools this never fails for being outside a checkout — the
    whole point is to be a transparent node/npm front-end.
    """
    checkout = find_brave_checkout()
    invocation = None
    if checkout is not None:
        # SHIM_TARGETS resolve from the workspace root (the parent of `src`).
        invocation = resolve_repo_binary(family, checkout.parent.parent, key)
    if invocation is None:
        system = resolve_system_binary(family)
        if system is None:
            sys.stderr.write(
                f'{family}: no checkout-local node found and no {family} on '
                f'$PATH.\n')
            return 1
        invocation = [system]
    # Intentionally do not change cwd: the tool must run relative to the
    # current path.
    return subprocess.call([*invocation, *args])


def _run_self_tool(tool: str, args: list[str]) -> int:
    """Dispatches a self-tool from the bootstrap's own checkout (SELF_TOOLS).

    Resolves the script relative to this launcher file (not the cwd or git) and
    runs it with the same interpreter that is running the launcher, since
    fetch_brave operates in an empty directory before vpython3 exists. The cwd
    is left untouched, so the tool sets up the checkout under the current path.
    """
    # SHIM_TARGETS is relative to the workspace root. This launcher lives at
    # <root>/src/brave/tools/cr/bootstrap/launcher.py, so parents[5] == <root>.
    script = Path(__file__).resolve().parents[5] / SHIM_TARGETS[tool]
    if not script.is_file():
        sys.stderr.write(f'{tool}: cannot find {script}\n')
        return 1
    return subprocess.call([sys.executable, str(script), *args])


def _run_vpython_tool(tool: str, relpath: Path, args: list[str]) -> int:
    """Dispatches a `tools/cr` python script via vpython3 from the cwd checkout.

    The checkout is resolved from the cwd (see `find_brave_checkout`); these
    tools must be run from inside a checkout. SHIM_TARGETS is relative to the
    workspace root (the checkout's grandparent), while vpython3 lives under the
    `src` root (the checkout's parent).
    """
    checkout = find_brave_checkout()
    if checkout is None:
        sys.stderr.write(f'{tool}: must be run inside a brave checkout '
                         f'(no src/brave found for the current directory).\n')
        return 1
    src = checkout.parent
    script = src.parent / relpath
    if not script.is_file():
        sys.stderr.write(
            f'{tool}: {script} not found in the resolved checkout.\n')
        return 1
    # Intentionally do not change cwd: the tool must run relative to the
    # current path.
    return subprocess.call([str(_resolve_vpython3(src)), str(script), *args])


def main() -> int:
    if len(sys.argv) < 2:
        sys.stderr.write(
            'launcher.py: missing tool name (the shim target to run)\n')
        return 2
    tool, args = sys.argv[1], sys.argv[2:]

    if tool in SELF_TOOLS:
        return _run_self_tool(tool, args)

    binary = split_binary(tool)
    if binary is not None:  # node / npm (bare family or `<family>-<platform>`)
        return _run_binary_tool(binary[0], binary[1], args)

    target = SHIM_TARGETS.get(tool)
    if target is None:
        known = ', '.join(sorted(SHIM_TARGETS))
        sys.stderr.write(f"launcher.py: unknown tool '{tool}' (known: "
                         f'{known})\n')
        return 2
    return _run_vpython_tool(tool, target, args)


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
