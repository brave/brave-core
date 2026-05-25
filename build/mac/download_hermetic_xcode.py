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

# This contains binaries from Xcode 26.4 (17E202) along with the macOS 26.4 SDK
# (25E251) and the Metal toolchain (17E188).
XCODE_VERSION = '26.4.1'
XCODE_TOOLCHAIN_DOWNLOAD_URL = (
    f'https://vhemnu34de4lf5cj6bx2wwshyy0egdxk.lambda-url.us-west-2.on.aws'
    f'/xcode-hermetic-toolchain/xcode-hermetic-toolchain-{XCODE_VERSION}.tar.gz'
)

# The toolchain will not be downloaded if the minimum OS version is not met. 19
# is the Darwin major version number for macOS 10.15. Xcode 26.0 17A324 only
# runs on macOS 15.6 and newer, but some bots are still running older OS
# versions. macOS 10.15.4, the OS minimum through Xcode 12.4, still seems to
# work.
MAC_MINIMUM_OS_VERSION = [19, 4]

MAC_TOOLCHAIN_ROOT = Path(
    __file__).resolve().parents[3] / 'build' / 'mac_files'


def LoadPList(path: Path) -> dict:
    """Loads Plist at |path| and returns it as a dictionary."""
    return plistlib.loads(path.read_bytes())


def PlatformMeetsHermeticXcodeRequirements() -> bool:
    if sys.platform == 'darwin':
        needed = MAC_MINIMUM_OS_VERSION
        major_version = [
            int(v) for v in platform.release().split('.')[:len(needed)]
        ]
        return major_version >= needed
    return sys.platform.startswith('linux')


def _UseHermeticToolchain() -> bool:
    return os.environ.get('USE_BRAVE_HERMETIC_TOOLCHAIN') == '1'


def GetHermeticXcodeVersion(binaries_root: Path) -> str:
    plist_path = binaries_root / 'Contents/version.plist'
    if not plist_path.exists():
        return ''
    return LoadPList(plist_path)['CFBundleShortVersionString']


def InstallXcodeBinaries() -> int:
    """Installs the Xcode binaries needed to build Brave and accepts the
    license."""
    binaries_root = MAC_TOOLCHAIN_ROOT / 'xcode_binaries'

    # Tarball extraction or not, if we have a hermetic toolchain,we still want
    # to process the license if the version is newer than the currently
    # accepted one.
    if (XCODE_VERSION != GetHermeticXcodeVersion(binaries_root)
            or binaries_root.is_symlink()):
        url = XCODE_TOOLCHAIN_DOWNLOAD_URL
        print(f"Downloading hermetic Xcode: {url}")
        try:
            deps.DownloadAndUnpack(url, binaries_root)
        except URLError:
            print(f"Failed to download hermetic Xcode: {url}")
            print("Exiting.")
            return 1
    else:
        print(f"Hermetic Xcode {XCODE_VERSION} already installed")

    on_disk_version = GetHermeticXcodeVersion(binaries_root)
    license_info_path = (binaries_root /
                         'Contents/Resources/LicenseInfo.plist')
    on_disk_license = (LoadPList(license_info_path).get(
        'licenseID', '(missing)') if license_info_path.exists() else
                       '(LicenseInfo.plist not present)')
    print(f"  on-disk hermetic version:    {on_disk_version}")
    print(f"  on-disk hermetic licenseID:  {on_disk_license}")
    current_license_path = Path(
        '/Library/Preferences/com.apple.dt.Xcode.plist')
    if current_license_path.exists():
        sys_plist = LoadPList(current_license_path)
        sys_version = sys_plist.get('IDEXcodeVersionForAgreedToGMLicense',
                                    '(missing)')
        sys_license = sys_plist.get('IDELastGMLicenseAgreedTo', '(missing)')
    else:
        sys_version = sys_license = '(plist not present)'
    print(f"  system recorded version:     {sys_version}")
    print(f"  system recorded licenseID:   {sys_license}")

    if sys.platform != 'darwin':
        return 0

    # Accept the license for this version of Xcode if it's newer than the
    # currently accepted version.
    hermetic_xcode_version_plist_path = binaries_root / 'Contents/version.plist'
    hermetic_xcode_version_plist = LoadPList(hermetic_xcode_version_plist_path)
    hermetic_xcode_version = (
        hermetic_xcode_version_plist['CFBundleShortVersionString'])

    hermetic_xcode_license_path = (binaries_root /
                                   'Contents/Resources/LicenseInfo.plist')
    hermetic_xcode_license_plist = LoadPList(hermetic_xcode_license_path)
    hermetic_xcode_license_version = hermetic_xcode_license_plist['licenseID']

    should_overwrite_license = True
    current_license_path = Path(
        '/Library/Preferences/com.apple.dt.Xcode.plist')
    if current_license_path.exists():
        current_license_plist = LoadPList(current_license_path)
        xcode_version = current_license_plist.get(
            'IDEXcodeVersionForAgreedToGMLicense')
        if (xcode_version is not None and parse_version(xcode_version)
                >= parse_version(hermetic_xcode_version)):
            should_overwrite_license = False

    if not should_overwrite_license:
        return 0

    # Use puppet's sudoers script to accept the license if its available.
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
    if not _UseHermeticToolchain():
        print("Brave hermetic toolchain is not configured")
        return 0

    parser = argparse.ArgumentParser(description='Download hermetic Xcode.')
    parser.parse_args()

    if not PlatformMeetsHermeticXcodeRequirements():
        print('OS version does not support hermetic Xcode toolchain.')
        return 0

    return InstallXcodeBinaries()


if __name__ == '__main__':
    sys.exit(main())
