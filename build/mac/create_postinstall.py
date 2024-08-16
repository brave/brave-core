#!/usr/bin/env python3

# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from argparse import ArgumentParser
from os import makedirs, chmod
from os.path import dirname

import os
import re
import stat


def main():
    args = parse_args()

    with open(args.postinstall_template_path) as f:
        script = f.read()

    replacements = {
        '@APP_DIR@': args.app_dir_name,
        '@APP_PRODUCT@': '',
        '@BRAND_CODE@': '',
        '@FRAMEWORK_DIR@': args.app_dir_name + '/' +
        args.framework_dir_in_app_dir,
        '@SHEBANG_GUARD@': '',
        'GoogleUpdater': 'BraveUpdater',
        'KSProductID': 'CFBundleIdentifier',
        'KSVersion': 'CFBundleShortVersionString',
        '/Library/Google/GoogleSoftwareUpdate/GoogleSoftwareUpdate.bundle/'
        'Contents/MacOS/ksadmin': '/Library/Application Support/BraveSoftware'
        '/BraveUpdater/Current/BraveUpdater.app/'
        'Contents/Helpers/BraveSoftwareUpdate.bundle/'
        'Contents/Helpers/ksadmin'
    }
    for key, value in replacements.items():
        assert key in script, key
        script = script.replace(key, value)

    m = re.search(r'@[^@\n]+@', script)
    assert not m, 'Unexpected placeholder: ' + m.group(0)

    makedirs(dirname(args.out_file), exist_ok=True)

    with open(args.out_file, 'w') as f:
        f.write(script)

    make_executable(args.out_file)


def parse_args():
    parser = ArgumentParser()
    parser.add_argument('postinstall_template_path')
    parser.add_argument('app_dir_name')
    parser.add_argument('framework_dir_in_app_dir')
    parser.add_argument('out_file')
    return parser.parse_args()


def make_executable(file_path):
    chmod(file_path, os.stat(file_path).st_mode | stat.S_IEXEC)


if __name__ == '__main__':
    main()
