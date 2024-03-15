# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import glob
import argparse
import os
import subprocess
import shutil

from pathlib import Path


def main():
    description = 'Create an xcframework from a framework & debug symbols'
    parser = argparse.ArgumentParser(description=description)

    parser.add_argument('--framework_dir',
                        type=Path,
                        required=True,
                        help='The framework bundle to prepare an xcframework')
    parser.add_argument('--remove_asset_catalogs',
                        action='store_true',
                        help='Remove any asset catalogs in the framework')
    parser.add_argument(
        '--fix_info_plist_versions',
        action='store_true',
        help='Ensures that the framework contains both version keys')
    parser.add_argument('--xcframework_dir',
                        type=Path,
                        required=True,
                        help='The output xcframework directory')

    args = parser.parse_args()

    if args.xcframework_dir.exists():
        # The command doesn't let you overwrite so prior files need to be
        # deleted first
        shutil.rmtree(args.xcframework_dir)

    create_xcframework_cmd_args = [
        'xcrun', 'xcodebuild', '-create-xcframework', '-output',
        args.xcframework_dir, '-framework', args.framework_dir
    ]

    symbols_dir = args.framework_dir.with_suffix('.dSYM')
    if symbols_dir.exists():
        create_xcframework_cmd_args += ['-debug-symbols', symbols_dir]
    subprocess.check_call(create_xcframework_cmd_args)

    if args.remove_asset_catalogs:
        RemoveAssetCatalogs(args.xcframework_dir)
    if args.fix_info_plist_versions:
        FixInfoPlistVersions(args.xcframework_dir)


def RemoveAssetCatalogs(xcframework_dir):
    for filename in xcframework_dir.glob('*/*/Assets.car'):
        os.remove(filename)


def FixInfoPlistVersions(xcframework_dir):
    for framework_dir in xcframework_dir.glob('*/*.framework'):
        cmd_args = [
            '/usr/libexec/PlistBuddy', '-c',
            'Add :CFBundleShortVersionString string 1.0',
            os.path.join(framework_dir, 'Info.plist')
        ]
        subprocess.check_call(cmd_args)


if __name__ == '__main__':
    main()
