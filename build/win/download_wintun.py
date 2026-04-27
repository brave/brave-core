#!/usr/bin/env vpython3

# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Downloads and stages the Wintun prebuilt binaries for Brave VPN on Windows.

Invoked from DEPS as a hook on Windows checkouts. Downloads the archive from
DEPS_PACKAGES_URL, verifies its SHA-256, replaces the destination directory
with a fresh extraction, and writes a README.chromium so the binaries show up
in brave://credits.
"""

from argparse import ArgumentParser
from brave_chromium_utils import wspath
from deps_config import DEPS_PACKAGES_URL
from os.path import basename, exists, join

import os
import shutil

import deps

README_TEMPLATE = """Name: WinTun
Version: {version}
URL: {url}
Update Mechanism: Manual
License: Prebuilt Binaries License
License File: /brave/third_party/wintun/LICENSE.txt
Security Critical: yes
Shipped: yes

Description:
Wintun is a Layer 3 TUN driver for Windows, used by the Brave VPN WireGuard
service. The signed wintun.dll is shipped unmodified from upstream; only the
Permitted API (wintun.h) is used at compile time.

Local Modifications:
None.
"""


def main():
    args = parse_args()
    dep_url = DEPS_PACKAGES_URL + '/' + args.dep_path
    dest_dir = wspath(args.dest_dir)
    deps.DownloadIfChanged(dep_url,
                           dest_dir,
                           sha256=args.sha256,
                           download_fn=fetch_and_stage)


def parse_args():
    parser = ArgumentParser(
        description='Download and stage the Wintun prebuilt binaries.')
    parser.add_argument('dep_path')
    parser.add_argument('dest_dir')
    parser.add_argument('sha256')
    return parser.parse_args()


def parse_version(filename):
    # wintun-0.14.1.zip -> 0.14.1
    prefix, suffix = 'wintun-', '.zip'
    if not filename.startswith(prefix) or not filename.endswith(suffix):
        raise ValueError(
            f'Could not parse version from filename: {filename!r}. '
            f'Expected the form "wintun-<version>.zip".')
    return filename[len(prefix):-len(suffix)]


def write_readme(dest_dir, version, url):
    with open(join(dest_dir, 'README.chromium'),
              'w',
              encoding='utf-8',
              newline='\n') as f:
        f.write(README_TEMPLATE.format(version=version, url=url))


def fetch_and_stage(dep_url, dest_dir, sha256):
    deps.DownloadAndUnpack(dep_url, dest_dir, sha256=sha256)
    filename = basename(dep_url)

    # The archive nests everything under a top-level 'wintun/' folder.
    # Extract to a scratch dir, then lift the inner contents up so
    # callers reference '//brave/third_party/wintun/...' directly.
    inner = join(dest_dir, 'wintun')
    if not exists(inner):
        raise ValueError(f'Expected a top-level "wintun/" directory in '
                         f'{filename}, but it was missing.')
    for entry in os.listdir(inner):
        shutil.move(join(inner, entry), join(dest_dir, entry))
    # The inner directory must be empty by now; rmdir asserts that.
    os.rmdir(inner)

    # Rename amd64 -> x64 so on-disk paths match GN's target_cpu values
    # and BUILD.gn can use sources = [ "bin/$target_cpu/wintun.dll" ].
    amd64_dir = join(dest_dir, 'bin', 'amd64')
    x64_dir = join(dest_dir, 'bin', 'x64')
    if exists(amd64_dir):
        os.rename(amd64_dir, x64_dir)

    # Sanity-check the license file the README points at, then write the
    # README. A missing license file would break tools/licenses.py.
    if not exists(join(dest_dir, 'LICENSE.txt')):
        raise ValueError(
            f'LICENSE.txt not found in {dest_dir} after extraction. '
            f'README.chromium references it, so the credits build would '
            f'fail.')

    write_readme(dest_dir, parse_version(filename),
                 f'https://www.wintun.net/builds/{filename}')


if __name__ == '__main__':
    main()
