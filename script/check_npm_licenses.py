#!/usr/bin/env python3
# Copyright (c) 2020 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import argparse
import json
import os
import sys

from lib.config import SOURCE_ROOT


EXCLUSIONS = [
    # Already covered by brave/third_party/npm-types
    '@types/jszip',
    '@types/parse-torrent',
    '@types/webtorrent',
]


def check_dependency(module_name):
    if module_name in EXCLUSIONS:
        return True

    third_party_dir = os.path.join(
        os.path.dirname(SOURCE_ROOT), 'brave', 'third_party')
    module_dir_name = module_name.replace('/', '_')
    readme_path = os.path.join(third_party_dir, 'npm_%s' %
                               module_dir_name, 'README.chromium')
    if not os.path.isfile(readme_path):
        print('npm module %s needs licensing information in %s' %
              (module_name, readme_path))
        return False

    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('output_file', nargs='?')
    args = parser.parse_args()

    return_code = 0
    package_json = os.path.join(os.path.dirname(
        SOURCE_ROOT), 'brave', 'package.json')
    with open(package_json) as file_handle:
        dependencies = json.loads(file_handle.read())["dependencies"]
        for module_name in dependencies:
            if not check_dependency(module_name):
                return_code = 1

    if args.output_file:
        with open(args.output_file, 'wt') as file_handle:
            file_handle.write('')

    return return_code


if __name__ == '__main__':
    sys.exit(main())
