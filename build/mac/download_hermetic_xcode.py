#!/usr/bin/env vpython3
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Brave's hermetic Xcode toolchain installer.

Mirrors the structure of upstream's build/mac_toolchain.py so changes there
port cleanly. Brave-specific differences:

  * Downloads a Brave-internal tarball rather than a CIPD package.
  * Gated on USE_BRAVE_HERMETIC_TOOLCHAIN=1 in place of upstream's
    should_use_hermetic_xcode.py check.
  * Compares Xcode versions via packaging.version.parse (semver-aware)
    instead of upstream's string-split lexicographic compare.
"""

from __future__ import annotations

import argparse
import os
import platform
import plistlib
import subprocess
import sys
from pathlib import Path
from urllib.error import URLError  # pylint: disable=no-name-in-module,import-error

from packaging.version import parse as parse_version

# Importing script explicitly, so we can call this script from the terminal
# without needing to set up the PYTHONPATH.
sys.path.append(str(Path(__file__).resolve().parents[2] / 'script'))

import deps  # pylint: disable=wrong-import-position

# The hash sum for the archive expected to be downloaded.
MAC_BINARIES_HASH = '710798910d2a458bcb41fc0f5181f472bf899d20bfdcb28dcd40efef556cd590'

# This contains binaries from Xcode 26.4.1 (17E202) along with the macOS 26.4
# SDK (25E251) and the Metal toolchain (17E188).
MAC_SDK_OFFICIAL_VERSION = '26.4'
MAC_SDK_OFFICIAL_BUILD_VERSION = '25E251'
XCODE_TOOLCHAIN_DOWNLOAD_URL = (
    'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-2.on.aws'
    '/xcode-hermetic-toolchain/xcode-hermetic-toolchain-'
    f'{MAC_SDK_OFFICIAL_VERSION}-{MAC_SDK_OFFICIAL_BUILD_VERSION}.tar.gz')

# The toolchain will not be downloaded if the minimum OS version is not met. 19
# is the Darwin major version number for macOS 10.15. Xcode 26.0 17A324 only
# runs on macOS 15.6 and newer, but some bots are still running older OS
# versions. macOS 10.15.4, the OS minimum through Xcode 12.4, still seems to
# work.
MAC_MINIMUM_OS_VERSION = [19, 4]

# Destination for the hermetic Xcode binaries to be extracted at.
MAC_BINARIES_ROOT = Path(
    __file__).resolve().parents[3] / 'build' / 'mac_files' / 'xcode_binaries'

# The name for the hash file use to track the installation of the toolchain.
DOWNLOADED_TOOLCHAIN_HASH = (
    MAC_BINARIES_ROOT /
    f'.{os.path.basename(XCODE_TOOLCHAIN_DOWNLOAD_URL).replace(".", "_")}_hash'
)


def _load_plist(path: Path) -> dict:
    """Loads the plist at path and returns it as a dictionary."""
    return plistlib.loads(path.read_bytes())


def _platform_meets_hermetic_xcode_requirements() -> bool:
    if sys.platform == 'darwin':
        needed = MAC_MINIMUM_OS_VERSION
        major_version = [
            int(v) for v in platform.release().split('.')[:len(needed)]
        ]
        return major_version >= needed
    return sys.platform.startswith('linux')


def _use_hermetic_toolchain() -> bool:
    return os.environ.get('USE_BRAVE_HERMETIC_TOOLCHAIN') == '1'


def _get_hermetic_xcode_version() -> str:
    plist_path = MAC_BINARIES_ROOT / 'Contents/version.plist'
    if not plist_path.exists():
        return ''
    return _load_plist(plist_path)['CFBundleShortVersionString']


class SdkInstaller:
    """A basic installer to handle the SDK archive.

    This installer is designed to download and extract the SDK archive when
    needed. It relies on a sidecar file to record the hash of the currently
    installed SDK, rather than keying up the install merely by the SDK version
    present in the destination.
    """

    def is_installed(self) -> bool:
        """Returns True if the sidecar exists and has the expected hash.

        A symlinked destination is never treated as installed, so the caller
        always redeploys a real directory on top of it.
        """
        if MAC_BINARIES_ROOT.is_symlink():
            return False
        if not DOWNLOADED_TOOLCHAIN_HASH.is_file():
            return False
        return DOWNLOADED_TOOLCHAIN_HASH.read_text().rstrip(
        ) == MAC_BINARIES_HASH

    def install(self) -> bool:
        """Downloads and extracts the SDK archive if it is not already present.

        Returns True if the archive was downloaded and extracted, or False if
        the expected hash was already in place and nothing needed to be done.
        Also writes the expected hash to the sidecar file, which is used to
        detect the current install. On a download failure it prints a
        diagnostic and re-raises.
        """
        if self.is_installed():
            return False
        print(f'Downloading hermetic Xcode: {XCODE_TOOLCHAIN_DOWNLOAD_URL}')
        try:
            deps.DownloadAndUnpack(XCODE_TOOLCHAIN_DOWNLOAD_URL,
                                   MAC_BINARIES_ROOT,
                                   sha256=MAC_BINARIES_HASH)
        except URLError:
            print(
                f'Failed to download hermetic Xcode: {XCODE_TOOLCHAIN_DOWNLOAD_URL}'
            )
            raise
        DOWNLOADED_TOOLCHAIN_HASH.write_text(MAC_BINARIES_HASH)
        return True


def _install_xcode_binaries() -> int:
    """Installs the Xcode binaries needed to build Brave and accepts the
    license."""
    SdkInstaller().install()

    if sys.platform != 'darwin':
        return 0

    # Accept the license for this version of Xcode if it's newer than the
    # currently accepted version.
    hermetic_xcode_version_plist_path = (MAC_BINARIES_ROOT /
                                         'Contents/version.plist')
    hermetic_xcode_version_plist = _load_plist(
        hermetic_xcode_version_plist_path)
    hermetic_xcode_version = (
        hermetic_xcode_version_plist['CFBundleShortVersionString'])

    hermetic_xcode_license_path = (MAC_BINARIES_ROOT /
                                   'Contents/Resources/LicenseInfo.plist')
    hermetic_xcode_license_plist = _load_plist(hermetic_xcode_license_path)
    hermetic_xcode_license_version = hermetic_xcode_license_plist['licenseID']

    should_overwrite_license = True
    current_license_path = Path(
        '/Library/Preferences/com.apple.dt.Xcode.plist')
    if current_license_path.exists():
        current_license_plist = _load_plist(current_license_path)
        xcode_version = current_license_plist.get(
            'IDEXcodeVersionForAgreedToGMLicense')
        if (xcode_version is not None and parse_version(xcode_version)
                >= parse_version(hermetic_xcode_version)):
            should_overwrite_license = False

    if not should_overwrite_license:
        return 0

    # Use puppet's sudoers script to accept the license if it's available.
    license_accept_script = Path('/usr/local/bin/xcode_accept_license.py')
    if license_accept_script.exists():
        args = [
            'sudo', license_accept_script, hermetic_xcode_version,
            hermetic_xcode_license_version
        ]
        subprocess.check_call(args)
        return 0

    # Otherwise manually accept the license. This will prompt for sudo.
    print('Accepting new Xcode license. Requires sudo.')
    sys.stdout.flush()
    args = [
        'sudo', 'defaults', 'write', current_license_path,
        'IDEXcodeVersionForAgreedToGMLicense', hermetic_xcode_version
    ]
    subprocess.check_call(args)
    args = [
        'sudo', 'defaults', 'write', current_license_path,
        'IDELastGMLicenseAgreedTo', hermetic_xcode_license_version
    ]
    subprocess.check_call(args)
    args = ['sudo', 'plutil', '-convert', 'xml1', current_license_path]
    subprocess.check_call(args)

    return 0


def main() -> int:
    if not _use_hermetic_toolchain():
        print('Brave hermetic toolchain is not configured')
        return 0

    parser = argparse.ArgumentParser(description='Download hermetic Xcode.')
    parser.parse_args()

    if not _platform_meets_hermetic_xcode_requirements():
        print('OS version does not support hermetic Xcode toolchain.')
        return 0

    return _install_xcode_binaries()


if __name__ == '__main__':
    sys.exit(main())
