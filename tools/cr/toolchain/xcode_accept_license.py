#!/usr/bin/env python3
# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""xcode_accept_license.py

Accepts the Xcode GM license on a macOS machine without prompting for a
password. Mirrors the approach used in Chromium's infra puppet.

USAGE
    sudo /usr/local/bin/xcode_accept_license.py <xcode-version> <license-version>

    Example:
        sudo /usr/local/bin/xcode_accept_license.py 26.3 EA1647

    Both values come from the hermetic Xcode package:
        <xcode-version>   = Contents/version.plist : CFBundleShortVersionString
        <license-version> = Contents/Resources/LicenseInfo.plist : licenseID

SETTING UP MANUALLY
    Replace `username` with the account name.

        sudo install -o root -g wheel -m 0755 \\
            xcode_accept_license.py /usr/local/bin/xcode_accept_license.py
        echo 'username ALL=(root) NOPASSWD: /usr/local/bin/xcode_accept_license.py [0-9]* [A-Za-z0-9_.-]*' \\
            | sudo tee /etc/sudoers.d/xcode_accept_license
        sudo chown root:wheel /etc/sudoers.d/xcode_accept_license
        sudo chmod 0440 /etc/sudoers.d/xcode_accept_license
        sudo visudo -c -f /etc/sudoers.d/xcode_accept_license
        sudo -n /usr/local/bin/xcode_accept_license.py 26.3 EA1647

SETTING UP WITH install_xcode_accept_license.py
    Same outcome with atomic sudoers placement and a non-destructive smoke
    test. See that script's own docstring for `--check-only` / `--uninstall`.

        python3 install_xcode_accept_license.py --username $USER

SECURITY NOTES
    - The script writes only to /Library/Preferences/com.apple.dt.Xcode.plist
      and performs no network I/O.
    - Arguments are regex-validated before being passed to `defaults` so the
      NOPASSWD path cannot be used to set arbitrary preference keys.
    - Subprocess calls use list-form `subprocess.check_call`
      (never `shell=True`), so each argument lands in argv as a single opaque
      element. Shell-style injection through quoting bugs is not reachable by
      construction.
    - Absolute paths to /usr/bin/defaults and /usr/bin/plutil are hard-coded so
      a manipulated PATH cannot redirect either tool.
"""

import argparse
import re
import subprocess
import sys

PLIST = '/Library/Preferences/com.apple.dt.Xcode.plist'
DEFAULTS = '/usr/bin/defaults'
PLUTIL = '/usr/bin/plutil'

# Xcode short versions look like "26.3" or "26.3.1"; license IDs are short
# alphanumeric tokens such as "EA1647".
XCODE_VERSION_RE = re.compile(r'\d+(\.\d+)*')
LICENSE_VERSION_RE = re.compile(r'[A-Za-z0-9_.-]+')


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Accept the Xcode GM license on behalf of all users.')
    parser.add_argument('xcode_version',
                        help='CFBundleShortVersionString from the hermetic '
                        'Xcode bundle, e.g. 26.3')
    parser.add_argument('license_version',
                        help='licenseID from LicenseInfo.plist, e.g. EA1647')
    args = parser.parse_args()

    if not XCODE_VERSION_RE.fullmatch(args.xcode_version):
        print(f'invalid xcode-version: {args.xcode_version}', file=sys.stderr)
        return 64
    if not LICENSE_VERSION_RE.fullmatch(args.license_version):
        print(f'invalid license-version: {args.license_version}',
              file=sys.stderr)
        return 64

    subprocess.check_call([
        DEFAULTS, 'write', PLIST, 'IDEXcodeVersionForAgreedToGMLicense',
        '-string', args.xcode_version
    ])
    subprocess.check_call([
        DEFAULTS, 'write', PLIST, 'IDELastGMLicenseAgreedTo', '-string',
        args.license_version
    ])
    subprocess.check_call([PLUTIL, '-convert', 'xml1', PLIST])
    return 0


if __name__ == '__main__':
    sys.exit(main())
