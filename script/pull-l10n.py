#!/usr/bin/env python3
#
# Copyright (c) 2022 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/. */

import argparse
import os.path
import sys

from lib.l10n.grd_utils import (get_override_file_path, update_xtbs_locally)
from lib.l10n.transifex.common import should_use_transifex_for_file
from lib.l10n.transifex.pull import (combine_override_xtb_into_original,
                                     pull_source_files_from_transifex)


BRAVE_SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))


def parse_args():
    parser = argparse.ArgumentParser(description='Pull strings from Transifex')
    parser.add_argument('--source_string_path', nargs=1)
    return parser.parse_args()


def main():
    args = parse_args()

    source_string_path = os.path.join(
        BRAVE_SOURCE_ROOT, args.source_string_path[0])
    filename = os.path.basename(source_string_path).split('.')[0]

    if should_use_transifex_for_file(source_string_path, filename):
        print('Transifex: ', source_string_path)
        pull_source_files_from_transifex(source_string_path, filename)
    else:
        print('Local: ', source_string_path)
        override_path = get_override_file_path(source_string_path)
        override_exists = os.path.exists(override_path)
        if override_exists:
            print('Transifex override: ', override_path)
            override_filename = os.path.basename(override_path).split('.')[0]
            pull_source_files_from_transifex(override_path, override_filename)
        else:
            print('No Transifex override.')

        update_xtbs_locally(source_string_path, BRAVE_SOURCE_ROOT)
        if override_exists:
            combine_override_xtb_into_original(source_string_path)


if __name__ == '__main__':
    sys.exit(main())
