#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Download and install opengrep binary from GitHub releases."""

import hashlib
import os
import platform
import stat
import sys

# Add script directory to path for brave_chromium_utils import
_SCRIPT_DIR = os.path.join(os.path.dirname(__file__), '..', '..', 'script')
sys.path.insert(0, os.path.abspath(_SCRIPT_DIR))

import brave_chromium_utils
import deps

# Configuration
OPENGREP_VERSION = 'v1.11.5'

# SHA256 checksums for each platform binary
BINARY_CHECKSUMS = {
    'opengrep_osx_arm64': (
        '895ec727663572dd9a5721cb9ae4f97f69c80f15d57b28cbd1e580f882a27028'),
    'opengrep_osx_x86': (
        'fc0b0d61b20a9b3d160e841540f5c60b456b036527a96d39c8df7a0ae6793bae'),
    'opengrep_manylinux_x86': (
        'b9dd6dede671e1ea2e2628ef4c04ff10afd8de2c4ef50af764b29d5f0980eec3'),
    'opengrep_manylinux_aarch64': (
        'd5def7bf466d25e7b57bbcdfd4243c58f297bf3271625f71125d67759dc98742'),
    'opengrep_musllinux_x86': (
        '564bdb4aae5230b0311af5333f08e8b6eff97b896d8569616957e10da25b8644'),
    'opengrep_musllinux_aarch64': (
        'd28498b42c2a1642e4c83071362802ded8aad4b45bd1b7884aadc55209c38916'),
    'opengrep_windows_x86.exe': (
        '3d9b283fde540cfc91afdc3949bb52ac64262592099c301a8da0d125f87bf552'),
}

# Download directory (relative to brave core)
INSTALL_DIR = brave_chromium_utils.wspath('//brave/third_party/opengrep/bin')


def GetPlatformDistName():
    """Determine the distribution name for the current platform."""
    system = platform.system()
    machine = platform.machine().lower()

    # Normalize architecture names
    if machine in ('amd64', 'x86_64'):
        arch = 'x86_64'
    elif machine in ('aarch64', 'arm64'):
        arch = 'arm64'
    else:
        print(f'Unsupported architecture: {machine}')
        return None, None

    if system == 'Darwin':
        if arch == 'arm64':
            return 'opengrep_osx_arm64', 'opengrep'
        return 'opengrep_osx_x86', 'opengrep'

    if system == 'Linux':
        # Simple detection: assume glibc unless Alpine or similar
        if arch == 'arm64':
            return 'opengrep_manylinux_aarch64', 'opengrep'
        return 'opengrep_manylinux_x86', 'opengrep'

    if system == 'Windows':
        if arch == 'arm64':
            print('Windows ARM64 is not supported')
            return None, None
        return 'opengrep_windows_x86.exe', 'opengrep.exe'

    print(f'Unsupported operating system: {system}')
    return None, None


def IsOpengrepDownloaded(binary_name):
    """Check if opengrep is already downloaded with the correct version."""
    binary_path = os.path.join(INSTALL_DIR, binary_name)
    version_file = os.path.join(INSTALL_DIR, '.version')

    if not os.path.exists(binary_path) or not os.path.exists(version_file):
        return False

    try:
        with open(version_file, 'r') as f:
            installed_version = f.read().strip()
            if installed_version == OPENGREP_VERSION:
                print(f'Opengrep {OPENGREP_VERSION} already downloaded')
                return True
    except IOError:
        pass

    return False


def VerifyChecksum(file_path, expected_checksum):
    """Verify SHA256 checksum of downloaded file."""
    sha256 = hashlib.sha256()
    with open(file_path, 'rb') as f:
        for chunk in iter(lambda: f.read(4096), b''):
            sha256.update(chunk)

    actual = sha256.hexdigest()
    if actual != expected_checksum:
        raise RuntimeError(
            f'Checksum mismatch!\nExpected: {expected_checksum}\nActual: {actual}'
        )


def DownloadOpengrep():
    """Download opengrep binary."""
    dist_name, binary_name = GetPlatformDistName()
    if not dist_name:
        return 1

    if IsOpengrepDownloaded(binary_name):
        return 0

    binary_path = os.path.join(INSTALL_DIR, binary_name)
    version_file = os.path.join(INSTALL_DIR, '.version')

    # Construct download URL
    url = (f'https://github.com/opengrep/opengrep/releases/download/'
           f'{OPENGREP_VERSION}/{dist_name}')

    print(f'Downloading opengrep {OPENGREP_VERSION}...')

    # Create download directory
    deps.EnsureDirExists(INSTALL_DIR)

    # Download to temporary file
    tmp_path = None
    try:
        import tempfile
        with tempfile.NamedTemporaryFile(delete=False, dir=INSTALL_DIR) as tmp:
            deps.DownloadUrl(url, tmp)
            tmp_path = tmp.name

        # Verify checksum
        expected_checksum = BINARY_CHECKSUMS.get(dist_name)
        if not expected_checksum:
            raise RuntimeError(
                f'No checksum available for {dist_name}. Cannot verify download.'
            )

        print('Verifying checksum...')
        VerifyChecksum(tmp_path, expected_checksum)

        # Make executable (Unix-like systems)
        if platform.system() != 'Windows':
            st = os.stat(tmp_path)
            os.chmod(tmp_path,
                     st.st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

        # Move to final location
        if os.path.exists(binary_path):
            os.remove(binary_path)
        os.rename(tmp_path, binary_path)
        tmp_path = None  # Successfully moved, don't clean up

        # Write version file
        with open(version_file, 'w') as f:
            f.write(OPENGREP_VERSION + '\n')

        print(f'Successfully downloaded opengrep to {binary_path}')
        return 0

    except Exception as e:
        print(f'Error downloading opengrep: {e}')
        return 1
    finally:
        # Clean up temporary file if it still exists
        if tmp_path and os.path.exists(tmp_path):
            try:
                os.remove(tmp_path)
            except Exception:
                pass  # Best effort cleanup


def main():
    """Entry point."""
    return DownloadOpengrep()


if __name__ == '__main__':
    sys.exit(main())
