#!/usr/bin/env vpython3
# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Script to automatically update opengrep to the latest version.

This script fetches the latest opengrep release from GitHub, downloads all
platform binaries, calculates their checksums, and updates install_opengrep.py
with the new version and checksums.
"""

import argparse
import hashlib
import json
import logging
import os
import re
import subprocess
import sys

try:
    from urllib2 import HTTPError, URLError, urlopen
except ImportError:  # For Py3 compatibility
    # pylint: disable=no-name-in-module,import-error
    from urllib.error import HTTPError, URLError
    from urllib.request import urlopen

# All supported platforms
PLATFORMS = [
    'opengrep_osx_arm64',
    'opengrep_osx_x86',
    'opengrep_manylinux_x86',
    'opengrep_manylinux_aarch64',
    'opengrep_musllinux_x86',
    'opengrep_musllinux_aarch64',
    'opengrep_windows_x86.exe',
]


def _FetchLatestRelease():
    """Fetch the latest opengrep release version from GitHub."""
    logging.info('Fetching latest opengrep release from GitHub...')
    url = 'https://api.github.com/repos/opengrep/opengrep/releases/latest'

    try:
        response = urlopen(url)
        data = json.loads(response.read().decode('utf-8'))
        version = data.get('tag_name')

        if not version:
            raise RuntimeError('Could not find version tag in release data')

        logging.info('Latest release: %s', version)
        return version

    except (HTTPError, URLError) as e:
        raise RuntimeError(f'Failed to fetch releases: {e}') from e


def _DownloadBinary(version, platform):
    """Download a binary for the specified platform and return its content."""
    url = (f'https://github.com/opengrep/opengrep/releases/download/'
           f'{version}/{platform}')
    logging.info('  Downloading %s...', platform)

    try:
        response = urlopen(url)
        data = response.read()
        return data

    except (HTTPError, URLError) as e:
        logging.warning('  Failed to download %s (%s)', platform, e)
        return None


def _CalculateSHA256(data):
    """Calculate SHA256 hash of binary data."""
    return hashlib.sha256(data).hexdigest()


def _DownloadAndHashBinaries(version):
    """Download all platform binaries and calculate their checksums."""
    logging.info('Downloading binaries and calculating checksums...')

    checksums = {}
    downloaded_count = 0

    for platform in PLATFORMS:
        data = _DownloadBinary(version, platform)

        if data:
            checksum = _CalculateSHA256(data)
            checksums[platform] = checksum
            logging.info('  \u2713 %s: %s', platform, checksum)
            downloaded_count += 1

    if downloaded_count == 0:
        raise RuntimeError(
            'No binaries could be downloaded. Check the version number.')

    logging.info('\u2713 Downloaded %d/%d binaries', downloaded_count,
                 len(PLATFORMS))
    return checksums


def _UpdateInstallScript(install_script_path, version, checksums):
    """Update install_opengrep.py with new version and checksums."""
    logging.info('Updating %s...', install_script_path)

    with open(install_script_path, 'r') as f:
        content = f.read()

    original_content = content
    has_changes = False

    # Update version
    version_regex = r"^OPENGREP_VERSION = '[^']*'"
    new_version_line = f"OPENGREP_VERSION = '{version}'"

    if re.search(version_regex, content, re.MULTILINE):
        new_content = re.sub(version_regex,
                             new_version_line,
                             content,
                             flags=re.MULTILINE)
        if new_content != content:
            logging.info("  \u2713 Updated OPENGREP_VERSION to '%s'", version)
            content = new_content
            has_changes = True

    # Update checksums in BINARY_CHECKSUMS dictionary
    # The format is:
    #     'platform': (
    #         'checksum'),
    for platform, checksum in checksums.items():
        # Escape special regex characters in platform name (for .exe)
        escaped_platform = re.escape(platform)

        # Match multi-line pattern: 'platform': (\n        'checksum'),
        checksum_regex = (rf"('{escaped_platform}': \(\n\s+')[a-f0-9]{{64}}"
                          r"('\),)")

        if re.search(checksum_regex, content):
            new_content = re.sub(checksum_regex, rf'\g<1>{checksum}\g<2>',
                                 content)
            if new_content != content:
                logging.info('  \u2713 Updated %s', platform)
                content = new_content
                has_changes = True
        else:
            logging.warning('  Could not find checksum entry for %s', platform)

    if has_changes:
        with open(install_script_path, 'w') as f:
            f.write(content)
        logging.info('\u2713 Successfully updated install_opengrep.py')
        return True

    logging.info('No changes needed')
    return False


def _VerifyPythonSyntax(file_path):
    """Verify Python syntax of the updated script."""
    logging.info('Verifying Python syntax...')

    try:
        subprocess.check_call(['python3', '-m', 'py_compile', file_path],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)
        logging.info('\u2713 Python syntax validation passed')
        return True
    except subprocess.CalledProcessError as e:
        logging.error('Python syntax validation failed: %s', e)
        return False


def _Roll():
    """Main roll function."""
    # Get repository root (script is in third_party/opengrep/)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    third_party_dir = os.path.dirname(script_dir)
    repo_root = os.path.dirname(third_party_dir)
    install_script_path = os.path.join(script_dir, 'install_opengrep.py')

    # Fetch latest release version
    version = _FetchLatestRelease()

    # Check if already up to date
    with open(install_script_path, 'r') as f:
        current_content = f.read()

    current_version_match = re.search(r"OPENGREP_VERSION = '([^']+)'",
                                      current_content)

    if current_version_match and current_version_match.group(1) == version:
        logging.info('\u2713 Opengrep is already up to date (%s)', version)
        return 0

    # Download binaries and calculate checksums
    checksums = _DownloadAndHashBinaries(version)

    # Update install script
    has_changes = _UpdateInstallScript(install_script_path, version, checksums)

    if not has_changes:
        logging.info('\u2713 No changes were made')
        return 0

    # Verify Python syntax
    if not _VerifyPythonSyntax(install_script_path):
        return 1

    logging.info('=' * 43)
    logging.info('\u2713 Successfully updated opengrep')
    logging.info('=' * 43)
    logging.info('Version: %s', version)
    logging.info('Updated: %d checksums', len(checksums))
    logging.info('Changes:')
    logging.info('  - third_party/opengrep/install_opengrep.py')

    return 0


def main():
    """Entry point."""
    parser = argparse.ArgumentParser(
        description='Update opengrep to the latest version')
    args = parser.parse_args()

    try:
        return _Roll()
    except Exception as e:  # pylint: disable=broad-except
        logging.error('Error updating opengrep: %s', e)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    logging.getLogger().setLevel(logging.INFO)
    sys.exit(main())
