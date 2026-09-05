# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Fetches `brave-core`, using git cache if possible.

To start out a workspace, just run:

  curl -fsSL https://raw.githubusercontent.com/brave/brave-core/master/tools/cr/bootstrap/fetch_brave.py | python3 -
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

# Default remotes for the two repos this script bootstraps.
BRAVE_CORE_URL = 'https://github.com/brave/brave-core.git'
DEPOT_TOOLS_URL = (
    'https://chromium.googlesource.com/chromium/tools/depot_tools.git')

# Template for the bootstrap `.gclient` to have `depot_tools` clone `brave-core`
# into `src/brave`, which permits git cache to be reused if present.
_GCLIENT_TEMPLATE = """\
solutions = [
  {
    "managed": False,
    "name": "src/brave",
    "url": %(url)s
  }
]
target_os = []
target_cpu = []
"""

# The git cache mirror path for depot_tools. We hardcode it because we cannot
# rely on `git cache exists` to check for it when deploy depot_tools.
_DEPOT_TOOLS_MIRROR_DIR = 'chromium.googlesource.com-chromium-tools-depot_tools'

# The path to bootstrap, to be added to `PATH`. This path is relative from the
# checkout root, as this script can be used standalone, so we don't rely on a
# relative position from `__file__`.
_BOOTSTRAP_RELATIVE_DIR = Path('src') / 'brave' / 'tools' / 'cr' / 'bootstrap'

GCLIENT_NAME = 'gclient.bat' if platform.system() == 'Windows' else 'gclient'


def prepend_to_path(path: str, *dirs: Path) -> str:
    """`path` with each of `dirs` prepended to it.
    """
    new_entries = [str(d) for d in dirs]
    if not path:
        return os.pathsep.join(new_entries)
    return os.pathsep.join([*new_entries, path])


def ensure_depot_tools(brave_core_dir: Path, cache_dir: Path | None) -> Path:
    """Clones depot_tools into `brave_core_dir/vendor/depot_tools`.

    This function attempts to clone depot_tools first from git cache, if a path
    is provided, and if that fails, it falls back to cloning it from the remote.
    """
    dest = brave_core_dir / 'vendor' / 'depot_tools'
    if (dest / '.git').exists():
        return dest
    dest.parent.mkdir(parents=True, exist_ok=True)
    cache_repo = cache_dir / _DEPOT_TOOLS_MIRROR_DIR if cache_dir else None
    if cache_repo is not None and not (cache_repo / 'config').is_file():
        cache_repo = None
    if cache_repo is None:
        subprocess.check_call(['git', 'clone', DEPOT_TOOLS_URL, str(dest)])
        return dest
    subprocess.check_call([
        'git', 'clone', '--no-checkout', '--local', '--shared',
        str(cache_repo),
        str(dest)
    ])
    subprocess.check_call(['git', 'checkout', '--force', 'HEAD'], cwd=dest)

    # Setting the remote URL to the official repo.
    subprocess.check_call(
        ['git', 'remote', 'set-url', 'origin', DEPOT_TOOLS_URL], cwd=dest)
    return dest


def fetch_brave_core(depot_tools_dir: Path, root: Path,
                     ref: str | None) -> None:
    """Clones brave-core into `root/src/brave` via `gclient sync`.

    This function writes a `.gclient` file in order to get `gclient sync` to
    clone brave-core into `src/brave`, which allows the use of git cache, if
    available, for `brave-core`.
    """
    gclient_text = _GCLIENT_TEMPLATE % {'url': json.dumps(BRAVE_CORE_URL)}
    # Using `write_bytes` for consistent line endings across platforms. Using
    # `newline='\n'` with `write_text` can run into issues with certain Python
    # versions, and this script has to run with the system Python.
    (root / '.gclient').write_bytes(gclient_text.encode('utf-8'))
    sync_args = ['sync', '--nohooks']
    if ref:
        sync_args += ['--revision', f'src/brave@{ref}']

    subprocess.check_call([str(depot_tools_dir / GCLIENT_NAME), *sync_args],
                          cwd=root)


def run_pnpm(brave_core_dir: Path, pnpm_args: list[str]) -> None:
    """Runs `pnpm <pnpm_args>` from `brave_core_dir`, via the bootstrap shim
    on `$PATH` (see `main()`).
    """
    subprocess.check_call(['pnpm', *pnpm_args], cwd=brave_core_dir)


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Bootstrap a fresh brave-core + Chromium checkout in the '
        'current directory.')
    parser.add_argument(
        '--ref',
        default=None,
        help='brave-core branch/tag to check out (default: remote HEAD)')
    parser.add_argument(
        '--no-sync',
        action='store_true',
        help='clone brave-core (and vendor depot_tools) but skip `pnpm '
        'install`/`pnpm run init` -- which is what brings in Chromium')
    args = parser.parse_args()

    root = Path.cwd()
    brave_core_dir = root / 'src' / 'brave'
    if (brave_core_dir / '.git').exists():
        # Fetch should be used exclusively from an empty directory to start a
        # new workspace.
        sys.stderr.write(
            f'{brave_core_dir} already looks like a checkout.\n'
            'Please run this script from an empty directory. Aborting.\n')
        return 1

    configured_cache_dir = os.environ.get('GIT_CACHE_PATH')
    cache_dir = (Path(configured_cache_dir).expanduser()
                 if configured_cache_dir else None)

    depot_tools_dir = ensure_depot_tools(brave_core_dir, cache_dir)

    # We add depot_tools and bootstrap to path for the duration of this script.
    bootstrap_dir = root / _BOOTSTRAP_RELATIVE_DIR
    os.environ['PATH'] = prepend_to_path(os.environ.get('PATH', ''),
                                         depot_tools_dir, bootstrap_dir)

    fetch_brave_core(depot_tools_dir, root, args.ref)

    if args.no_sync:
        return 0

    run_pnpm(brave_core_dir, ['run', 'init'])
    return 0


if __name__ == '__main__':
    sys.exit(main())
