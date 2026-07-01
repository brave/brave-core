#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Download upstream Node distributions into version-free platform directories.
"""

from __future__ import annotations

import argparse
import hashlib
import shutil
import ssl
import sys
import tarfile
import tempfile
import urllib.error
import urllib.parse
import urllib.request
import zipfile
from pathlib import Path

# The Node version to download. Bump this to roll Node, then re-run this script
# followed by `package_node.py`. It is the single source of truth for both.
NODE_VERSION = 'v24.17.0'

# Where the official archives and their checksums are published.
BASE_URL = 'https://nodejs.org/dist'

# This directory: third_party/node.
_NODE_DIR = Path(__file__).resolve().parent

# Verify TLS against the system trust store with hostname checking.
_TLS_CONTEXT = ssl.create_default_context()

PLATFORMS: dict[str, str] = {
    f'node-{NODE_VERSION}-linux-x64.tar.gz': f'node-{NODE_VERSION}-linux-x64.tar.gz',
    f'node-{NODE_VERSION}-darwin-x64.tar.gz': f'node-{NODE_VERSION}-mac-x64.tar.gz',
    f'node-{NODE_VERSION}-darwin-arm64.tar.gz': f'node-{NODE_VERSION}-mac-arm64.tar.gz',
    f'node-{NODE_VERSION}-win-x64.zip': f'node-{NODE_VERSION}-win-x64.tar.gz',
}


class _HttpsOnlyRedirectHandler(urllib.request.HTTPRedirectHandler):
    """Refuse any redirect that would leave HTTPS (no https->http downgrade)."""

    def redirect_request(self, req, fp, code, msg, headers, newurl):
        if urllib.parse.urlparse(newurl).scheme != 'https':
            raise urllib.error.URLError(
                f'refusing to follow non-HTTPS redirect to {newurl!r}')
        return super().redirect_request(req, fp, code, msg, headers, newurl)


# An opener that only ever speaks HTTPS with a verified certificate: no plain
# HTTP handler is registered, so http:// URLs (direct or via redirect) error out
# instead of being fetched in the clear.
_OPENER = urllib.request.build_opener(
    urllib.request.HTTPSHandler(context=_TLS_CONTEXT),
    _HttpsOnlyRedirectHandler)


def urlopen(url: str):
    """Open `url`, requiring HTTPS with a verified certificate handshake.

    Rejects non-HTTPS URLs up front and never downgrades across redirects, so
    every byte is fetched over a TLS connection whose certificate chain and
    hostname have been validated.
    """
    if urllib.parse.urlparse(url).scheme != 'https':
        raise ValueError(f'refusing to fetch non-HTTPS URL: {url!r}')
    return _OPENER.open(url)


def _strip_archive_ext(name: str) -> str:
    """`name` without its `.tar.gz`/`.zip` archive extension."""
    return name.removesuffix('.tar.gz').removesuffix('.zip')


def deployed_dir(tarball: str) -> str:
    """The version-free directory a tarball packages (and we deploy into).

    Drops the extension and the version, e.g.
    `node-v24.17.0-mac-x64.tar.gz` -> `node-mac-x64`.
    """
    return _strip_archive_ext(tarball).replace(f'-{NODE_VERSION}', '')


def fetch_shasums() -> dict[str, str]:
    """Return a {archive_name: sha256} map from the version's SHASUMS256.txt."""
    url = f'{BASE_URL}/{NODE_VERSION}/SHASUMS256.txt'
    with urlopen(url) as response:
        text = response.read().decode('utf-8')
    sums: dict[str, str] = {}
    for line in text.splitlines():
        parts = line.split()
        # Lines are '<sha256>  <name>'; entries for full archives carry a bare
        # filename (per-binary lines like 'win-x64/node.exe' are ignored here).
        if len(parts) == 2:
            sha256, name = parts
            sums[name] = sha256
    return sums


def download(url: str, dest: Path) -> str:
    """Stream `url` to `dest`, returning the sha256 of the bytes written."""
    digest = hashlib.sha256()
    with urlopen(url) as response, dest.open('wb') as out:
        while chunk := response.read(1 << 16):
            out.write(chunk)
            digest.update(chunk)
    return digest.hexdigest()


def extract(archive: Path, dest: Path) -> None:
    """Extract a tar or zip `archive` into `dest`."""
    if zipfile.is_zipfile(archive):
        with zipfile.ZipFile(archive) as zip_file:
            zip_file.extractall(path=dest)
        return
    with tarfile.open(archive, mode='r:*') as tar:
        tar.extractall(path=dest, filter='data')


def deploy(archive: str, tarball: str, shasums: dict[str, str]) -> None:
    """Download, verify and deploy one platform's Node into its dir.

    Any previous deployment is wiped so no stale tree lingers, then the upstream
    archive is extracted and its versioned top-level directory renamed to the
    version-free directory `tarball` packages.
    """
    expected = shasums.get(archive)
    if expected is None:
        raise RuntimeError(
            f'{archive} is not listed in SHASUMS256.txt for {NODE_VERSION}')

    # The archive's top-level dir is its name without the extension, e.g.
    # node-v24.17.0-darwin-x64; we rename it to the version-free deployed dir.
    versioned = _NODE_DIR / _strip_archive_ext(archive)
    dest = _NODE_DIR / deployed_dir(tarball)

    url = f'{BASE_URL}/{NODE_VERSION}/{archive}'
    print(f'Downloading {url}')
    with tempfile.TemporaryDirectory() as tmp:
        archive_path = Path(tmp) / archive
        actual = download(url, archive_path)
        if actual != expected:
            raise RuntimeError(f'SHA256 mismatch for {archive}:\n'
                               f'  expected {expected}\n'
                               f'  actual   {actual}')

        # Clear any prior deployment and any half-extracted versioned tree, then
        # extract into this directory and strip the version from the top-level.
        for stale in (dest, versioned):
            if stale.exists():
                shutil.rmtree(stale)
        extract(archive_path, _NODE_DIR)
        versioned.rename(dest)

    print(f'Deployed {archive} -> {dest.relative_to(_NODE_DIR.parent)}')


def clear_existing() -> None:
    """Remove every existing `node-*` path under this directory.
    """
    for path in sorted(_NODE_DIR.glob('node-*')):
        print(f'Removing {path.relative_to(_NODE_DIR.parent)}')
        if path.is_dir() and not path.is_symlink():
            shutil.rmtree(path)
        else:
            path.unlink()


def main() -> int:
    parser = argparse.ArgumentParser(
        description=f'Download Node {NODE_VERSION} into version-free '
        'directories under third_party/node.')
    parser.add_argument(
        '--clear',
        action='store_true',
        help='Delete any existing node-* paths under third_party/node first.')
    args = parser.parse_args()

    if args.clear:
        clear_existing()

    shasums = fetch_shasums()
    for archive, tarball in PLATFORMS.items():
        deploy(archive, tarball, shasums)

    print('\nDone. Run package_node.py to package these for upload.')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
