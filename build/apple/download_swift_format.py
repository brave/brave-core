#!/usr/bin/env python3

# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import argparse
import os
import hashlib
import deps

from brave_chromium_utils import wspath
from deps_config import DEPS_PACKAGES_URL


def main():
    parser = argparse.ArgumentParser(description='Download swift-format')
    parser.add_argument('version')
    parser.add_argument('sha256hash')
    args = parser.parse_args()

    url = f'{DEPS_PACKAGES_URL}/swift-format/swift-format-{args.version}.zip'
    # Overwrite Chromium's swift-format version as it very out of date and
    # we will not be editing Chromium swift files to format anyways.
    output_dir = wspath('//third_party/swift-format')
    swift_format_bin = os.path.join(output_dir, 'swift-format')
    if get_swift_format_hash(swift_format_bin) != args.sha256hash:
        deps.DownloadAndUnpack(url, output_dir)
        # Add execute privileges since they are not preserved in the zip
        os.chmod(swift_format_bin, 0o755)


def get_swift_format_hash(swift_format_bin):
    if not os.path.exists(swift_format_bin):
        return None
    with open(swift_format_bin, 'rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


if __name__ == '__main__':
    main()
