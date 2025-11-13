#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""
Secure Opengrep installer

Downloads opengrep binary directly from GitHub releases and installs to
third_party/opengrep/ directory. Includes optional signature verification using
cosign if available.
"""

import hashlib
import os
import platform
import stat
import subprocess
import sys
import tempfile

# Add script directory to path for brave_chromium_utils import
_SCRIPT_DIR = os.path.join(os.path.dirname(__file__), '..', '..', 'script')
sys.path.insert(0, os.path.abspath(_SCRIPT_DIR))

import brave_chromium_utils

try:
    from urllib2 import HTTPError, URLError, urlopen
except ImportError:  # For Py3 compatibility
    # pylint: disable=no-name-in-module,import-error
    from urllib.error import HTTPError, URLError
    from urllib.request import urlopen

# Configuration
OPENGREP_VERSION = 'v1.11.5'

# SHA256 checksums for each platform binary
# Run: ./script/generate_opengrep_checksums.sh to auto-generate these
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

# Installation directory (relative to brave core)
INSTALL_DIR = brave_chromium_utils.wspath('//brave/third_party/opengrep')


def download_url(url, output_file):
    """Download URL to output file with progress indication."""
    # Validate URL to prevent file:// scheme attacks
    if not url.startswith('https://github.com/'):
        raise RuntimeError(
            f'Invalid URL: {url}. Only HTTPS URLs from github.com are allowed.'
        )

    print(f'Downloading {url}')
    try:
        response = urlopen(url)
        total_size = int(response.info().get('Content-Length', '0').strip())

        bytes_downloaded = 0
        chunk_size = 4096

        while True:
            chunk = response.read(chunk_size)
            if not chunk:
                break
            output_file.write(chunk)
            bytes_downloaded += len(chunk)

            if total_size > 0:
                percent = (bytes_downloaded * 100) // total_size
                sys.stdout.write(f'\rProgress: {percent}%')
                sys.stdout.flush()

        sys.stdout.write('\n')
        print(f'Downloaded {bytes_downloaded} bytes')

    except (HTTPError, URLError) as e:
        raise RuntimeError(f'Failed to download: {e}') from e


def calculate_sha256(file_path):
    """Calculate SHA256 hash of file."""
    sha256 = hashlib.sha256()
    with open(file_path, 'rb') as f:
        for chunk in iter(lambda: f.read(4096), b''):
            sha256.update(chunk)
    return sha256.hexdigest()


def verify_checksum(file_path, expected_checksum):
    """Verify SHA256 checksum matches expected value."""
    if not expected_checksum:
        print('Warning: No checksum available for verification. '
              'Binary will be installed without checksum verification.')
        return

    actual_checksum = calculate_sha256(file_path)
    print(f'Expected SHA256: {expected_checksum}')
    print(f'Actual SHA256:   {actual_checksum}')

    if actual_checksum != expected_checksum:
        raise RuntimeError(
            'SHA256 hash mismatch! Binary may have been tampered with.')

    print('✓ Hash verification passed')


def has_cosign():
    """Check if cosign is available for signature verification."""
    try:
        subprocess.check_output(['cosign', 'version'],
                                stderr=subprocess.STDOUT)
        return True
    except (subprocess.CalledProcessError, OSError):
        return False


def verify_signature(binary_path, sig_path, cert_path):
    """
    Verify binary signature using cosign if available.

    Returns True if verification passed, False if skipped.
    Raises RuntimeError if verification failed.
    """
    if not has_cosign():
        print('Warning: cosign not found. Skipping signature verification.')
        print('Install cosign from https://github.com/sigstore/cosign')
        return False

    if not os.path.exists(sig_path) or not os.path.exists(cert_path):
        print('Warning: Signature files not found. Skipping verification.')
        return False

    print('Verifying signature with cosign...')
    try:
        subprocess.check_call([
            'cosign', 'verify-blob', '--cert', cert_path, '--signature',
            sig_path, '--certificate-identity-regexp',
            'https://github.com/opengrep/opengrep.+',
            '--certificate-oidc-issuer',
            'https://token.actions.githubusercontent.com', binary_path
        ],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)

        print('✓ Signature verification passed')
        return True
    except subprocess.CalledProcessError as e:
        raise RuntimeError(f'Signature verification failed: {e}') from e


def get_platform_dist_name():
    """
    Determine the distribution name for the current platform.

    Returns:
        tuple: (dist_name, platform_description, binary_name)
    """
    system = platform.system()
    machine = platform.machine().lower()

    # Normalize architecture names
    if machine in ('amd64', 'x86_64'):
        arch = 'x86_64'
    elif machine in ('aarch64', 'arm64'):
        arch = 'arm64'
    else:
        raise RuntimeError(f'Unsupported architecture: {machine}. '
                           f'Supported: x86_64, arm64')

    if system == 'Darwin':
        if arch == 'arm64':
            return 'opengrep_osx_arm64', 'macOS (Apple Silicon)', 'opengrep'
        return 'opengrep_osx_x86', 'macOS (Intel)', 'opengrep'

    if system == 'Linux':
        # Check if system uses musl libc
        is_musl = False
        try:
            ldd_output = subprocess.check_output(['ldd', '/bin/sh'],
                                                 stderr=subprocess.STDOUT,
                                                 universal_newlines=True)
            is_musl = 'musl' in ldd_output.lower()
        except (subprocess.CalledProcessError, OSError):
            # If ldd fails, assume glibc
            pass

        libc_type = 'musl' if is_musl else 'glibc'

        if arch == 'arm64':
            dist = ('opengrep_musllinux_aarch64'
                    if is_musl else 'opengrep_manylinux_aarch64')
            return dist, f'Linux {libc_type} (ARM64)', 'opengrep'
        dist = ('opengrep_musllinux_x86'
                if is_musl else 'opengrep_manylinux_x86')
        return dist, f'Linux {libc_type} (x86_64)', 'opengrep'

    if system == 'Windows':
        # Windows binaries have .exe extension (only x86_64 is available)
        if arch == 'arm64':
            raise RuntimeError(
                'Windows ARM64 is not supported. '
                'Only x86_64 binaries are available for Windows.')
        return 'opengrep_windows_x86.exe', 'Windows (x86_64)', 'opengrep.exe'

    raise RuntimeError(f'Unsupported operating system: {system}. '
                       f'Supported: Darwin (macOS), Linux, Windows')


def is_opengrep_installed(binary_name):
    """Check if opengrep is already installed with the correct version."""
    binary_path = os.path.join(INSTALL_DIR, binary_name)
    version_file = os.path.join(INSTALL_DIR, '.version')

    if not os.path.exists(binary_path) or not os.path.exists(version_file):
        return False

    try:
        # Check version file
        with open(version_file, 'r') as f:
            installed_version = f.read().strip()
            if installed_version != OPENGREP_VERSION:
                return False

        # Verify binary is executable and reports correct version
        output = subprocess.check_output([binary_path, '--version'],
                                         stderr=subprocess.STDOUT,
                                         universal_newlines=True).strip()

        print(f'Found existing opengrep: {output}')

        # Check if version matches (remove 'v' prefix)
        version_without_v = OPENGREP_VERSION.replace('v', '')
        return version_without_v in output

    except (subprocess.CalledProcessError, OSError, IOError):
        return False


def test_binary(binary_path):
    """Test that the binary is executable and works."""
    print('Testing binary...')
    try:
        output = subprocess.check_output([binary_path, '--version'],
                                         stderr=subprocess.STDOUT,
                                         universal_newlines=True).strip()

        if not output:
            raise RuntimeError('Binary produced no output')

        print(f'✓ Binary test passed: {output}')
        return True

    except (subprocess.CalledProcessError, OSError) as e:
        raise RuntimeError(f'Binary test failed: {e}') from e


def install_opengrep():
    """Main installation function."""
    # Get platform-specific distribution name
    try:
        dist_name, platform_desc, binary_name = get_platform_dist_name()
    except RuntimeError as e:
        print(f'Error: {e}', file=sys.stderr)
        return 1

    # Check if already installed
    if is_opengrep_installed(binary_name):
        print(f'✓ Opengrep {OPENGREP_VERSION} already installed at '
              f'{INSTALL_DIR}')
        return 0

    print(f'\n*** Installing Opengrep {OPENGREP_VERSION} for '
          f'{platform_desc} ***\n')

    # Construct download URL
    binary_url = (f'https://github.com/opengrep/opengrep/releases/download/'
                  f'{OPENGREP_VERSION}/{dist_name}')

    print(f'URL: {binary_url}')

    # Create installation directory
    try:
        os.makedirs(INSTALL_DIR, exist_ok=True)
    except OSError as e:
        print(f'Error: Failed to create directory {INSTALL_DIR}: {e}',
              file=sys.stderr)
        return 1

    binary_path = os.path.join(INSTALL_DIR, binary_name)
    version_file = os.path.join(INSTALL_DIR, '.version')

    # Download to temporary file
    tmp_fd, tmp_path = tempfile.mkstemp(suffix='-opengrep', dir=INSTALL_DIR)

    try:
        # Download binary
        with os.fdopen(tmp_fd, 'wb') as tmp_file:
            try:
                download_url(binary_url, tmp_file)
            except RuntimeError as e:
                print(f'Error: {e}', file=sys.stderr)
                return 1

        # Verify checksum if available
        expected_checksum = BINARY_CHECKSUMS.get(dist_name, '')
        try:
            verify_checksum(tmp_path, expected_checksum)
        except RuntimeError as e:
            print(f'Error: {e}', file=sys.stderr)
            return 1

        # Download and verify signature files (optional)
        sig_url = f'{binary_url}.sig'
        cert_url = f'{binary_url}.cert'
        tmp_sig_path = tmp_path + '.sig'
        tmp_cert_path = tmp_path + '.cert'

        sig_available = False
        try:
            print('Downloading signature files...')
            with open(tmp_sig_path, 'wb') as sig_file:
                download_url(sig_url, sig_file)
            with open(tmp_cert_path, 'wb') as cert_file:
                download_url(cert_url, cert_file)
            sig_available = True
        except RuntimeError:
            print('Warning: Signature files not available for this release.')

        # Verify signature if available
        if sig_available:
            try:
                verify_signature(tmp_path, tmp_sig_path, tmp_cert_path)
            except RuntimeError as e:
                print(f'Warning: {e}', file=sys.stderr)
                print('Continuing installation despite signature '
                      'verification failure.')

        # Make executable (Unix-like systems)
        if platform.system() != 'Windows':
            st = os.stat(tmp_path)
            os.chmod(tmp_path,
                     st.st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

        # Test binary before moving to final location
        try:
            test_binary(tmp_path)
        except RuntimeError as e:
            print(f'Error: {e}', file=sys.stderr)
            return 1

        # Move to final location (remove existing if present)
        if os.path.exists(binary_path):
            os.remove(binary_path)
        os.rename(tmp_path, binary_path)

        # Clean up signature files (no longer needed)
        for sig_path in [tmp_sig_path, tmp_cert_path]:
            if os.path.exists(sig_path):
                try:
                    os.unlink(sig_path)
                except OSError:
                    pass

        # Write version file
        with open(version_file, 'w') as f:
            f.write(OPENGREP_VERSION + '\n')

        print(f'\n✓ Successfully installed opengrep to {binary_path}')
        print(f'  Version: {OPENGREP_VERSION}')
        print(f'  Platform: {platform_desc}')
        print('\nTo use opengrep, run:')
        print(f'  {binary_path} --help')

        return 0

    except Exception as e:  # pylint: disable=broad-except
        print(f'Error during installation: {e}', file=sys.stderr)
        # Clean up temporary files
        for path in [tmp_path, tmp_path + '.sig', tmp_path + '.cert']:
            if os.path.exists(path):
                try:
                    os.unlink(path)
                except OSError:
                    pass
        return 1


def main():
    """Entry point."""
    try:
        return install_opengrep()
    except Exception as e:  # pylint: disable=broad-except
        print(f'Unexpected error: {e}', file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
