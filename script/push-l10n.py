#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import argparse
import os
import sys

from lib.l10n.transifex.common import should_use_transifex_for_file
from lib.l10n.transifex.push import (
    check_for_chromium_upgrade,
    check_missing_source_grd_strings_to_transifex,
    upload_missing_json_translations_to_transifex,
    upload_source_files_to_transifex,
    upload_source_strings_desc)
from lib.l10n.grd_utils import get_override_file_path


BRAVE_SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
SOURCE_ROOT = os.path.dirname(BRAVE_SOURCE_ROOT)


def parse_args():
    parser = argparse.ArgumentParser(description='Push strings to Transifex')
    parser.add_argument('--source_string_path',
                        nargs=1)
    return parser.parse_args()


def main():
    args = parse_args()

    source_string_path = os.path.join(BRAVE_SOURCE_ROOT,
                                      args.source_string_path[0])
    filename = os.path.basename(source_string_path).split('.')[0]

    if not should_use_transifex_for_file(source_string_path, filename):
        override_string_path = get_override_file_path(source_string_path)
        filename = os.path.basename(override_string_path).split('.')[0]
        # This check is needed because some files that we process have no
        # replacements needed so in that case we don't even put an override
        # file in Transifex.
        if not os.path.exists(override_string_path):
            print('Skipping fully locally handled, override not present: '
                  f'{override_string_path} filename: {filename}')
            return
        print('Handled locally, sending only overrides to Transifex: '
              f'{override_string_path} filename: {filename}')
        upload_source_files_to_transifex(override_string_path, filename)
        upload_source_strings_desc(override_string_path, filename)
        return

    print(f'[Transifex]: {source_string_path}')
    upload_source_files_to_transifex(source_string_path, filename)
    ext = os.path.splitext(source_string_path)[1]
    if ext == '.grd':
        check_for_chromium_upgrade(SOURCE_ROOT, source_string_path)
        check_missing_source_grd_strings_to_transifex(source_string_path)
    upload_source_strings_desc(source_string_path, filename)
    if ('ethereum-remote-client' in source_string_path or
            'brave-site-specific-scripts' in source_string_path):
        upload_missing_json_translations_to_transifex(source_string_path)


if __name__ == '__main__':
    sys.exit(main())
