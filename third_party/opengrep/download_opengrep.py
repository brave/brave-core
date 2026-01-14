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
OPENGREP_VERSION = 'v1.14.1'

# SHA256 checksums for each platform binary
BINARY_CHECKSUMS = {
    'opengrep_osx_arm64': (
        '1272dd1322d686d09fb97f1a3b743e17d7d1babcfc0d421f20c76d7dbdef4695'),
    'opengrep_osx_x86': (
        '34bc4b5db6dc6654cf71324388f13d29f9cb40a0170f47935389f0f30434b28a'),
    'opengrep_manylinux_x86': (
        '85e8a4af4a37d58565c35010221574887329a043271d4efff81e35e455c87f9e'),
    'opengrep_manylinux_aarch64': (
        '30efbe59a173c53f020c995c33d8db6ae34eccc958a14bbe5fe4047d36595abf'),
    'opengrep_musllinux_x86': (
        'ff9bb40afc87cb324ccf5bb79b8e772bf2eb05cca9bbe3f1c42ea95488d9ae5a'),
    'opengrep_musllinux_aarch64': (
        '829d7a6b72a501c3ff938bd8551cc596a66bbc1e8ac55966742b5992c430d681'),
    'opengrep_windows_x86.exe': (
        '58f4bba7ef474b143bb5eeee803a4791a05269fd461737e0e2faa96732160ac3'),
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
    # original url: https://github.com/opengrep/opengrep/releases/download/
    url = (f'https://brave-build-deps-public.s3.brave.com/opengrep/'
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
