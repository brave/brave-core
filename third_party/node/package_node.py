#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Package the version-free Node directories deployed by `download_node.py`.

For each platform, this packs the *contents* of the version-free `node-<suffix>`
directory (its `bin/`, `lib/`, ... entries) into a single gzipped tarball, so
extracting it does not recreate the `node-<suffix>/` wrapper directory:

    third_party/node/node-linux-x64/{bin,lib,...}
        ->  node-v24.17.0-linux-x64.tar.gz  (members: bin/, lib/, ...)
"""

from __future__ import annotations

import argparse
import hashlib
import sys
import tarfile
from pathlib import Path

# Import the shared version/platform definitions from the sibling downloader so
# the Node version lives in exactly one place.
sys.path.insert(0, str(Path(__file__).resolve().parent))

# pylint: disable=wrong-import-position
from download_node import NODE_VERSION, PLATFORMS

# This directory: third_party/node.
_NODE_DIR = Path(__file__).resolve().parent


def package(tarball_name: str, deployed_dir: str,
            output_dir: Path) -> Path | None:
    """Pack the *contents* of `deployed_dir` into `tarball_name`.

    Returns the tarball path, or None when the platform has not been deployed
    yet (run `download_node.py` first).
    """
    dest_dir = _NODE_DIR / deployed_dir
    if not dest_dir.is_dir():
        print(f'Skipping {tarball_name}: {dest_dir} does not exist '
              f'(run download_node.py first).')
        return None

    tarball = output_dir / tarball_name
    # Archive the contents of the deployed dir (bin/, lib/, ...) at the archive
    # root, so extracting the tarball does not recreate the node-<suffix>/
    # wrapper directory.
    with tarfile.open(tarball, mode='w:gz') as tar:
        for child in sorted(dest_dir.iterdir()):
            tar.add(child, arcname=child.name)
    return tarball


def sha256_and_size(path: Path) -> tuple[str, int]:
    """Return the (sha256, byte size) of `path`."""
    digest = hashlib.sha256()
    size = 0
    with path.open('rb') as file:
        while chunk := file.read(1 << 16):
            digest.update(chunk)
            size += len(chunk)
    return digest.hexdigest(), size


def main() -> int:
    parser = argparse.ArgumentParser(
        description=f'Package Node {NODE_VERSION} directories into '
        'version-free tarballs for upload.')
    parser.add_argument(
        '--output-dir',
        type=Path,
        default=_NODE_DIR,
        help='Directory to write the tarballs into (defaults to this '
        'directory).')
    args = parser.parse_args()

    args.output_dir.mkdir(parents=True, exist_ok=True)

    results: list[tuple[str, str, int]] = []
    for _archive, tarball_name, deployed_dir in PLATFORMS:
        tarball = package(tarball_name, deployed_dir, args.output_dir)
        if tarball is None:
            continue
        sha256, size = sha256_and_size(tarball)
        print(f'Packaged {tarball.name}')
        results.append((tarball.name, sha256, size))

    if results:
        # Echo the values needed to update the EXTRA_DEPS entry after upload.
        print('\nObject details for install_extra_deps.py:')
        for name, sha256, size in results:
            print(f"  object_name: '{name}'")
            print(f"  sha256sum:   '{sha256}'  ({size} bytes)")
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
