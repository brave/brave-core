#!/usr/bin/env vpython3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Download a Node.js distribution (including npm) for the host platform.

This is the prototype counterpart to Chromium's `third_party/node`: instead of
pulling a stripped, npm-less tarball from GCS via a `gcs` DEPS entry, this hook
fetches the official Node.js distribution straight from `nodejs.org/dist` and
keeps npm/npx intact, so the repository can ship a self-contained node + npm.

It mirrors the layout of Chromium's `update_node_binaries`: the archive is
verified against the published `SHASUMS256.txt`, extracted, and the
version-stamped top-level directory (`node-v24.16.0-linux-x64`) is renamed to a
version-independent one (`node-linux-x64`) so the shims in
`tools/cr/bootstrap` can resolve a stable path. npm is deliberately *not*
removed (Chromium strips it; we want it).

The download is idempotent: a `.brave_node_version` stamp in the extracted
directory short-circuits re-runs for the same version, so `gclient sync` is
cheap on the common path.

Stdlib-only so it can run as a DEPS hook under `vpython3` without any prior
setup.
"""

from __future__ import annotations

import argparse
import hashlib
import platform
import shutil
import sys
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path

# Keep in sync with brave/package.json `engines` (node 24.16.0 bundles npm
# 11.13.0, satisfying both pins). This is intentionally ahead of Chromium's
# `third_party/node` pin, which lags and ships no npm.
NODE_VERSION = 'v24.16.0'
BASE_URL = 'https://nodejs.org/dist'

# This file lives in `src/brave/third_party/node`; everything is extracted
# alongside it.
NODE_DIR = Path(__file__).resolve().parent

# Name of the per-directory stamp recording the version currently extracted.
STAMP_NAME = '.brave_node_version'


class Platform:
    """A host platform Node.js publishes binaries for.

    `subdir` is the directory under `third_party/node` (matching Chromium's
    `linux` / `mac` / `mac_arm64` / `win` scheme), `suffix` is the Node.js
    release suffix, and `archive_ext` is the archive format for that platform.
    """

    def __init__(self, subdir: str, suffix: str, archive_ext: str):
        self.subdir = subdir
        self.suffix = suffix
        self.archive_ext = archive_ext

    @property
    def archive_name(self) -> str:
        return f'node-{NODE_VERSION}-{self.suffix}.{self.archive_ext}'

    @property
    def versioned_root(self) -> str:
        """Top-level directory name inside the archive."""
        return f'node-{NODE_VERSION}-{self.suffix}'

    @property
    def stable_root(self) -> str:
        """Version-independent directory name the shims resolve against."""
        return f'node-{self.suffix}'

    @property
    def stable_dir(self) -> Path:
        """Absolute path of the extracted, version-stripped node directory."""
        return NODE_DIR / self.subdir / self.stable_root

    @property
    def is_windows(self) -> bool:
        return self.archive_ext == 'zip'

    @property
    def bin_dir(self) -> Path:
        """Directory holding the `node`/`npm` executables.

        On Windows they sit at the distribution root (`node.exe`, `npm.cmd`);
        on POSIX they live under `bin/`.
        """
        return self.stable_dir if self.is_windows else self.stable_dir / 'bin'

    @property
    def node_exe(self) -> Path:
        """Absolute path of the `node` executable."""
        return self.bin_dir / ('node.exe' if self.is_windows else 'node')

    @property
    def npm_cli(self) -> Path:
        """Absolute path of npm's `npm-cli.js`.

        Running npm as `node npm-cli.js` is portable: it sidesteps the
        `#!/usr/bin/env node` shebang on POSIX and the need to invoke `npm.cmd`
        through a shell on Windows. The bundled npm lives under the
        distribution's `node_modules` (`lib/node_modules` on POSIX).
        """
        base = self.stable_dir if self.is_windows else self.stable_dir / 'lib'
        return base / 'node_modules' / 'npm' / 'bin' / 'npm-cli.js'


def detect_platform() -> Platform:
    """Returns the `Platform` for the host, or exits if it is unsupported."""
    system = platform.system()
    machine = platform.machine().lower()
    is_arm = machine in ('arm64', 'aarch64')
    if system == 'Linux':
        return Platform('linux', 'linux-x64', 'tar.gz')
    if system == 'Darwin':
        if is_arm:
            return Platform('mac_arm64', 'darwin-arm64', 'tar.gz')
        return Platform('mac', 'darwin-x64', 'tar.gz')
    if system == 'Windows':
        return Platform('win', 'win-x64', 'zip')
    sys.exit(f'download_node: unsupported host platform {system}/{machine}')


def expected_sha256(plat: Platform) -> str:
    """Fetches and parses the published checksum for the platform archive."""
    url = f'{BASE_URL}/{NODE_VERSION}/SHASUMS256.txt'
    with urllib.request.urlopen(url) as response:  # nosec B310 (https only)
        text = response.read().decode('utf-8')
    for line in text.splitlines():
        digest, _, name = line.partition(' ')
        if name.strip() == plat.archive_name:
            return digest.strip()
    sys.exit(f'download_node: {plat.archive_name} not found in SHASUMS256.txt')


def download(url: str, dest: Path) -> None:
    print(f'download_node: fetching {url}')
    with urllib.request.urlopen(url) as response:  # nosec B310 (https only)
        dest.write_bytes(response.read())


def verify_sha256(archive: Path, expected: str) -> None:
    actual = hashlib.sha256(archive.read_bytes()).hexdigest()
    if actual != expected:
        sys.exit(f'download_node: SHA256 mismatch for {archive.name}\n'
                 f'  expected {expected}\n  actual   {actual}')


def extract(archive: Path, into: Path) -> None:
    if archive.suffix == '.zip':
        with zipfile.ZipFile(archive) as zf:
            zf.extractall(into)
    else:
        with tarfile.open(archive) as tf:
            # `filter='data'` (Python 3.12+) blocks path traversal / unsafe
            # members; fall back gracefully on older interpreters.
            try:
                tf.extractall(into, filter='data')
            except TypeError:
                tf.extractall(into)


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Download a Node.js distribution (including npm) for the '
        'host platform.')
    parser.add_argument(
        '--print',
        dest='print_path',
        choices=('bin-dir', 'node', 'npm-cli'),
        help='Print an absolute path for the host platform and exit (does not '
        'download): the node/npm bin directory, the node executable, or npm\'s '
        'npm-cli.js. Lets callers locate the toolchain without duplicating the '
        'platform layout.')
    args = parser.parse_args()

    plat = detect_platform()

    if args.print_path:
        print({
            'bin-dir': plat.bin_dir,
            'node': plat.node_exe,
            'npm-cli': plat.npm_cli,
        }[args.print_path])
        return 0

    target_dir = NODE_DIR / plat.subdir
    stable_dir = plat.stable_dir
    stamp = stable_dir / STAMP_NAME

    if stamp.is_file() and stamp.read_text(encoding='utf-8').strip() == \
            NODE_VERSION:
        print(f'download_node: {plat.stable_root} already at {NODE_VERSION}')
        return 0

    target_dir.mkdir(parents=True, exist_ok=True)
    expected = expected_sha256(plat)

    with tempfile.TemporaryDirectory(dir=target_dir) as tmp:
        tmp_dir = Path(tmp)
        archive = tmp_dir / plat.archive_name
        download(f'{BASE_URL}/{NODE_VERSION}/{plat.archive_name}', archive)
        verify_sha256(archive, expected)
        extract(archive, tmp_dir)

        extracted = tmp_dir / plat.versioned_root
        if not extracted.is_dir():
            sys.exit(f'download_node: expected {plat.versioned_root} in '
                     f'{plat.archive_name}')
        (extracted / STAMP_NAME).write_text(NODE_VERSION, encoding='utf-8')

        # Swap the freshly extracted tree into place atomically-ish: drop any
        # previous version first, then move the version-stripped directory in.
        if stable_dir.exists():
            shutil.rmtree(stable_dir)
        extracted.rename(stable_dir)

    print(f'download_node: installed {NODE_VERSION} at {stable_dir}')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
