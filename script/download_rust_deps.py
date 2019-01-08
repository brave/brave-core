#!/usr/bin/env python
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

"""Script to download rust_deps."""

import os
import re
import subprocess
import sys
import urllib2

import deps
from rust_deps_config import RUST_DEPS_PACKAGES_URL, RUST_DEPS_PACKAGE_VERSION

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
RUSTUP_DIR = os.path.join(SOURCE_ROOT, 'build', 'rustup', RUST_DEPS_PACKAGE_VERSION)


def GetUrl(platform):
    if platform == "win32" or platform == "cygwin":
        filename = "rust_deps_win_" + RUST_DEPS_PACKAGE_VERSION + ".zip"
    elif platform == 'darwin':
        filename = "rust_deps_mac_" + RUST_DEPS_PACKAGE_VERSION + ".gz"
    elif platform.startswith('linux'):
        filename = "rust_deps_linux_" + RUST_DEPS_PACKAGE_VERSION + ".gz"
    else:
        print 'Invalid platform for Rust deps %s' % platform
        print 'Exiting.'
        sys.exit(1)

    return RUST_DEPS_PACKAGES_URL + "/" + filename


def AlreadyUpToDate():
    if not os.path.exists(RUSTUP_DIR):
        return False

    return True


def DownloadAndUnpackRustDeps(platform):
    if AlreadyUpToDate():
        return 0

    url = GetUrl(platform)

    try:
        deps.DownloadAndUnpack(url, RUSTUP_DIR)
    except urllib2.URLError:
        print 'Failed to download Rust deps %s' % rust_deps_url
        print 'Exiting.'
        sys.exit(1)


def main():
    DownloadAndUnpackRustDeps(sys.platform)
    return 0


if __name__ == '__main__':
    sys.exit(main())
